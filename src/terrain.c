#include "terrain.h"
#include "pet_data.h"
#include "raymath.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ---- Simple procedural noise (value noise with interpolation) ----

unsigned int HashInt(int x, int z) {
    unsigned int n = (unsigned short)(x * 374761393 + z * 668265263);
    n = (n ^ (n >> 13)) * 1274126177;
    return n;
}

static float HashFloat(int x, int z) {
    return (float)(HashInt(x, z) & 0x7fffffff) / (float)0x7fffffff;
}

static float SmoothNoise(float x, float z) {
    int ix = (int)floorf(x);
    int iz = (int)floorf(z);
    float fx = x - (float)ix;
    float fz = z - (float)iz;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);

    float n00 = HashFloat(ix, iz);
    float n10 = HashFloat(ix + 1, iz);
    float n01 = HashFloat(ix, iz + 1);
    float n11 = HashFloat(ix + 1, iz + 1);
    float nx0 = n00 + (n10 - n00) * fx;
    float nx1 = n01 + (n11 - n01) * fx;
    return nx0 + (nx1 - nx0) * fz;
}

static float ProceduralHeight(float x, float z) {
    float h = 0.0f;
    h += SmoothNoise(x * 0.005f, z * 0.005f) * 22.0f;
    h += SmoothNoise(x * 0.015f + 100.0f, z * 0.015f + 200.0f) * 8.0f;
    h += SmoothNoise(x * 0.04f - 50.0f, z * 0.04f + 100.0f) * 3.0f;
    h += SmoothNoise(x * 0.1f + 300.0f, z * 0.1f - 150.0f) * 1.0f;
    return h;
}

#define TERRAIN_H_MIN (-3.0f)
#define TERRAIN_H_MAX 29.0f
#define TERRAIN_H_RANGE (TERRAIN_H_MAX - TERRAIN_H_MIN)

