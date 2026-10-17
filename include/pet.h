#ifndef PET_H
#define PET_H

#include "pet_data.h"
#include <stdbool.h>

typedef struct {
    int speciesId;
    char nickname[32];
    int level;
    int currentHP;
    int maxHP;
    int attack;
    int defense;
    int speed;
    int moveIds[4];
    int currentExp;
    int expToNext;
    int status;
    int statusTimer;
    bool speedDown;
    int sharedPP;       // shared PP pool, consumed by all moves
    int maxSharedPP;    // = BASE_SHARED_PP + level * PP_PER_LEVEL
    int skillPacks[4];  // per-slot independent uses (0..SKILLPACK_MAX), from purchased packs
} Pet;

Pet CreatePet(int speciesId, int level);
void RecalcStats(Pet *pet);
void LevelUp(Pet *pet);
void GainExp(Pet *pet, int exp);
bool IsFainted(const Pet *pet);
void HealPet(Pet *pet);
int GetPetCatchRate(int speciesId);
bool ApplySkillPack(Pet *pet, int moveId, int packCount);
int RandomMoveForPet(int speciesId, int powerMin, int powerMax);

#endif
