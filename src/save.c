#include "save.h"
#include <stdio.h>
#include <string.h>

static bool ReadExact(void *dst, size_t size, size_t count, FILE *f) {
    return fread(dst, size, count, f) == count;
}

static bool WriteExact(const void *src, size_t size, size_t count, FILE *f) {
    return fwrite(src, size, count, f) == count;
}

bool SaveFileExists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

bool SaveGame(const char *path, const SaveData *data) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    int maxTeam = (int)(sizeof(data->team) / sizeof(data->team[0]));
    int teamSize = data->teamSize;
    if (teamSize < 0) teamSize = 0;
    if (teamSize > maxTeam) teamSize = maxTeam;

    int trainerCount = data->defeatedTrainerCount;
    if (trainerCount < 0) trainerCount = 0;
    if (trainerCount > MAX_DEFEATED_TRAINERS) trainerCount = MAX_DEFEATED_TRAINERS;

    int version = SAVE_VERSION;
    bool ok = true;
    ok &= WriteExact(&version, sizeof(int), 1, f);
    ok &= WriteExact(&data->playerPosition, sizeof(Vector3), 1, f);
    ok &= WriteExact(&teamSize, sizeof(int), 1, f);
    ok &= WriteExact(&data->activePetIndex, sizeof(int), 1, f);
    ok &= WriteExact(&data->inventory, sizeof(Inventory), 1, f);
    ok &= WriteExact(&data->playTime, sizeof(int), 1, f);

    for (int i = 0; i < teamSize; i++) {
        ok &= WriteExact(&data->team[i], sizeof(Pet), 1, f);
    }

    ok &= WriteExact(data->npcDefeated, sizeof(bool), MAX_SAVE_NPCS, f);
    ok &= WriteExact(&trainerCount, sizeof(int), 1, f);
    ok &= WriteExact(data->defeatedTrainerHashes, sizeof(unsigned int), trainerCount, f);

    return (fclose(f) == 0) && ok;
}

static void SanitizeInventory(Inventory *inv) {
    if (inv->potions < 0) inv->potions = 0;
    if (inv->superPotions < 0) inv->superPotions = 0;
    if (inv->ethers < 0) inv->ethers = 0;
    if (inv->petBalls < 0) inv->petBalls = 0;
    if (inv->greatBalls < 0) inv->greatBalls = 0;
    if (inv->revives < 0) inv->revives = 0;
    if (inv->gold < 0) inv->gold = 0;
}

static bool SanitizePet(Pet *p) {
    if (p->speciesId < 0 || p->speciesId >= (int)PET_COUNT) return false;

    p->nickname[sizeof(p->nickname) - 1] = '\0';
    if (p->nickname[0] == '\0')
        snprintf(p->nickname, sizeof(p->nickname), "%s", PET_DATABASE[p->speciesId].name);

    if (p->level < 1) p->level = 1;
    if (p->level > 100) p->level = 100;
    if (p->maxHP < 1) p->maxHP = 1;
    if (p->currentHP < 0) p->currentHP = 0;
    if (p->currentHP > p->maxHP) p->currentHP = p->maxHP;

    if (p->maxSharedPP < 0) p->maxSharedPP = 0;
    if (p->sharedPP < 0) p->sharedPP = 0;
    if (p->sharedPP > p->maxSharedPP) p->sharedPP = p->maxSharedPP;

    if (p->status < STATUS_NONE || p->status > STATUS_FREEZE) p->status = STATUS_NONE;
    if (p->status == STATUS_FREEZE) {
        if (p->statusTimer < 0) p->statusTimer = 0;
        if (p->statusTimer > 3) p->statusTimer = 3;
    } else {
        p->statusTimer = 0;
    }

    if (p->currentExp < 0) p->currentExp = 0;
    if (p->expToNext < 1) p->expToNext = p->level * p->level * 30;

    for (int m = 0; m < 4; m++) {
        if (p->moveIds[m] < 0 || p->moveIds[m] >= (int)MOVE_COUNT) p->moveIds[m] = 0;
        if (p->skillPacks[m] < 0) p->skillPacks[m] = 0;
        if (p->skillPacks[m] > SKILLPACK_MAX) p->skillPacks[m] = SKILLPACK_MAX;
    }

    return true;
}

bool LoadGame(const char *path, SaveData *data) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    memset(data, 0, sizeof(SaveData));

    int version = 0;
    if (!ReadExact(&version, sizeof(int), 1, f) || version < 1 || version > SAVE_VERSION) {
        fclose(f);
        return false;
    }

    if (!ReadExact(&data->playerPosition, sizeof(Vector3), 1, f) ||
        !ReadExact(&data->teamSize, sizeof(int), 1, f) ||
        !ReadExact(&data->activePetIndex, sizeof(int), 1, f)) {
        fclose(f);
        return false;
    }

    int maxTeam = (int)(sizeof(data->team) / sizeof(data->team[0]));
    if (data->teamSize < 1 || data->teamSize > maxTeam) {
        fclose(f);
        return false;
    }
    if (data->activePetIndex < 0 || data->activePetIndex >= data->teamSize)
        data->activePetIndex = 0;

    if (version >= 6) {
        if (!ReadExact(&data->inventory, sizeof(Inventory), 1, f)) {
            fclose(f);
            return false;
        }
    } else {
        // Old format: 6 ints (potions, superPotions, petBalls, greatBalls, revives, gold)
        int old[6];
        if (!ReadExact(old, sizeof(int), 6, f)) {
            fclose(f);
            return false;
        }
        data->inventory.potions = old[0];
        data->inventory.superPotions = old[1];
        data->inventory.ethers = 0;
        data->inventory.petBalls = old[2];
        data->inventory.greatBalls = old[3];
        data->inventory.revives = old[4];
        data->inventory.gold = old[5];
    }
    SanitizeInventory(&data->inventory);

    if (!ReadExact(&data->playTime, sizeof(int), 1, f)) {
        fclose(f);
        return false;
    }
    if (data->playTime < 0) data->playTime = 0;

    for (int i = 0; i < data->teamSize; i++) {
        if (!ReadExact(&data->team[i], sizeof(Pet), 1, f) || !SanitizePet(&data->team[i])) {
            fclose(f);
            return false;
        }
    }

    if (version >= 2) {
        if (!ReadExact(data->npcDefeated, sizeof(bool), MAX_SAVE_NPCS, f)) {
            fclose(f);
            return false;
        }
    }

    if (version >= 3) {
        if (!ReadExact(&data->defeatedTrainerCount, sizeof(int), 1, f)) {
            fclose(f);
            return false;
        }
        if (data->defeatedTrainerCount < 0 ||
            data->defeatedTrainerCount > MAX_DEFEATED_TRAINERS) {
            fclose(f);
            return false;
        }
        if (!ReadExact(data->defeatedTrainerHashes, sizeof(unsigned int),
                       (size_t)data->defeatedTrainerCount, f)) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}