static float NormalizeHeight(float rawH) {
    float norm = (rawH - TERRAIN_H_MIN) / TERRAIN_H_RANGE;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

// ---- Terrain mesh ----

#define MESH_RES 255
#define MESH_SPAN 512.0f
#define HALF_SPAN (MESH_SPAN * 0.5f)
#define WATER_LEVEL 0.22f
#define RECENTER_THRESHOLD 180.0f

static unsigned char LerpU8(unsigned char a, unsigned char b, float t) {
    float v = (float)a + ((float)b - (float)a) * t;
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}

static Color BlendTerrainColor(float water, float grass, float rock, float norm) {
    unsigned char r, g, b;
    if (water > 0.5f) {
        float depth = 1.0f - (norm / WATER_LEVEL);
        if (depth > 1.0f) depth = 1.0f;
        r = LerpU8(80, 15, depth);
        g = LerpU8(160, 35, depth);
        b = LerpU8(230, 130, depth);
    } else if (rock > grass) {
        float t = norm;
        r = LerpU8(120, 175, t);
        g = LerpU8(110, 170, t);
        b = LerpU8(105, 165, t);
    } else {
        float t = norm;
        r = LerpU8(40, 115, t);
        g = LerpU8(120, 175, t);
        b = LerpU8(45, 60, t);
    }
    return (Color){r, g, b, 255};
}

static float SampleHm(float *hm, int res, int x, int z) {
    if (x < 0) x = 0;
    if (z < 0) z = 0;
    if (x >= res) x = res - 1;
    if (z >= res) z = res - 1;
    return hm[z * res + x];
}

// ---- Build terrain mesh at given world center ----

static void BuildTerrainAt(Terrain *t, float centerX, float centerZ) {
    t->centerX = centerX;
    t->centerZ = centerZ;
    t->meshRes = MESH_RES;
    t->meshSpan = MESH_SPAN;
    t->position = (Vector3){centerX - HALF_SPAN, 0.0f, centerZ - HALF_SPAN};
    t->size = (Vector3){MESH_SPAN, 32.0f, MESH_SPAN};

    int numVerts = MESH_RES * MESH_RES;
    int numTris = (MESH_RES - 1) * (MESH_RES - 1) * 2;
    float step = MESH_SPAN / (float)(MESH_RES - 1);

    t->heightmap = (float *)malloc(numVerts * sizeof(float));
    Image img = GenImageColor(MESH_RES, MESH_RES, BLACK);

    // Pass 1: raw normalized heights
    for (int z = 0; z < MESH_RES; z++) {
        for (int x = 0; x < MESH_RES; x++) {
            float wx = t->position.x + (float)x * step;
            float wz = t->position.z + (float)z * step;
            float rawH = ProceduralHeight(wx, wz);
            t->heightmap[z * MESH_RES + x] = NormalizeHeight(rawH);
        }
    }

    // Pass 2: water flatten (with smooth shore), slope, channel mix, color
    #define SHORE_WIDTH 0.05f
    for (int z = 0; z < MESH_RES; z++) {
        for (int x = 0; x < MESH_RES; x++) {
            int vi = z * MESH_RES + x;
            float norm = t->heightmap[vi];
            float water, grass, rock;

            if (norm <= WATER_LEVEL) {
                t->heightmap[vi] = WATER_LEVEL;
                water = 1.0f; grass = 0.0f; rock = 0.0f;
                norm = WATER_LEVEL;
            } else if (norm < WATER_LEVEL + SHORE_WIDTH) {
                // Smooth shore: bevel geometry + blend colors
                float tShore = (norm - WATER_LEVEL) / SHORE_WIDTH;
                float landNorm = norm;
                float smoothed = WATER_LEVEL + tShore * (landNorm - WATER_LEVEL);
                t->heightmap[vi] = smoothed;
                norm = smoothed;
                // Compute underlying land type
                float hL = SampleHm(t->heightmap, MESH_RES, x - 1, z);
                float hR = SampleHm(t->heightmap, MESH_RES, x + 1, z);
                float hD = SampleHm(t->heightmap, MESH_RES, x, z - 1);
                float hU = SampleHm(t->heightmap, MESH_RES, x, z + 1);
                float dhdx = (hR - hL) * t->size.y / (2.0f * step);
                float dhdz = (hU - hD) * t->size.y / (2.0f * step);
                float slope = sqrtf(dhdx * dhdx + dhdz * dhdz);
                float slopeNorm = slope / 1.2f;
                if (slopeNorm > 1.0f) slopeNorm = 1.0f;
                float rVal = slopeNorm * 0.55f + fmaxf(0.0f, (landNorm - 0.55f) / 0.27f) * 0.45f;
                if (rVal > 1.0f) rVal = 1.0f;
                water = 1.0f - tShore;
                grass = tShore * (1.0f - rVal);
                rock = tShore * rVal;
            } else if (norm > 0.82f) {
                water = 0.0f; grass = 0.0f; rock = 1.0f;
            } else {
                float hL = SampleHm(t->heightmap, MESH_RES, x - 1, z);
                float hR = SampleHm(t->heightmap, MESH_RES, x + 1, z);
                float hD = SampleHm(t->heightmap, MESH_RES, x, z - 1);
                float hU = SampleHm(t->heightmap, MESH_RES, x, z + 1);
                float dhdx = (hR - hL) * t->size.y / (2.0f * step);
                float dhdz = (hU - hD) * t->size.y / (2.0f * step);
                float slope = sqrtf(dhdx * dhdx + dhdz * dhdz);
                float slopeNorm = slope / 1.2f;
                if (slopeNorm > 1.0f) slopeNorm = 1.0f;
                rock = slopeNorm * 0.55f + fmaxf(0.0f, (norm - 0.55f) / 0.27f) * 0.45f;
                if (rock > 1.0f) rock = 1.0f;
                grass = 1.0f - rock;
                water = 0.0f;
            }

            ImageDrawPixel(&img, x, z, BlendTerrainColor(water, grass, rock, norm));
        }
    }

    t->image = img;
    Texture2D texture = LoadTextureFromImage(img);

    // ---- Build mesh ----
    Mesh mesh = {0};
    mesh.vertexCount = numVerts;
    mesh.triangleCount = numTris;

    mesh.vertices = (float *)MemAlloc(numVerts * 3 * sizeof(float));
    mesh.texcoords = (float *)MemAlloc(numVerts * 2 * sizeof(float));
    mesh.normals = (float *)MemAlloc(numVerts * 3 * sizeof(float));
    mesh.indices = (unsigned short *)MemAlloc(numTris * 3 * sizeof(unsigned short));

    memset(mesh.normals, 0, numVerts * 3 * sizeof(float));

    for (int z = 0; z < MESH_RES; z++) {
        for (int x = 0; x < MESH_RES; x++) {
            int vi = z * MESH_RES + x;
            float vy = t->heightmap[vi] * t->size.y;
            mesh.vertices[vi * 3 + 0] = (float)x * step;
            mesh.vertices[vi * 3 + 1] = vy;
            mesh.vertices[vi * 3 + 2] = (float)z * step;
            mesh.texcoords[vi * 2 + 0] = (float)x / (float)(MESH_RES - 1);
            mesh.texcoords[vi * 2 + 1] = (float)z / (float)(MESH_RES - 1);
        }
    }

    // Indices (32-bit)
    for (int z = 0; z < MESH_RES - 1; z++) {
        for (int x = 0; x < MESH_RES - 1; x++) {
            int ti = (z * (MESH_RES - 1) + x) * 6;
            int vi = z * MESH_RES + x;
            mesh.indices[ti + 0] = (unsigned short)(vi);
            mesh.indices[ti + 1] = (unsigned short)(vi + MESH_RES);
            mesh.indices[ti + 2] = (unsigned short)(vi + 1);
            mesh.indices[ti + 3] = (unsigned short)(vi + 1);
            mesh.indices[ti + 4] = (unsigned short)(vi + MESH_RES);
            mesh.indices[ti + 5] = (unsigned short)(vi + MESH_RES + 1);
        }
    }

    // Normals
    for (int ti = 0; ti < numTris; ti++) {
        unsigned short i0 = mesh.indices[ti * 3 + 0];
        unsigned short i1 = mesh.indices[ti * 3 + 1];
        unsigned short i2 = mesh.indices[ti * 3 + 2];
        float *v0 = &mesh.vertices[i0 * 3];
        float *v1 = &mesh.vertices[i1 * 3];
        float *v2 = &mesh.vertices[i2 * 3];
        float e1x = v1[0] - v0[0], e1y = v1[1] - v0[1], e1z = v1[2] - v0[2];
        float e2x = v2[0] - v0[0], e2y = v2[1] - v0[1], e2z = v2[2] - v0[2];
        float nx = e1y * e2z - e1z * e2y;
        float ny = e1z * e2x - e1x * e2z;
        float nz = e1x * e2y - e1y * e2x;
        mesh.normals[i0 * 3 + 0] += nx; mesh.normals[i0 * 3 + 1] += ny; mesh.normals[i0 * 3 + 2] += nz;
        mesh.normals[i1 * 3 + 0] += nx; mesh.normals[i1 * 3 + 1] += ny; mesh.normals[i1 * 3 + 2] += nz;
        mesh.normals[i2 * 3 + 0] += nx; mesh.normals[i2 * 3 + 1] += ny; mesh.normals[i2 * 3 + 2] += nz;
    }
    for (int i = 0; i < numVerts; i++) {
        float nx = mesh.normals[i * 3 + 0], ny = mesh.normals[i * 3 + 1], nz = mesh.normals[i * 3 + 2];
        float len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len > 0.0001f) {
            mesh.normals[i * 3 + 0] /= len; mesh.normals[i * 3 + 1] /= len; mesh.normals[i * 3 + 2] /= len;
        } else {
            mesh.normals[i * 3 + 1] = 1.0f;
        }
    }

    UploadMesh(&mesh, false);
    t->model = LoadModelFromMesh(mesh);
    t->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
}

