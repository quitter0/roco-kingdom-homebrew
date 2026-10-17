// Standalone tool: generates .glb models for all game objects
// Build: gcc -o gen_models generate_models.c -lraylib -lm
// Run:   ./gen_models    (outputs to current dir, then copy to resources/models/)

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Combine multiple meshes into one (allocates new vertex arrays)
static Mesh CombineMeshes(Mesh *parts, int count) {
    int totalVerts = 0, totalIdx = 0;
    for (int i = 0; i < count; i++) {
        totalVerts += parts[i].vertexCount;
        totalIdx  += parts[i].triangleCount * 3;
    }

    Mesh out = {0};
    out.vertexCount   = totalVerts;
    out.triangleCount = totalIdx / 3;
    out.vertices  = (float *)malloc(totalVerts * 3 * sizeof(float));
    out.normals   = (float *)malloc(totalVerts * 3 * sizeof(float));
    out.texcoords = (float *)malloc(totalVerts * 2 * sizeof(float));
    out.indices   = (unsigned short *)malloc(totalIdx * sizeof(unsigned short));

    int voff = 0, ioff = 0, baseVert = 0;
    for (int i = 0; i < count; i++) {
        Mesh *m = &parts[i];
        int vc = m->vertexCount, ic = m->triangleCount * 3;
        // Vertices
        memcpy(out.vertices  + voff * 3, m->vertices,  vc * 3 * sizeof(float));
        // Normals  (generate if missing)
        if (m->normals) {
            memcpy(out.normals + voff * 3, m->normals,   vc * 3 * sizeof(float));
        } else {
            for (int j = 0; j < vc * 3; j++) out.normals[voff * 3 + j] = 0;
        }
        // Texcoords (generate if missing)
        if (m->texcoords) {
            memcpy(out.texcoords + voff * 2, m->texcoords, vc * 2 * sizeof(float));
        } else {
            for (int j = 0; j < vc * 2; j++) out.texcoords[voff * 2 + j] = 0;
        }
        // Indices (offset by base vertex)
        if (m->indices) {
            for (int j = 0; j < ic; j++)
                out.indices[ioff + j] = m->indices[j] + (unsigned short)baseVert;
        }
        voff += vc;
        ioff += ic;
        baseVert += vc;
    }
    return out;
}

static void FreeMeshParts(Mesh *parts, int count) {
    for (int i = 0; i < count; i++) {
        free(parts[i].vertices);
        free(parts[i].normals);
        free(parts[i].texcoords);
        free(parts[i].indices);
        free(parts[i].colors);
    }
}

static void Export(const char *name, Mesh mesh) {
    ExportMesh(mesh, name);
}

// --- Pet generator helpers ---

static Mesh PetBody(float radius, float squish) {
    Mesh m = GenMeshSphere(radius, 16, 12);
    // Squish Y axis
    for (int i = 0; i < m.vertexCount; i++)
        m.vertices[i * 3 + 1] *= squish;
    return m;
}

static Mesh PetHead(float radius, float yOffset) {
    Mesh m = GenMeshSphere(radius, 12, 10);
    for (int i = 0; i < m.vertexCount; i++)
        m.vertices[i * 3 + 1] += yOffset;
    return m;
}

static Mesh PetEar(float x, float y, float z, float r, float h) {
    Mesh m = GenMeshCone(r, h, 8);
    for (int i = 0; i < m.vertexCount; i++) {
        m.vertices[i * 3 + 0] += x;
        m.vertices[i * 3 + 1] += y;
        m.vertices[i * 3 + 2] += z;
    }
    return m;
}

static Mesh PetFoot(float x, float z, float r, float h) {
    Mesh m = GenMeshCylinder(r, h, 8);
    for (int i = 0; i < m.vertexCount; i++) {
        m.vertices[i * 3 + 0] += x;
        m.vertices[i * 3 + 2] += z;
        m.vertices[i * 3 + 1] += h * 0.5f;
    }
    return m;
}

static Mesh PetEye(float x, float y, float z, float r) {
    Mesh m = GenMeshSphere(r, 6, 4);
    for (int i = 0; i < m.vertexCount; i++) {
        m.vertices[i * 3 + 0] += x;
        m.vertices[i * 3 + 1] += y;
        m.vertices[i * 3 + 2] += z;
    }
    return m;
}

static Mesh PetTail(float x, float y, float z, float r) {
    return PetEye(x, y, z, r); // same as sphere placement
}

static Mesh PetHorn(float x, float y, float z, float r, float h) {
    return PetEar(x, y, z, r, h); // cone
}

