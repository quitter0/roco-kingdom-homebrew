#include "models_3d.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "external/cgltf.h"

// Re-load a GLB file and convert to non-indexed mesh (fixes 16-bit index overflow)
static Mesh LoadGLBNonIndexed(const char *fileName)
{
    Mesh mesh = {0};
    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);
    if (!fileData)
        return mesh;

    cgltf_options opts = {0};
    cgltf_data *data = NULL;
    if (cgltf_parse(&opts, fileData, dataSize, &data) != cgltf_result_success)
    {
        UnloadFileData(fileData);
        return mesh;
    }
    if (cgltf_load_buffers(&opts, data, fileName) != cgltf_result_success)
    {
        cgltf_free(data);
        UnloadFileData(fileData);
        return mesh;
    }

    // Count total triangles across all primitives in the first mesh
    int totalTris = 0;
    cgltf_mesh *gltfMesh = &data->meshes[0];

    // Quantized glTF (KHR_mesh_quantization) stores positions in integer space
    // and restores the original scale/offset through the node transform. This
    // path bypasses raylib's glTF loader, so apply that transform manually.
    Matrix world = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    for (cgltf_size n = 0; n < data->nodes_count; n++) {
        if (data->nodes[n].mesh != gltfMesh) continue;
        cgltf_float wt[16];
        cgltf_node_transform_world(&data->nodes[n], wt);
        world = (Matrix){
            wt[0], wt[4], wt[8], wt[12],
            wt[1], wt[5], wt[9], wt[13],
            wt[2], wt[6], wt[10], wt[14],
            wt[3], wt[7], wt[11], wt[15]
        };
        break;
    }
    for (cgltf_size p = 0; p < gltfMesh->primitives_count; p++)
    {
        cgltf_primitive *prim = &gltfMesh->primitives[p];
        if (prim->type != cgltf_primitive_type_triangles)
            continue;
        cgltf_accessor *idxAcc = prim->indices;
        totalTris += (int)(idxAcc ? idxAcc->count : prim->attributes[0].data->count) / 3;
    }

    int newVtxCount = totalTris * 3;
    float *newVerts = (float *)RL_MALLOC(newVtxCount * 3 * sizeof(float));
    float *newNorms = (float *)RL_MALLOC(newVtxCount * 3 * sizeof(float));
    float *newTexcs = (float *)RL_MALLOC(newVtxCount * 2 * sizeof(float));
    memset(newVerts, 0, newVtxCount * 3 * sizeof(float));
    memset(newNorms, 0, newVtxCount * 3 * sizeof(float));
    memset(newTexcs, 0, newVtxCount * 2 * sizeof(float));

    int dstTri = 0;
    for (cgltf_size p = 0; p < gltfMesh->primitives_count; p++)
    {
        cgltf_primitive *prim = &gltfMesh->primitives[p];
        if (prim->type != cgltf_primitive_type_triangles)
            continue;

        // Read vertex position accessor
        cgltf_accessor *posAcc = NULL;
        cgltf_accessor *nrmAcc = NULL;
        cgltf_accessor *uvAcc = NULL;
        for (cgltf_size a = 0; a < prim->attributes_count; a++)
        {
            if (prim->attributes[a].type == cgltf_attribute_type_position)
                posAcc = prim->attributes[a].data;
            else if (prim->attributes[a].type == cgltf_attribute_type_normal)
                nrmAcc = prim->attributes[a].data;
            else if (prim->attributes[a].type == cgltf_attribute_type_texcoord)
                uvAcc = prim->attributes[a].data;
        }

        float *posBuf = (float *)RL_MALLOC(posAcc->count * 3 * sizeof(float));
        float *nrmBuf = nrmAcc ? (float *)RL_MALLOC(nrmAcc->count * 3 * sizeof(float)) : NULL;
        float *uvBuf = uvAcc ? (float *)RL_MALLOC(uvAcc->count * 2 * sizeof(float)) : NULL;
        cgltf_accessor_unpack_floats(posAcc, posBuf, posAcc->count * 3);
        if (nrmAcc)
            cgltf_accessor_unpack_floats(nrmAcc, nrmBuf, nrmAcc->count * 3);
        if (uvAcc)
            cgltf_accessor_unpack_floats(uvAcc, uvBuf, uvAcc->count * 2);

        for (cgltf_size v = 0; v < posAcc->count; v++)
        {
            Vector3 p = { posBuf[v*3], posBuf[v*3 + 1], posBuf[v*3 + 2] };
            p = Vector3Transform(p, world);
            posBuf[v*3] = p.x;
            posBuf[v*3 + 1] = p.y;
            posBuf[v*3 + 2] = p.z;
        }
        if (nrmAcc)
        {
            for (cgltf_size v = 0; v < nrmAcc->count; v++)
            {
                Vector3 n = { nrmBuf[v*3], nrmBuf[v*3 + 1], nrmBuf[v*3 + 2] };
                Vector3 tn = {
                    n.x*world.m0 + n.y*world.m4 + n.z*world.m8,
                    n.x*world.m1 + n.y*world.m5 + n.z*world.m9,
                    n.x*world.m2 + n.y*world.m6 + n.z*world.m10
                };
                tn = Vector3Normalize(tn);
                nrmBuf[v*3] = tn.x;
                nrmBuf[v*3 + 1] = tn.y;
                nrmBuf[v*3 + 2] = tn.z;
            }
        }

        // Read indices as uint32 (cgltf always unpacks to uint32 for index accessors)
        cgltf_accessor *idxAcc = prim->indices;
        unsigned int *indexBuf = NULL;
        if (idxAcc)
        {
            indexBuf = (unsigned int *)RL_MALLOC(idxAcc->count * sizeof(unsigned int));
            cgltf_accessor_unpack_indices(idxAcc, indexBuf, sizeof(unsigned int), idxAcc->count);
        }

        int triCount = (int)(idxAcc ? idxAcc->count : posAcc->count) / 3;
        for (int t = 0; t < triCount; t++)
        {
            for (int v = 0; v < 3; v++)
            {
                int src = indexBuf ? (int)indexBuf[t * 3 + v] : (t * 3 + v);
                int dst = (dstTri + t) * 3 + v;
                memcpy(&newVerts[dst * 3], &posBuf[src * 3], 3 * sizeof(float));
                if (nrmBuf)
                    memcpy(&newNorms[dst * 3], &nrmBuf[src * 3], 3 * sizeof(float));
                if (uvBuf)
                    memcpy(&newTexcs[dst * 2], &uvBuf[src * 2], 2 * sizeof(float));
            }
        }

        dstTri += triCount;
        RL_FREE(posBuf);
        if (nrmBuf)
            RL_FREE(nrmBuf);
        if (uvBuf)
            RL_FREE(uvBuf);
        if (indexBuf)
            RL_FREE(indexBuf);
    }

    cgltf_free(data);
    UnloadFileData(fileData);

    mesh.vertexCount = newVtxCount;
    mesh.triangleCount = newVtxCount / 3;
    mesh.vertices = newVerts;
    mesh.normals = newNorms;
    mesh.texcoords = newTexcs;
    mesh.indices = NULL; // non-indexed rendering — no 16-bit overflow

    UploadMesh(&mesh, false);
    return mesh;
}