// ---- Public API ----

Terrain LoadTerrain(void) {
    Terrain terrain = {0};
    BuildTerrainAt(&terrain, 0.0f, 0.0f);
    return terrain;
}

void RecenterTerrain(Terrain *t, float newCenterX, float newCenterZ) {
    UnloadModel(t->model);
    UnloadImage(t->image);
    free(t->heightmap);
    t->heightmap = NULL;
    BuildTerrainAt(t, newCenterX, newCenterZ);
}

bool TerrainNeedsRecenter(Terrain *t, float playerX, float playerZ) {
    float dx = playerX - t->centerX;
    float dz = playerZ - t->centerZ;
    return (fabsf(dx) > RECENTER_THRESHOLD || fabsf(dz) > RECENTER_THRESHOLD);
}

// ---- Height query (bilinear, matches GPU) ----

float GetTerrainHeight(Terrain *terrain, float x, float z) {
    float fx = (x - terrain->position.x) * (float)(terrain->meshRes - 1) / terrain->size.x;
    float fz = (z - terrain->position.z) * (float)(terrain->meshRes - 1) / terrain->size.z;

    if (fx < 0.0f) fx = 0.0f;
    if (fz < 0.0f) fz = 0.0f;
    if (fx > (float)(terrain->meshRes - 1)) fx = (float)(terrain->meshRes - 1);
    if (fz > (float)(terrain->meshRes - 1)) fz = (float)(terrain->meshRes - 1);

    int ix = (int)fx, iz = (int)fz;
    float fracX = fx - (float)ix, fracZ = fz - (float)iz;
    int ix2 = (ix + 1 < terrain->meshRes) ? ix + 1 : ix;
    int iz2 = (iz + 1 < terrain->meshRes) ? iz + 1 : iz;

    float *hm = terrain->heightmap;
    float h00 = hm[iz  * terrain->meshRes + ix];
    float h10 = hm[iz  * terrain->meshRes + ix2];
    float h01 = hm[iz2 * terrain->meshRes + ix];
    float h11 = hm[iz2 * terrain->meshRes + ix2];
    float h0 = h00 + (h10 - h00) * fracX;
    float h1 = h01 + (h11 - h01) * fracX;
    float h = h0 + (h1 - h0) * fracZ;

    return terrain->position.y + h * terrain->size.y;
}

