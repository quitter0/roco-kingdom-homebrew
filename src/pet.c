#include "pet.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Pet CreatePet(int speciesId, int level) {
    Pet pet;
    memset(&pet, 0, sizeof(Pet));
    pet.speciesId = speciesId;
    pet.level = level < 1 ? 1 : level;
    strcpy(pet.nickname, PET_DATABASE[speciesId].name);
    RecalcStats(&pet);
    pet.currentHP = pet.maxHP;
    pet.currentExp = 0;
    pet.expToNext = pet.level * pet.level * 30;
    pet.status = STATUS_NONE;
    pet.statusTimer = 0;
    pet.speedDown = false;
    pet.sharedPP = pet.maxSharedPP;
    for (int i = 0; i < 4; i++) {
        pet.moveIds[i] = -1;  // empty slot sentinel
        pet.skillPacks[i] = 0;
    }

    int moveCount = 0;
    for (int i = 0; i < 6 && moveCount < 4; i++) {
        int moveId = PET_DATABASE[speciesId].learnableMoves[i];
        if (moveId < 0) break;
        if (PET_DATABASE[speciesId].learnLevels[i] <= level) {
            pet.moveIds[moveCount++] = moveId;
        }
    }

    return pet;
}

void RecalcStats(Pet *pet) {
    const PetSpecies *species = &PET_DATABASE[pet->speciesId];
    int lv = pet->level;
    pet->maxHP = species->baseHP * 3 + lv * 2;
    pet->attack = species->baseAttack * lv / 20 + 8;
    pet->defense = species->baseDefense * lv / 20 + 8;
    pet->speed = species->baseSpeed * lv / 20 + 8;
    pet->maxSharedPP = BASE_SHARED_PP + lv * PP_PER_LEVEL;
    // Only refill shared PP if creating or leveling up (not on every stat recalc)
    if (pet->sharedPP > pet->maxSharedPP) pet->sharedPP = pet->maxSharedPP;
}

void LevelUp(Pet *pet) {
    pet->level++;
    RecalcStats(pet);
    pet->sharedPP = pet->maxSharedPP; // full refill on level up
    pet->currentHP = pet->maxHP;
    pet->expToNext = pet->level * pet->level * 30;

    const PetSpecies *species = &PET_DATABASE[pet->speciesId];
    int moveCount = 0;
    for (int i = 0; i < 4; i++) {
        if (pet->moveIds[i] >= 0) moveCount++;
    }

    for (int i = 0; i < 6 && moveCount < 4; i++) {
        if (species->learnableMoves[i] < 0) break;
        if (species->learnLevels[i] == pet->level) {
            bool alreadyKnown = false;
            for (int j = 0; j < moveCount; j++) {
                if (pet->moveIds[j] == species->learnableMoves[i]) {
                    alreadyKnown = true;
                    break;
                }
            }
            if (!alreadyKnown) {
                pet->moveIds[moveCount] = species->learnableMoves[i];
                pet->skillPacks[moveCount] = 0;
                moveCount++;
            }
        }
    }
}

void GainExp(Pet *pet, int exp) {
    pet->currentExp += exp;
    while (pet->currentExp >= pet->expToNext && pet->level < 100) {
        pet->currentExp -= pet->expToNext;
        LevelUp(pet);
    }
}

bool IsFainted(const Pet *pet) {
    return pet->currentHP <= 0;
}

void HealPet(Pet *pet) {
    pet->currentHP = pet->maxHP;
    pet->sharedPP = pet->maxSharedPP;
    pet->status = STATUS_NONE;
    pet->statusTimer = 0;
    pet->speedDown = false;
}

int GetPetCatchRate(int speciesId) {
    switch (speciesId) {
        case 0: case 6: return 40;
        case 1: case 5: return 40;
        case 2: case 4: return 35;
        case 3:          return 55;
        case 7:          return 30;
        default:         return 45;
    }
}

// ---- Skill pack usage ----
// Returns true if the pack was applied (move learned + packs added)

bool ApplySkillPack(Pet *pet, int moveId, int packCount) {
    if (packCount < 1) return false;

    // Find if this move is already known
    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if (pet->moveIds[i] == moveId) { slot = i; break; }
    }

    if (slot < 0) {
        // Not yet learned — find an empty slot
        for (int i = 0; i < 4; i++) {
            if (pet->moveIds[i] < 0) { slot = i; break; }
        }
    }

    if (slot < 0) return false; // no slot available

    pet->moveIds[slot] = moveId;
    pet->skillPacks[slot] += packCount;
    if (pet->skillPacks[slot] > SKILLPACK_MAX)
        pet->skillPacks[slot] = SKILLPACK_MAX;
    return true;
}

// Pick a random move from the pet's learnable pool within power range
int RandomMoveForPet(int speciesId, int powerMin, int powerMax) {
    const PetSpecies *sp = &PET_DATABASE[speciesId];
    int candidates[6];
    int candCount = 0;
    int strongest = -1;
    for (int i = 0; i < 6; i++) {
        int mid = sp->learnableMoves[i];
        if (mid < 0) break;
        int pw = MOVE_DATABASE[mid].basePower;
        if (pw >= powerMin && pw <= powerMax) {
            // Deduplicate
            bool dup = false;
            for (int j = 0; j < candCount; j++)
                if (candidates[j] == mid) { dup = true; break; }
            if (!dup) candidates[candCount++] = mid;
        }
        if (pw > 0 && (strongest < 0 || pw > MOVE_DATABASE[strongest].basePower))
            strongest = mid;
    }
    // No move in range: fall back to the pet's strongest damaging move
    if (candCount == 0) return strongest;
    return candidates[rand() % candCount];
}