// Character GLB model cache
static Model g_charModels[CHAR_MODEL_COUNT];
static bool g_charModelLoaded[CHAR_MODEL_COUNT] = {false};
static float g_charModelScale[CHAR_MODEL_COUNT] = {1.0f};
static float g_charModelYOff[CHAR_MODEL_COUNT] = {0.0f};
static const char *g_charModelFiles[CHAR_MODEL_COUNT] = {
    "resources/characters/protagonist.glb",
    "resources/characters/nurse.glb",
    "resources/characters/merchant.glb",
    "resources/characters/old.glb",
    "resources/characters/researcher.glb",
};
// Per-character Y-rotation to face +Z (default model forward)
#define MY_PI 3.14159265
static float g_charModelFacing[CHAR_MODEL_COUNT] = {
    0.0f,    // protagonist
    PI / 2,  // nurse
    -PI / 2, // merchant
    -PI / 4, // old
    PI / 2,  // researcher
};

// GLB model cache — two copies per species: enemy-facing + player-facing
static Model g_petModels[8];    // pre-rotated to face -Z (enemy)
static Model g_petModelsRot[8]; // pre-rotated to face +Z (player pet)
static bool g_petModelLoaded[8] = {false};
static float g_petModelScale[8] = {1.0f};
// GLB file index → species ID: pet_0=火花,1=喵喵,2=水蓝蓝,3=格兰花,6=雪精灵,7=幽灵,8=瞌睡王,9=小包子
static int glbToSpecies[10] = {0, 2, 1, 4, -1, -1, 5, 7, 3, 6};
// Per-species Y-rotation offset (radians) to make the raw GLB model face +Z
// Tune these so each pet faces the camera when viewed as the player's pet
static float g_petModelFacing[8] = {
    MY_PI / 3,       // 0: 火花
    MY_PI / 2,       // 1: 水蓝蓝
    MY_PI * 3 / 4,   // 2: 喵喵
    MY_PI * 13 / 15, // 3: 瞌睡王
    MY_PI * 3 / 4,   // 4: 格兰花
    MY_PI * 2 / 3,   // 5: 雪精灵
    MY_PI / 10,      // 6: 小包子
    MY_PI / 4,       // 7: 幽灵
};

// Deep-clone a mesh and rotate all vertices+normals around Y axis by angle (radians)
static Mesh CloneMeshRotatedY(const Mesh *src, float angle)
{
    Mesh dst = {0};
    dst.vertexCount = src->vertexCount;
    dst.triangleCount = src->triangleCount;
    int vc = src->vertexCount;
    float ca = cosf(angle), sa = sinf(angle);

    dst.vertices = (float *)RL_MALLOC(vc * 3 * sizeof(float));
    for (int v = 0; v < vc; v++)
    {
        float x = src->vertices[v * 3];
        float z = src->vertices[v * 3 + 2];
        dst.vertices[v * 3] = x * ca - z * sa;
        dst.vertices[v * 3 + 1] = src->vertices[v * 3 + 1];
        dst.vertices[v * 3 + 2] = x * sa + z * ca;
    }

    if (src->normals)
    {
        dst.normals = (float *)RL_MALLOC(vc * 3 * sizeof(float));
        for (int v = 0; v < vc; v++)
        {
            float x = src->normals[v * 3];
            float z = src->normals[v * 3 + 2];
            dst.normals[v * 3] = x * ca - z * sa;
            dst.normals[v * 3 + 1] = src->normals[v * 3 + 1];
            dst.normals[v * 3 + 2] = x * sa + z * ca;
        }
    }

    if (src->texcoords)
    {
        dst.texcoords = (float *)RL_MALLOC(vc * 2 * sizeof(float));
        memcpy(dst.texcoords, src->texcoords, vc * 2 * sizeof(float));
    }
    if (src->colors)
    {
        dst.colors = (unsigned char *)RL_MALLOC(vc * 4);
        memcpy(dst.colors, src->colors, vc * 4);
    }
    if (src->indices)
    {
        int ic = src->triangleCount * 3;
        dst.indices = (unsigned short *)RL_MALLOC(ic * sizeof(unsigned short));
        memcpy(dst.indices, src->indices, ic * sizeof(unsigned short));
    }
    if (src->tangents)
    {
        dst.tangents = (float *)RL_MALLOC(vc * 4 * sizeof(float));
        memcpy(dst.tangents, src->tangents, vc * 4 * sizeof(float));
    }
    if (src->texcoords2)
    {
        dst.texcoords2 = (float *)RL_MALLOC(vc * 2 * sizeof(float));
        memcpy(dst.texcoords2, src->texcoords2, vc * 2 * sizeof(float));
    }
    if (src->boneIndices)
    {
        dst.boneIndices = (unsigned char *)RL_MALLOC(vc * 4);
        memcpy(dst.boneIndices, src->boneIndices, vc * 4);
    }
    if (src->boneWeights)
    {
        dst.boneWeights = (float *)RL_MALLOC(vc * 4 * sizeof(float));
        memcpy(dst.boneWeights, src->boneWeights, vc * 4 * sizeof(float));
    }
    UploadMesh(&dst, false);
    return dst;
}

// Load a GLB model silently (suppress raylib's u32→u16 conversion warnings)
static Model LoadModelSilent(const char *path)
{
    SetTraceLogLevel(LOG_ERROR);  // suppress LOG_WARNING
    Model m = LoadModel(path);
    SetTraceLogLevel(LOG_WARNING);
    return m;
}

static void LoadOneGLB(int glbIdx)
{
    int sMY_pId = glbToSpecies[glbIdx];
    if (sMY_pId < 0 || sMY_pId >= 8)
        return;
    char path[64];
    snprintf(path, sizeof(path), "resources/models/pet_%d.glb", glbIdx);
    if (!FileExists(path))
        return;

    Model loaded = LoadModelSilent(path);
    if (loaded.meshCount <= 0 || loaded.meshes == NULL)
        return;

    // Fix 16-bit index overflow for models with >65535 vertices
    for (int mi = 0; mi < loaded.meshCount; mi++)
    {
        Mesh *m = &loaded.meshes[mi];
        if (m->indices != NULL && m->vertexCount > 65535)
        {
            Mesh fixed = LoadGLBNonIndexed(path);
            if (fixed.vertexCount > 0)
            {
                UnloadMesh(*m);
                *m = fixed;
            }
            break;
        }
    }

    // Compute scale from bounding box
    BoundingBox bb = GetModelBoundingBox(loaded);
    float maxDim = bb.max.x - bb.min.x;
    if (bb.max.y - bb.min.y > maxDim)
        maxDim = bb.max.y - bb.min.y;
    if (bb.max.z - bb.min.z > maxDim)
        maxDim = bb.max.z - bb.min.z;
    if (maxDim > 0.0001f)
        g_petModelScale[sMY_pId] = 1.5f / maxDim;
    if (g_petModelScale[sMY_pId] > 1.5f)
        g_petModelScale[sMY_pId] = 1.5f;
    if (g_petModelScale[sMY_pId] < 0.01f)
        g_petModelScale[sMY_pId] = 0.01f;

    float baseAngle = g_petModelFacing[sMY_pId];

    // Enemy model (far side): pre-rotate to face toward the player
    g_petModels[sMY_pId] = loaded;
    g_petModels[sMY_pId].meshes = (Mesh *)RL_MALLOC(loaded.meshCount * sizeof(Mesh));
    for (int mi = 0; mi < loaded.meshCount; mi++)
        g_petModels[sMY_pId].meshes[mi] = CloneMeshRotatedY(&loaded.meshes[mi], baseAngle);

    // Player model (near side): pre-rotate to face toward the enemy (+Z)
    g_petModelsRot[sMY_pId] = loaded;
    g_petModelsRot[sMY_pId].meshes = (Mesh *)RL_MALLOC(loaded.meshCount * sizeof(Mesh));
    for (int mi = 0; mi < loaded.meshCount; mi++)
        g_petModelsRot[sMY_pId].meshes[mi] = CloneMeshRotatedY(&loaded.meshes[mi], baseAngle + 3.14159265f);

    // Free original loaded meshes (now replaced by pre-rotated copies)
    for (int mi = 0; mi < loaded.meshCount; mi++)
        UnloadMesh(loaded.meshes[mi]);
    RL_FREE(loaded.meshes);

    g_petModelLoaded[sMY_pId] = true;
}