// ---- Terrain type query ----

void GetTerrainType(Terrain *t, float x, float z, float *outWater, float *outGrass, float *outRock) {
    float h = GetTerrainHeight(t, x, z);
    float norm = (h - t->position.y) / t->size.y;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    if (norm <= WATER_LEVEL + 0.01f) {
        *outWater = 1.0f; *outGrass = 0.0f; *outRock = 0.0f;
        return;
    }
    if (norm > 0.82f) {
        *outWater = 0.0f; *outGrass = 0.0f; *outRock = 1.0f;
        return;
    }

    float s = 0.3f;
    float hL = GetTerrainHeight(t, x - s, z);
    float hR = GetTerrainHeight(t, x + s, z);
    float hD = GetTerrainHeight(t, x, z - s);
    float hU = GetTerrainHeight(t, x, z + s);
    float dhdx = (hR - hL) / (2.0f * s);
    float dhdz = (hU - hD) / (2.0f * s);
    float slope = sqrtf(dhdx * dhdx + dhdz * dhdz);
    float slopeNorm = slope / 1.2f;
    if (slopeNorm > 1.0f) slopeNorm = 1.0f;

    float rock = slopeNorm * 0.55f + fmaxf(0.0f, (norm - 0.55f) / 0.27f) * 0.45f;
    if (rock > 1.0f) rock = 1.0f;
    *outWater = 0.0f;
    *outGrass = 1.0f - rock;
    *outRock = rock;
}

void DrawTerrain(Terrain *terrain) {
    DrawModel(terrain->model, terrain->position, 1.0f, WHITE);
}

void UnloadTerrain(Terrain *terrain) {
    UnloadModel(terrain->model);
    UnloadImage(terrain->image);
    free(terrain->heightmap);
    terrain->heightmap = NULL;
}

// ---- Procedural object placement ----

bool ShouldPlaceTree(Terrain *t, int cellX, int cellZ, float *outOffX, float *outOffZ) {
    unsigned int h = HashInt(cellX, cellZ);
    float r = (float)(h & 0xffff) / 65536.0f;
    if (r <= 0.86f) return false;

    float cx = (float)cellX * 6.0f;
    float cz = (float)cellZ * 6.0f;
    float water, grass, rock;
    GetTerrainType(t, cx, cz, &water, &grass, &rock);
    if (water > 0.1f) return false;
    if (grass < 0.75f) return false;

    *outOffX = ((float)((h >> 16) & 0xffff) / 65536.0f - 0.5f) * 2.5f;
    *outOffZ = ((float)((h >> 20) & 0xffff) / 65536.0f - 0.5f) * 2.5f;
    return true;
}

