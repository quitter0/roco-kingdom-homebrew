#include "save.h"
#include <stdio.h>
#include <string.h>

bool SaveFileExists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

bool SaveGame(const char *path, const SaveData *data) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    int version = SAVE_VERSION;
    fwrite(&version, sizeof(int), 1, f);
    fwrite(&data->playerPosition, sizeof(Vector3), 1, f);
    fwrite(&data->teamSize, sizeof(int), 1, f);
    fwrite(&data->activePetIndex, sizeof(int), 1, f);
    fwrite(&data->inventory, sizeof(Inventory), 1, f);
    fwrite(&data->playTime, sizeof(int), 1, f);

    for (int i = 0; i < data->teamSize; i++) {
        fwrite(&data->team[i], sizeof(Pet), 1, f);
    }

    fwrite(data->npcDefeated, sizeof(bool), MAX_SAVE_NPCS, f);

    // Defeated trainer hashes (version 3+)
    fwrite(&data->defeatedTrainerCount, sizeof(int), 1, f);
    fwrite(data->defeatedTrainerHashes, sizeof(unsigned int), data->defeatedTrainerCount, f);

    fclose(f);
    return true;
}

bool LoadGame(const char *path, SaveData *data) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    memset(data, 0, sizeof(SaveData));

    int version = 0;
    size_t read = fread(&version, sizeof(int), 1, f);
    if (read != 1) { fclose(f); return false; }

    fread(&data->playerPosition, sizeof(Vector3), 1, f);
    fread(&data->teamSize, sizeof(int), 1, f);
    fread(&data->activePetIndex, sizeof(int), 1, f);
    if (version >= 6) {
        fread(&data->inventory, sizeof(Inventory), 1, f);
    } else {
        // Old format: 6 ints (potions, superPotions, petBalls, greatBalls, revives, gold)
        fread(&data->inventory.potions, sizeof(int), 1, f);
        fread(&data->inventory.superPotions, sizeof(int), 1, f);
        data->inventory.ethers = 0;
        fread(&data->inventory.petBalls, sizeof(int), 1, f);
        fread(&data->inventory.greatBalls, sizeof(int), 1, f);
        fread(&data->inventory.revives, sizeof(int), 1, f);
        fread(&data->inventory.gold, sizeof(int), 1, f);
    }
    fread(&data->playTime, sizeof(int), 1, f);

    for (int i = 0; i < data->teamSize; i++) {
        fread(&data->team[i], sizeof(Pet), 1, f);
    }

    // NPC state (version >= 2)
    if (version >= 2) {
        fread(data->npcDefeated, sizeof(bool), MAX_SAVE_NPCS, f);
    }

    // Defeated trainer hashes (version >= 3)
    if (version >= 3) {
        fread(&data->defeatedTrainerCount, sizeof(int), 1, f);
        if (data->defeatedTrainerCount > MAX_DEFEATED_TRAINERS)
            data->defeatedTrainerCount = MAX_DEFEATED_TRAINERS;
        fread(data->defeatedTrainerHashes, sizeof(unsigned int), data->defeatedTrainerCount, f);
    }

    fclose(f);
    return true;
}