void InitPetModels(void)
{
    memset(g_petModels, 0, sizeof(g_petModels));
    memset(g_petModelsRot, 0, sizeof(g_petModelsRot));
    for (int i = 0; i < 10; i++)
        LoadOneGLB(i);
}

void UnloadPetModels(void)
{
    for (int i = 0; i < 8; i++)
    {
        if (g_petModelLoaded[i])
        {
            // The rotated copy shares materials/meshMaterial/skeleton with the
            // primary model, so only its cloned meshes are owned separately.
            for (int mi = 0; mi < g_petModelsRot[i].meshCount; mi++)
                UnloadMesh(g_petModelsRot[i].meshes[mi]);
            RL_FREE(g_petModelsRot[i].meshes);
            g_petModelsRot[i].meshes = NULL;

            // UnloadModel also unloads the primary model's meshes and frees
            // the shared material/skeleton arrays.
            UnloadModel(g_petModels[i]);
            g_petModelLoaded[i] = false;
        }
    }
}

static void LoadOneCharModel(int ct)
{
    if (ct < 0 || ct >= CHAR_MODEL_COUNT)
        return;
    const char *path = g_charModelFiles[ct];
    if (!FileExists(path))
        return;

    Model loaded = LoadModelSilent(path);
    if (loaded.meshCount <= 0 || loaded.meshes == NULL)
        return;

    for (int mi = 0; mi < loaded.meshCount; mi++)
    {
        Mesh *m = &loaded.meshes[mi];
        if (m->indices != NULL && m->vertexCount > 65535)
        {
            Mesh fixed = LoadGLBNonIndexed(path);
            if (fixed.vertexCount > 0)
            {
                UnloadMesh(*m);
                *m = fixed;
            }
            break;
        }
    }

    BoundingBox bb = GetModelBoundingBox(loaded);
    float maxDim = bb.max.x - bb.min.x;
    if (bb.max.y - bb.min.y > maxDim)
        maxDim = bb.max.y - bb.min.y;
    if (bb.max.z - bb.min.z > maxDim)
        maxDim = bb.max.z - bb.min.z;
    if (maxDim > 0.0001f)
        g_charModelScale[ct] = 2.0f / maxDim;
    if (g_charModelScale[ct] > 4.0f)
        g_charModelScale[ct] = 4.0f;
    if (g_charModelScale[ct] < 0.01f)
        g_charModelScale[ct] = 0.01f;
    // Save Y offset so feet (or lowest point) rest on ground
    g_charModelYOff[ct] = bb.min.y;

    float angle = g_charModelFacing[ct];
    g_charModels[ct] = loaded;
    g_charModels[ct].meshes = (Mesh *)RL_MALLOC(loaded.meshCount * sizeof(Mesh));
    for (int mi = 0; mi < loaded.meshCount; mi++)
        g_charModels[ct].meshes[mi] = CloneMeshRotatedY(&loaded.meshes[mi], angle);

    for (int mi = 0; mi < loaded.meshCount; mi++)
        UnloadMesh(loaded.meshes[mi]);
    RL_FREE(loaded.meshes);

    g_charModelLoaded[ct] = true;
}

void InitCharacterModels(void)
{
    memset(g_charModels, 0, sizeof(g_charModels));
    for (int i = 0; i < CHAR_MODEL_COUNT; i++)
        LoadOneCharModel(i);
}

void UnloadCharacterModels(void)
{
    for (int i = 0; i < CHAR_MODEL_COUNT; i++)
    {
        if (g_charModelLoaded[i])
        {
            UnloadModel(g_charModels[i]);
            g_charModelLoaded[i] = false;
        }
    }
}

static inline Vector3 R(Vector3 base, float lx, float ly, float lz, float ca, float sa)
{
    return (Vector3){base.x + lx * ca - lz * sa, base.y + ly, base.z + lx * sa + lz * ca};
}

// Draw a Pokemon-style eye (horizontal oval, puMY_pil, highlight)
static void PkmnEye(Vector3 pos, float size, Color iris, float facingSign)
{
    float sx = facingSign > 0 ? 1.0f : -1.0f;
    // White sclera
    DrawSphere(pos, size, WHITE);
    DrawSphere((Vector3){pos.x + sx * size * 0.3f, pos.y + size * 0.1f, pos.z}, size * 0.55f,
               iris); // puMY_pil
    DrawSphere((Vector3){pos.x + sx * size * 0.15f, pos.y + size * 0.3f, pos.z + size * 0.4f},
               size * 0.22f, WHITE); // highlight
}

// ==================== PLAYER ====================

static void DrawCharModelGLB(int ct, Vector3 pos, float scale, float facingAngle, Color tint)
{
    if (ct < 0 || ct >= CHAR_MODEL_COUNT || !g_charModelLoaded[ct])
        return;
    float ms = scale * g_charModelScale[ct];
    float yOff = -g_charModelYOff[ct] * ms; // lift feet to ground level
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + yOff, pos.z);
    rlRotatef(-facingAngle * RAD2DEG, 0, 1, 0); // negate: game's R() uses opposite convention from OpenGL
    rlScalef(ms, ms, ms);
    DrawModel(g_charModels[ct], (Vector3){0, 0, 0}, 1.0f, tint);
    rlPopMatrix();
}