bool ShouldPlaceRock(Terrain *t, int cellX, int cellZ, float *outOffX, float *outOffZ) {
    unsigned int h = HashInt(cellX + 9999, cellZ - 5555);

    float cx = (float)cellX * 6.0f;
    float cz = (float)cellZ * 6.0f;
    float water, grass, rock;
    GetTerrainType(t, cx, cz, &water, &grass, &rock);
    if (water > 0.1f) return false;

    float density;
    if (rock > 0.75f)       density = 0.40f;
    else if (rock > 0.5f)   density = 0.22f;
    else if (grass > 0.75f) density = 0.06f;
    else                    return false;

    float r = (float)(h & 0xffff) / 65536.0f;
    if (r <= 1.0f - density) return false;

    *outOffX = ((float)((h >> 16) & 0xffff) / 65536.0f - 0.5f) * 2.0f;
    *outOffZ = ((float)((h >> 20) & 0xffff) / 65536.0f - 0.5f) * 2.0f;
    return true;
}

// ---- Procedural trainer generation ----

bool ShouldPlaceTrainer(int cellX, int cellZ, int *outSpeciesId, int *outLevel,
                        int *outNameIdx, int *outColorIdx) {
    unsigned int h = HashInt(cellX * 3 + 7777, cellZ * 5 - 3333);
    float r = (float)(h & 0xffff) / 65536.0f;
    if (r > 0.975f) {
        *outSpeciesId = (int)((h >> 16) & 0x7) % PET_COUNT;
        *outLevel = 3 + (int)((h >> 20) & 0xf) % 8;
        *outNameIdx = (int)((h >> 8) & 0x7);
        *outColorIdx = (int)((h >> 24) & 0x7);
        return true;
    }
    return false;
}

unsigned int GetTrainerHash(int cellX, int cellZ) {
    return HashInt(cellX * 3 + 7777, cellZ * 5 - 3333);
}

// ---- Collision queries ----

#define TREE_TRUNK_RADIUS 0.6f
#define ROCK_COL_RADIUS 1.2f

bool GetTreeCollision(Terrain *t, float wx, float wz, Vector3 *outPos, float *outRadius) {
    int pcx = (int)floorf(wx / 6.0f);
    int pcz = (int)floorf(wz / 6.0f);
    for (int dcx = -1; dcx <= 1; dcx++) {
        for (int dcz = -1; dcz <= 1; dcz++) {
            int cx = pcx + dcx, cz = pcz + dcz;
            float offX, offZ;
            if (!ShouldPlaceTree(t, cx, cz, &offX, &offZ)) continue;
            float tx = (float)cx * 6.0f + offX;
            float tz = (float)cz * 6.0f + offZ;
            float dx = wx - tx, dz = wz - tz;
            float minDist = TREE_TRUNK_RADIUS + 0.5f;
            if (dx * dx + dz * dz < minDist * minDist) {
                if (outPos) { outPos->x = tx; outPos->y = 0; outPos->z = tz; }
                if (outRadius) *outRadius = TREE_TRUNK_RADIUS;
                return true;
            }
        }
    }
    return false;
}

bool GetRockCollision(Terrain *t, float wx, float wz, Vector3 *outPos, float *outRadius) {
    int pcx = (int)floorf(wx / 6.0f);
    int pcz = (int)floorf(wz / 6.0f);
    for (int dcx = -1; dcx <= 1; dcx++) {
        for (int dcz = -1; dcz <= 1; dcz++) {
            int cx = pcx + dcx, cz = pcz + dcz;
            float offX, offZ;
            if (!ShouldPlaceRock(t, cx, cz, &offX, &offZ)) continue;
            float rx = (float)cx * 6.0f + offX;
            float rz = (float)cz * 6.0f + offZ;
            float dx = wx - rx, dz = wz - rz;
            float minDist = ROCK_COL_RADIUS + 0.5f;
            if (dx * dx + dz * dz < minDist * minDist) {
                if (outPos) { outPos->x = rx; outPos->y = 0; outPos->z = rz; }
                if (outRadius) *outRadius = ROCK_COL_RADIUS;
                return true;
            }
        }
    }
    return false;
}