// Fire pets: dragon-like (pet_0 火花, pet_6 小包子)
static void GenFirePet(int idx) {
    Mesh parts[12];
    int n = 0;
    parts[n++] = PetBody(0.35f, 0.75f);       // squat body
    parts[n++] = PetHead(0.24f, 0.45f);        // head on top
    parts[n++] = PetEye(-0.08f, 0.55f, 0.18f, 0.06f);  // eyes
    parts[n++] = PetEye(0.08f, 0.55f, 0.18f, 0.06f);
    parts[n++] = PetHorn(-0.08f, 0.65f, 0.05f, 0.04f, 0.18f); // horns
    parts[n++] = PetHorn(0.08f, 0.65f, 0.05f, 0.04f, 0.18f);
    parts[n++] = PetFoot(-0.14f, 0.08f, 0.1f, 0.06f); // feet
    parts[n++] = PetFoot(0.14f, 0.08f, 0.1f, 0.06f);
    parts[n++] = PetTail(0.0f, 0.15f, -0.3f, 0.09f);  // tail
    char name[64];
    snprintf(name, sizeof(name), "resources/models/pet_%d.glb", idx);
    Mesh combined = CombineMeshes(parts, n);
    Export(name, combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Water pets: round aquatic (pet_1 水蓝蓝, pet_5 雪精灵)
static void GenWaterPet(int idx) {
    Mesh parts[12]; int n = 0;
    parts[n++] = PetBody(0.38f, 0.9f);
    parts[n++] = PetHead(0.2f, 0.5f);
    parts[n++] = PetEye(-0.09f, 0.58f, 0.15f, 0.08f);
    parts[n++] = PetEye(0.09f, 0.58f, 0.15f, 0.08f);
    // Fin on top
    parts[n++] = PetEar(0.0f, 0.7f, -0.02f, 0.06f, 0.22f);
    // Side fins
    parts[n++] = PetEar(-0.3f, 0.35f, 0.0f, 0.06f, 0.1f);
    parts[n++] = PetEar(0.3f, 0.35f, 0.0f, 0.06f, 0.1f);
    char name[64];
    snprintf(name, sizeof(name), "resources/models/pet_%d.glb", idx);
    Mesh combined = CombineMeshes(parts, n);
    Export(name, combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Grass pets: plant creature (pet_2 喵喵, pet_4 格兰球)
static void GenGrassPet(int idx) {
    Mesh parts[12]; int n = 0;
    parts[n++] = PetBody(0.32f, 0.85f);
    parts[n++] = PetHead(0.22f, 0.42f);
    parts[n++] = PetEye(-0.07f, 0.5f, 0.16f, 0.06f);
    parts[n++] = PetEye(0.07f, 0.5f, 0.16f, 0.06f);
    // Leaf on head
    parts[n++] = PetEar(0.0f, 0.62f, 0.0f, 0.05f, 0.28f);
    // Leaf collar
    parts[n++] = PetEar(-0.16f, 0.32f, 0.08f, 0.04f, 0.16f);
    parts[n++] = PetEar(0.16f, 0.32f, 0.08f, 0.04f, 0.16f);
    parts[n++] = PetFoot(-0.1f, 0.05f, 0.08f, 0.05f);
    parts[n++] = PetFoot(0.1f, 0.05f, 0.08f, 0.05f);
    parts[n++] = PetTail(0.0f, 0.15f, -0.28f, 0.06f);
    char name[64];
    snprintf(name, sizeof(name), "resources/models/pet_%d.glb", idx);
    Mesh combined = CombineMeshes(parts, n);
    Export(name, combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Normal pets: cute round creature with ears (pet_3 瞌睡王, pet_7 幽灵)
static void GenNormalPet(int idx) {
    Mesh parts[12]; int n = 0;
    parts[n++] = PetBody(0.35f, 0.8f);
    parts[n++] = PetHead(0.23f, 0.48f);
    // Ears
    parts[n++] = PetEar(-0.14f, 0.65f, 0.0f, 0.06f, 0.16f);
    parts[n++] = PetEar(0.14f, 0.65f, 0.0f, 0.06f, 0.16f);
    parts[n++] = PetEye(-0.08f, 0.55f, 0.18f, 0.07f);
    parts[n++] = PetEye(0.08f, 0.55f, 0.18f, 0.07f);
    // Nose
    parts[n++] = PetEye(0.0f, 0.48f, 0.2f, 0.04f);
    parts[n++] = PetFoot(-0.1f, 0.08f, 0.1f, 0.06f);
    parts[n++] = PetFoot(0.1f, 0.08f, 0.1f, 0.06f);
    parts[n++] = PetTail(0.0f, 0.15f, -0.32f, 0.1f);
    char name[64];
    snprintf(name, sizeof(name), "resources/models/pet_%d.glb", idx);
    Mesh combined = CombineMeshes(parts, n);
    Export(name, combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Player model
static void GenPlayer(void) {
    Mesh parts[16]; int n = 0;
    // Legs
    parts[n++] = PetFoot(-0.14f, 0.0f, 0.1f, 0.55f);
    parts[n++] = PetFoot(0.14f, 0.0f, 0.1f, 0.55f);
    // Torso
    Mesh torso = GenMeshCylinder(0.30f, 0.85f, 12);
    for (int i = 0; i < torso.vertexCount; i++) torso.vertices[i*3+1] += 0.7f;
    parts[n++] = torso;
    // Head
    parts[n++] = PetHead(0.32f, 1.6f);
    parts[n++] = PetEye(-0.1f, 1.68f, 0.28f, 0.06f);
    parts[n++] = PetEye(0.1f, 1.68f, 0.28f, 0.06f);
    // Hat
    parts[n++] = PetEar(0.0f, 1.98f, 0.0f, 0.2f, 0.22f);
    // Arms
    parts[n++] = PetFoot(-0.36f, 0.18f, 0.08f, 0.48f);
    parts[n++] = PetFoot(0.36f, 0.18f, 0.08f, 0.48f);
    Mesh combined = CombineMeshes(parts, n);
    Export("resources/models/player.glb", combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// NPC trainer
static void GenNPCTrainer(void) {
    Mesh parts[12]; int n = 0;
    parts[n++] = PetFoot(-0.12f, 0.0f, 0.08f, 0.5f);
    parts[n++] = PetFoot(0.12f, 0.0f, 0.08f, 0.5f);
    Mesh torso = GenMeshCylinder(0.24f, 0.65f, 10);
    for (int i = 0; i < torso.vertexCount; i++) torso.vertices[i*3+1] += 0.55f;
    parts[n++] = torso;
    parts[n++] = PetHead(0.26f, 1.3f);
    parts[n++] = PetEye(-0.08f, 1.35f, 0.22f, 0.05f);
    parts[n++] = PetEye(0.08f, 1.35f, 0.22f, 0.05f);
    parts[n++] = PetEar(0.0f, 1.6f, 0.0f, 0.18f, 0.16f); // hat
    Mesh combined = CombineMeshes(parts, n);
    Export("resources/models/npc_trainer.glb", combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// NPC civilian
static void GenNPCCivil(void) {
    Mesh parts[10]; int n = 0;
    parts[n++] = PetFoot(-0.12f, 0.0f, 0.08f, 0.5f);
    parts[n++] = PetFoot(0.12f, 0.0f, 0.08f, 0.5f);
    Mesh torso = GenMeshCylinder(0.24f, 0.65f, 10);
    for (int i = 0; i < torso.vertexCount; i++) torso.vertices[i*3+1] += 0.55f;
    parts[n++] = torso;
    parts[n++] = PetHead(0.26f, 1.3f);
    parts[n++] = PetEye(-0.08f, 1.35f, 0.22f, 0.05f);
    parts[n++] = PetEye(0.08f, 1.35f, 0.22f, 0.05f);
    parts[n++] = PetHead(0.2f, 1.55f); // hair
    Mesh combined = CombineMeshes(parts, n);
    Export("resources/models/npc_civil.glb", combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Tree
static void GenTree(void) {
    Mesh parts[3]; int n = 0;
    Mesh trunk = GenMeshCylinder(0.2f, 3.0f, 8);
    for (int i = 0; i < trunk.vertexCount; i++) trunk.vertices[i*3+1] += 1.5f;
    parts[n++] = trunk;
    Mesh canopy = GenMeshSphere(1.4f, 10, 8);
    for (int i = 0; i < canopy.vertexCount; i++) canopy.vertices[i*3+1] += 3.2f;
    parts[n++] = canopy;
    Mesh combined = CombineMeshes(parts, n);
    Export("resources/models/tree.glb", combined);
    FreeMeshParts(parts, n);
    free(combined.vertices); free(combined.normals);
    free(combined.texcoords); free(combined.indices);
}

// Rock
static void GenRock(void) {
    Mesh m = GenMeshSphere(0.8f, 8, 6);
    // Deform to look irregular
    for (int i = 0; i < m.vertexCount; i++) {
        m.vertices[i*3+0] *= 1.0f + ((float)rand()/RAND_MAX - 0.5f) * 0.35f;
        m.vertices[i*3+1] *= 0.6f + ((float)rand()/RAND_MAX) * 0.3f;
        m.vertices[i*3+2] *= 1.0f + ((float)rand()/RAND_MAX - 0.5f) * 0.35f;
    }
    Export("resources/models/rock.glb", m);
    free(m.vertices); free(m.normals); free(m.texcoords); free(m.indices);
}

int main(void) {
    InitWindow(100, 100, "gen"); // minimal window, required by raylib
    SetTargetFPS(1);

    GenFirePet(0);    // 火花
    GenWaterPet(1);   // 水蓝蓝
    GenGrassPet(2);   // 喵喵
    GenNormalPet(3);  // 瞌睡王
    GenGrassPet(4);   // 格兰球
    GenWaterPet(5);   // 雪精灵
    GenFirePet(6);    // 小包子
    GenNormalPet(7);  // 幽灵

    GenPlayer();
    GenNPCTrainer();
    GenNPCCivil();
    GenTree();
    GenRock();

    CloseWindow();
    return 0;
}