void DrawPlayerModel(Vector3 pos, Color bodyColor, float scale,
                     float facingAngle, bool isWalking, float walkTime)
{
    int ct = CHAR_MODEL_PLAYER;
    if (g_charModelLoaded[ct])
    {
        float ms = scale * g_charModelScale[ct];
        float bounce = 0;
        if (isWalking)
            bounce = fabsf(sinf(walkTime * 8)) * 0.06f * ms;
        Vector3 drawPos = {pos.x, pos.y + bounce, pos.z};
        DrawCharModelGLB(ct, drawPos, scale, facingAngle, WHITE);
        DrawCylinder((Vector3){pos.x, pos.y + 0.01f, pos.z}, 0.35f * scale, 0.35f * scale, 0.02f * scale, 16, (Color){0, 0, 0, 70});
        return;
    }

    float s = scale, ca = cosf(facingAngle), sa = sinf(facingAngle);
    float wc = 0, bounce = 0, legS = 0, armS = 0;
    if (isWalking)
    {
        wc = walkTime * 8;
        bounce = fabsf(sinf(wc)) * 0.1f * s;
        legS = sinf(wc) * 0.16f * s;
        armS = sinf(wc) * 0.13f * s;
    }
    Color sk = {255, 210, 170, 255}, pn = {50, 50, 90, 255}, bt = {45, 35, 30, 255};
    Color hr = {50, 25, 15, 255}, sc = {255, 70, 40, 255};

    DrawCylinder(R(pos, 0, 0.01f, 0, ca, sa), 0.5f * s, 0.5f * s, 0.02f * s, 16, (Color){0, 0, 0, 70});
    DrawCylinder(R(pos, -0.14f, 0.1f, legS, ca, sa), 0.09f * s, 0.11f * s, 0.65f * s, 8, pn);
    DrawCylinder(R(pos, 0.14f, 0.1f, -legS, ca, sa), 0.09f * s, 0.11f * s, 0.65f * s, 8, pn);
    DrawCylinder(R(pos, -0.14f, 0, legS, ca, sa), 0.13f * s, 0.11f * s, 0.16f * s, 8, bt);
    DrawCylinder(R(pos, 0.14f, 0, -legS, ca, sa), 0.13f * s, 0.11f * s, 0.16f * s, 8, bt);
    DrawCylinder(R(pos, 0, 0.75f + bounce, 0, ca, sa), 0.26f * s, 0.3f * s, 0.8f * s, 12, bodyColor);
    DrawCylinder(R(pos, 0, 0.9f + bounce, 0, ca, sa), 0.31f * s, 0.28f * s, 0.07f * s, 16, (Color){80, 55, 25, 255});
    DrawCube(R(pos, 0, 0.91f + bounce, 0.26f, ca, sa), 0.1f * s, 0.09f * s, 0.04f * s, (Color){220, 180, 40, 255});
    DrawCylinder(R(pos, -0.36f, 1.1f + bounce, -armS, ca, sa), 0.07f * s, 0.06f * s, 0.5f * s, 8, bodyColor);
    DrawSphere(R(pos, -0.36f, 0.58f + bounce, -armS, ca, sa), 0.08f * s, sk);
    DrawCylinder(R(pos, 0.36f, 1.1f + bounce, armS, ca, sa), 0.07f * s, 0.06f * s, 0.5f * s, 8, bodyColor);
    DrawSphere(R(pos, 0.36f, 0.58f + bounce, armS, ca, sa), 0.08f * s, sk);
    Color pk = {bodyColor.r / 2 + 60, bodyColor.g / 2 + 40, bodyColor.b / 2 + 60, 255};
    DrawCube(R(pos, 0, 1.15f + bounce, -0.33f, ca, sa), 0.26f * s, 0.32f * s, 0.14f * s, pk);
    DrawCylinder(R(pos, 0, 1.5f + bounce, 0, ca, sa), 0.08f * s, 0.07f * s, 0.12f * s, 8, sk);
    DrawSphere(R(pos, 0, 1.8f + bounce, 0, ca, sa), 0.3f * s, sk);
    PkmnEye(R(pos, -0.1f, 1.87f + bounce, 0.27f, ca, sa), 0.07f * s, BLACK, 1);
    PkmnEye(R(pos, 0.1f, 1.87f + bounce, 0.27f, ca, sa), 0.07f * s, BLACK, -1);
    DrawCylinder(R(pos, 0, 1.72f + bounce, 0.28f, ca, sa), 0.03f * s, 0.03f * s, 0.05f * s, 6, (Color){200, 130, 110, 255});
    DrawSphere(R(pos, 0, 2.08f + bounce, 0, ca, sa), 0.25f * s, hr);
    DrawSphere(R(pos, -0.18f, 2.0f + bounce, 0.05f, ca, sa), 0.13f * s, hr);
    DrawSphere(R(pos, 0.18f, 2.0f + bounce, 0.05f, ca, sa), 0.13f * s, hr);
    DrawCylinder(R(pos, 0, 2.2f + bounce, 0, ca, sa), 0.18f * s, 0.26f * s, 0.18f * s, 16, bodyColor);
    DrawCylinder(R(pos, 0, 2.35f + bounce, 0, ca, sa), 0.26f * s, 0.3f * s, 0.22f * s, 16, bodyColor);
    DrawSphere(R(pos, 0, 2.55f + bounce, 0, ca, sa), 0.09f * s, WHITE);
    DrawCylinder(R(pos, 0, 1.52f + bounce, 0, ca, sa), 0.18f * s, 0.2f * s, 0.05f * s, 16, sc);
}

// ==================== NPC ====================

