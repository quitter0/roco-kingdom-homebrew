#ifndef TERRAIN_H
#define TERRAIN_H

#include "raylib.h"

typedef struct Terrain {
    Model model;
    Image image;
    Vector3 size;
    Vector3 position;
    int meshRes;
    float meshSpan;
    float *heightmap;   // normalized heights (meshRes × meshRes), bilinearly sampled
    float centerX;      // world center X of the mesh
    float centerZ;      // world center Z of the mesh
} Terrain;

Terrain LoadTerrain(void);
void RecenterTerrain(Terrain *t, float newCenterX, float newCenterZ);
bool TerrainNeedsRecenter(Terrain *t, float playerX, float playerZ);
float GetTerrainHeight(Terrain *terrain, float x, float z);
void GetTerrainType(Terrain *t, float x, float z, float *outWater, float *outGrass, float *outRock);
void DrawTerrain(Terrain *terrain);
void UnloadTerrain(Terrain *terrain);

// Deterministic hash function
unsigned int HashInt(int x, int z);

// Procedural placement — requires terrain for type checks
bool ShouldPlaceTree(Terrain *t, int cellX, int cellZ, float *outOffX, float *outOffZ);
bool ShouldPlaceRock(Terrain *t, int cellX, int cellZ, float *outOffX, float *outOffZ);

// Collision queries
bool GetTreeCollision(Terrain *t, float wx, float wz, Vector3 *outPos, float *outRadius);
bool GetRockCollision(Terrain *t, float wx, float wz, Vector3 *outPos, float *outRadius);

// Procedural trainer generation
bool ShouldPlaceTrainer(int cellX, int cellZ, int *outSpeciesId, int *outLevel,
                        int *outNameIdx, int *outColorIdx);
unsigned int GetTrainerHash(int cellX, int cellZ);

#endif
