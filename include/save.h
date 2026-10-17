#ifndef SAVE_H
#define SAVE_H

#include "raylib.h"
#include "pet.h"
#include "inventory.h"
#include <stdbool.h>

#define SAVE_VERSION 6   // +ethers field in Inventory
#define MAX_SAVE_NPCS 16
#define MAX_DEFEATED_TRAINERS 64

typedef struct {
    Vector3 playerPosition;
    Pet team[MAX_TEAM_SIZE];
    int teamSize;
    int activePetIndex;
    Inventory inventory;
    int playTime;
    bool npcDefeated[MAX_SAVE_NPCS];
    unsigned int defeatedTrainerHashes[MAX_DEFEATED_TRAINERS];
    int defeatedTrainerCount;
} SaveData;

bool SaveFileExists(const char *path);
bool SaveGame(const char *path, const SaveData *data);
bool LoadGame(const char *path, SaveData *data);

#endif