void DrawNPCModel(Vector3 pos, Color bodyColor, bool isTrainer, float scale,
                  float facingAngle, int charType)
{
    // Try GLB model if character type specified
    if (charType >= 0 && charType < CHAR_MODEL_COUNT && g_charModelLoaded[charType])
    {
        Vector3 drawPos = {pos.x, pos.y, pos.z};
        DrawCharModelGLB(charType, drawPos, scale, facingAngle, WHITE);
        DrawCylinder((Vector3){pos.x, pos.y + 0.01f, pos.z}, 0.35f * scale, 0.35f * scale, 0.02f * scale, 16, (Color){0, 0, 0, 60});
        return;
    }

    float s = scale, ca = cosf(facingAngle), sa = sinf(facingAngle);
    Color sk = {255, 210, 170, 255}, pn = isTrainer ? (Color){45, 45, 75, 255} : (Color){75, 65, 55, 255};
    Color sh = isTrainer ? (Color){35, 35, 45, 255} : (Color){45, 35, 25, 255};
    Color hr = isTrainer ? (Color){35, 25, 45, 255} : (Color){25, 20, 15, 255};
    DrawCylinder(R(pos, 0, 0.01f, 0, ca, sa), 0.45f * s, 0.45f * s, 0.02f * s, 16, (Color){0, 0, 0, 60});
    DrawCylinder(R(pos, -0.11f, 0.3f, 0, ca, sa), 0.07f * s, 0.09f * s, 0.5f * s, 8, pn);
    DrawCylinder(R(pos, 0.11f, 0.3f, 0, ca, sa), 0.07f * s, 0.09f * s, 0.5f * s, 8, pn);
    DrawCylinder(R(pos, -0.11f, 0.05f, 0, ca, sa), 0.11f * s, 0.09f * s, 0.1f * s, 8, sh);
    DrawCylinder(R(pos, 0.11f, 0.05f, 0, ca, sa), 0.11f * s, 0.09f * s, 0.1f * s, 8, sh);
    DrawCylinder(R(pos, 0, 0.8f, 0, ca, sa), 0.2f * s, 0.24f * s, 0.6f * s, 12, bodyColor);
    if (isTrainer)
        DrawCylinder(R(pos, 0, 0.85f, 0, ca, sa), 0.25f * s, 0.23f * s, 0.05f * s, 16, (Color){180, 150, 40, 255});
    DrawCylinder(R(pos, -0.28f, 1.0f, 0, ca, sa), 0.05f * s, 0.05f * s, 0.4f * s, 8, bodyColor);
    DrawSphere(R(pos, -0.28f, 0.58f, 0, ca, sa), 0.06f * s, sk);
    DrawCylinder(R(pos, 0.28f, 1.0f, 0, ca, sa), 0.05f * s, 0.05f * s, 0.4f * s, 8, bodyColor);
    DrawSphere(R(pos, 0.28f, 0.58f, 0, ca, sa), 0.06f * s, sk);
    DrawCylinder(R(pos, 0, 1.35f, 0, ca, sa), 0.06f * s, 0.05f * s, 0.1f * s, 8, sk);
    DrawSphere(R(pos, 0, 1.6f, 0, ca, sa), 0.24f * s, sk);
    PkmnEye(R(pos, -0.07f, 1.65f, 0.22f, ca, sa), 0.05f * s, BLACK, 1);
    PkmnEye(R(pos, 0.07f, 1.65f, 0.22f, ca, sa), 0.05f * s, BLACK, -1);
    DrawSphere(R(pos, 0, 1.83f, 0, ca, sa), 0.2f * s, hr);
    if (isTrainer)
    {
        DrawCylinder(R(pos, 0, 1.9f, 0, ca, sa), 0.2f * s, 0.23f * s, 0.13f * s, 16, bodyColor);
        DrawCylinder(R(pos, 0, 2.0f, 0, ca, sa), 0.23f * s, 0.2f * s, 0.08f * s, 16, WHITE);
    }
}

// ==================== PET BATTLE MODEL ====================
// Multi-segment body + Pokemon eyes + diverse limbs + unique accessories

static void ApplyHitFlash(Vector3 pos, float s, float intensity)
{
    if (intensity < 0.01f)
        return;
    DrawSphere((Vector3){pos.x, pos.y + 0.55f * s, pos.z + 0.05f * s}, 0.45f * s, (Color){255, 255, 255, (unsigned char)(intensity * 200)});
}

void DrawPetBattleModel(Vector3 pos, int speciesId, float scale,
                        bool isEnemy, bool isAttacking, float animTime,
                        PetType moveType, float animDuration)
{
    int sMY_pId = (speciesId >= 0 && speciesId < (int)PET_COUNT) ? speciesId : 0;
    float s = scale;

    const PetSpecies *species = &PET_DATABASE[sMY_pId];
    Color c = species->color;
    Color dk = {c.r / 3, c.g / 3, c.b / 3, 255};
    Color lt = {c.r + 50 > 255 ? 255 : c.r + 50, c.g + 50 > 255 ? 255 : c.g + 50, c.b + 50 > 255 ? 255 : c.b + 50, 255};
    float bob = sinf(animTime * 3) * 0.08f * s;
    float lunge = 0, squash = 1.0f;
    float chargeRingR = 0, chargeRingAlpha = 0;
    float animDur = animDuration > 0 ? animDuration : 1.0f;
    if (isAttacking)
    {
        float ap = fmodf(animTime, animDur) / animDur;
        if (ap < 0.3f)
        {
            float cp = ap / 0.3f;
            squash = 1.0f - cp * 0.65f;
            chargeRingR = 0.35f * s + cp * 0.25f * s;
            chargeRingAlpha = cp * 0.8f;
        }
        else if (ap < 0.55f)
        {
            float lp = (ap - 0.3f) / 0.25f;
            squash = 0.35f + lp * 1.8f;
            lunge = lp * 3.0f * s;
            chargeRingAlpha = (1 - lp) * 0.8f;
        }
        else if (ap < 0.75f)
        {
            float ip = (ap - 0.55f) / 0.2f;
            squash = 2.15f - ip * 1.3f;
            lunge = (1 - ip) * 3.0f * s;
        }
        else
        {
            float rp = (ap - 0.75f) / 0.25f;
            squash = 0.85f + rp * 0.15f;
        }
        if (moveType == TYPE_FIRE)
        {
            squash = squash < 1 ? squash * 0.8f : squash * 1.12f;
            lunge *= 1.15f;
        }
        else if (moveType == TYPE_WATER)
        {
            squash = squash < 1 ? squash * 1.15f : squash * 0.92f;
            lunge *= 0.88f;
        }
        else if (moveType == TYPE_GRASS)
        {
            lunge *= 1.02f;
        }
    }
    float by = pos.y + bob, lz = isEnemy ? lunge : -lunge;
    DrawCylinder((Vector3){pos.x, pos.y + 0.01f, pos.z}, 0.35f * s, 0.35f * s, 0.02f * s, 16, (Color){0, 0, 0, 50});
    // Charge energy ring
    if (chargeRingAlpha > 0.01f)
    {
        Color ringCol = {c.r, c.g, c.b, (unsigned char)(chargeRingAlpha * 255)};
        if (moveType == TYPE_FIRE)
            ringCol = (Color){255, 160, 30, (unsigned char)(chargeRingAlpha * 255)};
        else if (moveType == TYPE_WATER)
            ringCol = (Color){60, 180, 255, (unsigned char)(chargeRingAlpha * 255)};
        else if (moveType == TYPE_GRASS)
            ringCol = (Color){50, 220, 60, (unsigned char)(chargeRingAlpha * 255)};
        float ry = pos.y + bob;
        DrawCylinderWires((Vector3){pos.x, ry + 0.3f * s, pos.z}, chargeRingR, chargeRingR * 0.7f, 0.03f * s, 12, ringCol);
        DrawCylinderWires((Vector3){pos.x, ry + 0.3f * s, pos.z}, chargeRingR * 0.6f, chargeRingR * 0.4f, 0.03f * s, 8, ringCol);
        float ringPulse = 1 + sinf(animTime * 15) * 0.15f;
        DrawCylinderWires((Vector3){pos.x, ry + 0.3f * s, pos.z}, chargeRingR * ringPulse, chargeRingR * ringPulse * 0.7f, 0.02f * s, 16, (Color){ringCol.r, ringCol.g, ringCol.b, (unsigned char)(chargeRingAlpha * 160)});
    }

    // If this species has a GLB model, MY_pick the correct pre-rotated version
    if (sMY_pId >= 0 && sMY_pId < 8 && g_petModelLoaded[sMY_pId])
    {
        float ms = s * g_petModelScale[sMY_pId];
        Vector3 drawPos = {pos.x, pos.y + bob + 1.0f * ms, pos.z + lz};
        // Enemy uses default facing, player pet uses 180° pre-rotated copy
        Model *pm = isEnemy ? &g_petModels[sMY_pId] : &g_petModelsRot[sMY_pId];
        DrawModel(*pm, drawPos, ms, WHITE);
    }
    else
        switch (sMY_pId)
        {
        case 0:
        {
            // Fallback: procedural 火花
            DrawSphere((Vector3){pos.x, by + 0.3f * s * squash, pos.z - 0.05f * s + lz}, 0.35f * s, c);
            DrawSphere((Vector3){pos.x, by + 0.55f * s, pos.z + 0.02f * s + lz}, 0.28f * s, c);
            DrawSphere((Vector3){pos.x, by + 0.78f * s, pos.z + 0.1f * s + lz}, 0.22f * s, c);
            DrawSphere((Vector3){pos.x, by + 0.72f * s, pos.z + 0.25f * s + lz}, 0.09f * s, lt);
            PkmnEye((Vector3){pos.x - 0.07f * s, by + 0.82f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, 1);
            PkmnEye((Vector3){pos.x + 0.07f * s, by + 0.82f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, -1);
            DrawCylinder((Vector3){pos.x - 0.07f * s, by + 0.95f * s, pos.z + 0.05f * s + lz}, 0.02f * s, 0.05f * s, 0.16f * s, 6, (Color){255, 200, 50, 255});
            DrawCylinder((Vector3){pos.x + 0.07f * s, by + 0.95f * s, pos.z + 0.05f * s + lz}, 0.02f * s, 0.05f * s, 0.16f * s, 6, (Color){255, 200, 50, 255});
            DrawCylinder((Vector3){pos.x - 0.25f * s, by + 0.5f * s, pos.z + 0.05f * s + lz}, 0.05f * s, 0.04f * s, 0.35f * s, 8, c);
            DrawSphere((Vector3){pos.x - 0.25f * s, by + 0.3f * s, pos.z + 0.05f * s + lz}, 0.06f * s, lt);
            DrawCylinder((Vector3){pos.x + 0.25f * s, by + 0.5f * s, pos.z + 0.05f * s + lz}, 0.05f * s, 0.04f * s, 0.35f * s, 8, c);
            DrawSphere((Vector3){pos.x + 0.25f * s, by + 0.3f * s, pos.z + 0.05f * s + lz}, 0.06f * s, lt);
            DrawCylinder((Vector3){pos.x - 0.12f * s, by + 0.05f * s, pos.z + 0.05f * s + lz}, 0.1f * s, 0.07f * s, 0.08f * s, 8, dk);
            DrawCylinder((Vector3){pos.x + 0.12f * s, by + 0.05f * s, pos.z + 0.05f * s + lz}, 0.1f * s, 0.07f * s, 0.08f * s, 8, dk);
            float tw = sinf(animTime * 6) * 0.05f * s;
            DrawCylinder((Vector3){pos.x + tw, by + 0.2f * s, pos.z - 0.28f * s + lz}, 0.06f * s, 0.03f * s, 0.35f * s, 6, c);
            DrawSphere((Vector3){pos.x + tw * 1.5f, by + 0.08f * s, pos.z - 0.5f * s + lz}, 0.08f * s, (Color){255, 150, 20, 255});
            float fk = 1 + sinf(animTime * 10) * 0.12f;
            DrawSphere((Vector3){pos.x - 0.2f * s, by + 0.55f * s, pos.z + lz}, 0.1f * s * fk, (Color){255, 180, 30, 180});
            DrawSphere((Vector3){pos.x + 0.2f * s, by + 0.55f * s, pos.z + lz}, 0.1f * s * fk, (Color){255, 180, 30, 180});
            DrawSphere((Vector3){pos.x - 0.22f * s, by + 0.7f * s, pos.z - 0.05f * s + lz}, 0.07f * s * fk, (Color){255, 200, 50, 150});
            DrawSphere((Vector3){pos.x + 0.22f * s, by + 0.7f * s, pos.z - 0.05f * s + lz}, 0.07f * s * fk, (Color){255, 200, 50, 150});
            break;
        }
        case 6:
        {                                                                                               // 小包子(dumpling) — procedural
            DrawSphere((Vector3){pos.x, by + 0.3f * s * squash, pos.z - 0.05f * s + lz}, 0.35f * s, c); // belly
            DrawSphere((Vector3){pos.x, by + 0.55f * s, pos.z + 0.02f * s + lz}, 0.28f * s, c);         // chest
            DrawSphere((Vector3){pos.x, by + 0.78f * s, pos.z + 0.1f * s + lz}, 0.22f * s, c);          // head
            DrawSphere((Vector3){pos.x, by + 0.72f * s, pos.z + 0.25f * s + lz}, 0.09f * s, lt);
            PkmnEye((Vector3){pos.x - 0.07f * s, by + 0.82f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, 1);
            PkmnEye((Vector3){pos.x + 0.07f * s, by + 0.82f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, -1);
            DrawCylinder((Vector3){pos.x - 0.07f * s, by + 0.95f * s, pos.z + 0.05f * s + lz}, 0.02f * s, 0.05f * s, 0.16f * s, 6, (Color){255, 200, 50, 255});
            DrawCylinder((Vector3){pos.x + 0.07f * s, by + 0.95f * s, pos.z + 0.05f * s + lz}, 0.02f * s, 0.05f * s, 0.16f * s, 6, (Color){255, 200, 50, 255});
            DrawCylinder((Vector3){pos.x - 0.25f * s, by + 0.5f * s, pos.z + 0.05f * s + lz}, 0.05f * s, 0.04f * s, 0.35f * s, 8, c);
            DrawSphere((Vector3){pos.x - 0.25f * s, by + 0.3f * s, pos.z + 0.05f * s + lz}, 0.06f * s, lt);
            DrawCylinder((Vector3){pos.x + 0.25f * s, by + 0.5f * s, pos.z + 0.05f * s + lz}, 0.05f * s, 0.04f * s, 0.35f * s, 8, c);
            DrawSphere((Vector3){pos.x + 0.25f * s, by + 0.3f * s, pos.z + 0.05f * s + lz}, 0.06f * s, lt);
            DrawCylinder((Vector3){pos.x - 0.12f * s, by + 0.05f * s, pos.z + 0.05f * s + lz}, 0.1f * s, 0.07f * s, 0.08f * s, 8, dk);
            DrawCylinder((Vector3){pos.x + 0.12f * s, by + 0.05f * s, pos.z + 0.05f * s + lz}, 0.1f * s, 0.07f * s, 0.08f * s, 8, dk);
            float tw = sinf(animTime * 6) * 0.05f * s;
            DrawCylinder((Vector3){pos.x + tw, by + 0.2f * s, pos.z - 0.28f * s + lz}, 0.06f * s, 0.03f * s, 0.35f * s, 6, c);
            DrawSphere((Vector3){pos.x + tw * 1.5f, by + 0.08f * s, pos.z - 0.5f * s + lz}, 0.08f * s, (Color){255, 150, 20, 255});
            // Dumpling wrapper
            DrawCylinder((Vector3){pos.x, by + 0.35f * s, pos.z + lz}, 0.38f * s, 0.38f * s, 0.04f * s, 16, (Color){255, 240, 200, 200});
            break;
        }
        case 1:
        case 5:
        {                                                                                       // 水蓝蓝 / 雪精灵 (aquatic)
            DrawSphere((Vector3){pos.x, by + 0.28f * s * squash, pos.z + lz}, 0.36f * s, c);    // belly
            DrawSphere((Vector3){pos.x, by + 0.52f * s, pos.z + 0.05f * s + lz}, 0.28f * s, c); // chest
            DrawSphere((Vector3){pos.x, by + 0.72f * s, pos.z + 0.1f * s + lz}, 0.2f * s, c);   // head
            // Big pokemon eyes
            PkmnEye((Vector3){pos.x - 0.09f * s, by + 0.77f * s, pos.z + 0.18f * s + lz}, 0.08f * s, BLACK, 1);
            PkmnEye((Vector3){pos.x + 0.09f * s, by + 0.77f * s, pos.z + 0.18f * s + lz}, 0.08f * s, BLACK, -1);
            // Mouth
            DrawSphere((Vector3){pos.x, by + 0.67f * s, pos.z + 0.22f * s + lz}, 0.04f * s, dk);
            // Dorsal fin
            float fw = sinf(animTime * 5) * 0.04f * s;
            DrawCylinder((Vector3){pos.x + fw, by + 0.88f * s, pos.z - 0.02f * s + lz}, 0.02f * s, 0.07f * s, 0.2f * s, 6, lt);
            // Side flippers (no legs)
            DrawSphere((Vector3){pos.x - 0.32f * s, by + 0.35f * s, pos.z + lz}, 0.08f * s, lt);
            DrawSphere((Vector3){pos.x + 0.32f * s, by + 0.35f * s, pos.z + lz}, 0.08f * s, lt);
            // Tail fin
            DrawCylinder((Vector3){pos.x, by + 0.2f * s, pos.z - 0.3f * s + lz}, 0.06f * s, 0.02f * s, 0.18f * s, 6, lt);
            // Bubbles / snowflakes
            float b1 = fmodf(animTime * 1.5f, 1), b2 = fmodf(animTime * 1.5f + 0.5f, 1);
            if (sMY_pId == 1)
            { // bubbles for 水蓝蓝
                DrawSphere((Vector3){pos.x - 0.13f * s, by + 0.5f * s + b1 * 0.5f, pos.z + 0.15f * s + lz}, 0.035f * s * (1 - b1), (Color){200, 230, 255, (unsigned char)(150 * (1 - b1))});
                DrawSphere((Vector3){pos.x + 0.17f * s, by + 0.4f * s + b2 * 0.6f, pos.z + 0.12f * s + lz}, 0.025f * s * (1 - b2), (Color){200, 230, 255, (unsigned char)(120 * (1 - b2))});
            }
            else
            { // snowflakes for 雪精灵
                DrawSphere((Vector3){pos.x - 0.15f * s, by + 0.55f * s + b1 * 0.4f, pos.z + 0.1f * s + lz}, 0.03f * s * (1 - b1), (Color){220, 240, 255, (unsigned char)(160 * (1 - b1))});
                DrawSphere((Vector3){pos.x + 0.18f * s, by + 0.45f * s + b2 * 0.5f, pos.z + 0.08f * s + lz}, 0.025f * s * (1 - b2), (Color){220, 240, 255, (unsigned char)(130 * (1 - b2))});
            }
            // Accessory: pearl necklace (水蓝蓝) / ice crown (雪精灵)
            if (sMY_pId == 1)
                for (int n = 0; n < 5; n++)
                {
                    float na = (float)n / 5 * MY_PI * 2;
                    DrawSphere((Vector3){pos.x + cosf(na) * 0.3f * s, by + 0.48f * s, pos.z + sinf(na) * 0.3f * s + lz}, 0.04f * s, WHITE);
                }
            else
                for (int n = 0; n < 3; n++)
                {
                    DrawCylinder((Vector3){pos.x + (float)(n - 1) * 0.1f * s, by + 0.88f * s, pos.z - 0.05f * s + lz}, 0.02f * s, 0.03f * s, 0.12f * s, 6, (Color){200, 230, 255, 255});
                }
            break;
        }
        case 2:
        case 4:
        {                                                                                      // 喵喵(cat) / 格兰球(ball plant)
            DrawSphere((Vector3){pos.x, by + 0.25f * s * squash, pos.z + lz}, 0.32f * s, c);   // belly
            DrawSphere((Vector3){pos.x, by + 0.5f * s, pos.z + 0.03f * s + lz}, 0.25f * s, c); // chest
            DrawSphere((Vector3){pos.x, by + 0.7f * s, pos.z + 0.08f * s + lz}, 0.2f * s, c);  // head
            PkmnEye((Vector3){pos.x - 0.07f * s, by + 0.74f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, 1);
            PkmnEye((Vector3){pos.x + 0.07f * s, by + 0.74f * s, pos.z + 0.16f * s + lz}, 0.06f * s, BLACK, -1);
            // Leaf crest
            float sw = sinf(animTime * 4) * 0.04f * s;
            DrawCylinder((Vector3){pos.x + sw, by + 0.86f * s, pos.z + 0.02f * s + lz}, 0.03f * s, 0.08f * s, 0.22f * s, 6, (Color){30, 170, 40, 255});
            DrawSphere((Vector3){pos.x + sw * 1.5f + 0.03f * s, by + 1.02f * s, pos.z + 0.02f * s + lz}, 0.06f * s, (Color){50, 190, 55, 255});
            // Quadruped legs (4 short)
            for (int lx = -1; lx <= 1; lx += 2)
                for (int lz2 = -1; lz2 <= 1; lz2 += 2)
                    DrawCylinder((Vector3){pos.x + lx * 0.12f * s, by + 0.04f * s, pos.z + lz2 * 0.1f * s + lz}, 0.07f * s, 0.05f * s, 0.09f * s, 6, dk);
            // Tail
            float vw = sinf(animTime * 5) * 0.04f * s;
            DrawCylinder((Vector3){pos.x + vw, by + 0.18f * s, pos.z - 0.24f * s + lz}, 0.03f * s, 0.02f * s, 0.22f * s, 5, (Color){28, 160, 38, 255});
            // Accessory: flower (喵喵) / seed shell (格兰球)
            if (sMY_pId == 2)
                for (int p = 0; p < 5; p++)
                {
                    float pa = (float)p / 5 * MY_PI * 2;
                    DrawSphere((Vector3){pos.x + cosf(pa) * 0.09f * s, by + 0.82f * s, pos.z + 0.16f * s + sinf(pa) * 0.09f * s + lz}, 0.04f * s, (Color){255, 200, 220, 255});
                }
            else
                DrawCylinder((Vector3){pos.x, by + 0.3f * s, pos.z + lz}, 0.34f * s, 0.28f * s, 0.06f * s, 16, (Color){120, 80, 40, 180});
            break;
        }
        case 3:
        case 7:
        default:
        {                                                                                       // 瞌睡王(sleepy) / 幽灵(ghost)
            DrawSphere((Vector3){pos.x, by + 0.3f * s * squash, pos.z + lz}, 0.34f * s, c);     // belly
            DrawSphere((Vector3){pos.x, by + 0.54f * s, pos.z + 0.04f * s + lz}, 0.26f * s, c); // chest
            DrawSphere((Vector3){pos.x, by + 0.74f * s, pos.z + 0.1f * s + lz}, 0.2f * s, c);   // head
            // Droopy / sleepy eyes
            DrawSphere((Vector3){pos.x - 0.08f * s, by + 0.78f * s, pos.z + 0.16f * s + lz}, 0.06f * s, WHITE);
            DrawSphere((Vector3){pos.x + 0.08f * s, by + 0.78f * s, pos.z + 0.16f * s + lz}, 0.06f * s, WHITE);
            // Half-closed eyelids
            DrawCylinder((Vector3){pos.x - 0.08f * s, by + 0.8f * s, pos.z + 0.2f * s + lz}, 0.07f * s, 0.07f * s, 0.02f * s, 8, c);
            DrawCylinder((Vector3){pos.x + 0.08f * s, by + 0.8f * s, pos.z + 0.2f * s + lz}, 0.07f * s, 0.07f * s, 0.02f * s, 8, c);
            DrawSphere((Vector3){pos.x - 0.06f * s, by + 0.78f * s, pos.z + 0.2f * s + lz}, 0.03f * s, BLACK); // puMY_pil
            DrawSphere((Vector3){pos.x + 0.1f * s, by + 0.78f * s, pos.z + 0.2f * s + lz}, 0.03f * s, BLACK);
            // Nose + mouth
            DrawSphere((Vector3){pos.x, by + 0.7f * s, pos.z + 0.2f * s + lz}, 0.035f * s, dk);
            DrawSphere((Vector3){pos.x, by + 0.66f * s, pos.z + 0.2f * s + lz}, 0.025f * s, dk);
            // Ears
            float ew = sinf(animTime * 7) * 0.02f * s;
            DrawCylinder((Vector3){pos.x - 0.14f * s + ew, by + 0.88f * s, pos.z + 0.02f * s + lz}, 0.05f * s, 0.07f * s, 0.14f * s, 6, c);
            DrawCylinder((Vector3){pos.x + 0.14f * s - ew, by + 0.88f * s, pos.z + 0.02f * s + lz}, 0.05f * s, 0.07f * s, 0.14f * s, 6, c);
            DrawSphere((Vector3){pos.x - 0.14f * s, by + 0.95f * s, pos.z + 0.04f * s + lz}, 0.035f * s, lt);
            DrawSphere((Vector3){pos.x + 0.14f * s, by + 0.95f * s, pos.z + 0.04f * s + lz}, 0.035f * s, lt);
            // Short legs
            DrawCylinder((Vector3){pos.x - 0.1f * s, by + 0.04f * s, pos.z + 0.05f * s + lz}, 0.09f * s, 0.07f * s, 0.07f * s, 8, dk);
            DrawCylinder((Vector3){pos.x + 0.1f * s, by + 0.04f * s, pos.z + 0.05f * s + lz}, 0.09f * s, 0.07f * s, 0.07f * s, 8, dk);
            // Tail
            float tw2 = sinf(animTime * 6) * 0.07f * s;
            DrawSphere((Vector3){pos.x + tw2, by + 0.18f * s, pos.z - 0.3f * s + lz}, 0.09f * s, c);
            // Accessory: sleeMY_ping cap (瞌睡王) / ghostly wisps (幽灵)
            if (sMY_pId == 3)
            {
                DrawCylinder((Vector3){pos.x, by + 0.92f * s, pos.z + 0.05f * s + lz}, 0.05f * s, 0.15f * s, 0.25f * s, 8, (Color){200, 180, 220, 255});
                DrawSphere((Vector3){pos.x, by + 1.15f * s, pos.z + 0.05f * s + lz}, 0.07f * s, (Color){255, 240, 255, 255});
            }
            else
            {
                float gw = sinf(animTime * 3) * 0.1f * s;
                DrawCylinder((Vector3){pos.x + gw, by + 0.88f * s, pos.z + lz}, 0.02f * s, 0.05f * s, 0.2f * s, 4, (Color){c.r, c.g, c.b, 120});
                DrawCylinder((Vector3){pos.x - gw, by + 0.85f * s, pos.z + lz}, 0.02f * s, 0.04f * s, 0.18f * s, 4, (Color){c.r, c.g, c.b, 100});
            }
            break;
        }
        }

    if (isEnemy)
    {
        float ay = by + 1.3f * s;
        DrawCylinder((Vector3){pos.x, ay, pos.z + lz}, 0.03f * s, 0.03f * s, 0.28f * s, 4, (Color){255, 50, 50, 200});
        DrawSphere((Vector3){pos.x, ay + 0.28f * s, pos.z + lz}, 0.05f * s, (Color){255, 50, 50, 200});
    }
    // Hit flash for receiving damage (impact phase, non-enemy = player's pet)
    if (isAttacking && !isEnemy && animDuration > 0)
    {
        float ap = fmodf(animTime, animDur) / animDur;
        if (ap >= 0.53f && ap < 0.65f)
        {
            float flash = 1.0f - (ap - 0.53f) / 0.12f;
            ApplyHitFlash(pos, s, flash);
        }
    }
}

// ==================== BATTLE ARENA ====================

void DrawBattleArena(Vector3 center, float radius)
{
    DrawCylinder(center, radius, radius, 0.15f, 32, (Color){35, 40, 55, 255});
    DrawCylinderWires(center, radius, radius, 0.16f, 32, (Color){80, 180, 255, 200});
    float ir = radius * 0.7f;
    DrawCylinder((Vector3){center.x, center.y + 0.15f, center.z}, ir, ir, 0.02f, 32, (Color){55, 60, 75, 255});
    DrawCylinderWires((Vector3){center.x, center.y + 0.17f, center.z}, ir, ir, 0.01f, 32, (Color){100, 200, 255, 120});
    DrawCylinder((Vector3){center.x, center.y + 0.17f, center.z}, 0.03f, 0.03f, 0.01f, 8, (Color){255, 255, 255, 150});
    for (int i = 0; i < 4; i++)
    {
        float a = (float)i * MY_PI / 2 + MY_PI / 4;
        float px = center.x + cosf(a) * radius * 0.85f, pz = center.z + sinf(a) * radius * 0.85f;
        DrawCylinder((Vector3){px, center.y + 0.15f, pz}, 0.07f, 0.07f, 0.55f, 8, (Color){55, 60, 80, 255});
        DrawSphere((Vector3){px, center.y + 0.7f, pz}, 0.09f, (Color){80, 200, 255, 200});
    }
    float t = (float)GetTime() * 0.5f;
    for (int i = 0; i < 12; i++)
    {
        float a = t + (float)i * MY_PI * 2 / 12, pr = radius * 0.72f;
        DrawSphere((Vector3){center.x + cosf(a) * pr, center.y + 0.45f + sinf(t * 2 + i) * 0.08f, center.z + sinf(a) * pr}, 0.025f, (Color){100, 200, 255, 90});
    }
}
