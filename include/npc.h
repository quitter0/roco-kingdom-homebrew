#ifndef NPC_H
#define NPC_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_NPCS 8

typedef struct {
    Vector3 position;
    float baseX;
    float baseZ;
    Color color;
    const char *name;
    const char *dialog;          // single default dialog
    const char *dialogs[4];      // multi-page dialog tree (non-trainer NPCs)
    int dialogCount;             // number of pages in dialog tree
    bool isTrainer;
    int petSpeciesId;
    int petLevel;
    bool defeated;
    bool givenItem;              // has given free item to player
    int giftItemType;            // 0=none, 1=potion, 2=super, 3=ether, 4=petBall, 5=gold
    int giftCount;               // how many of the item
    // Quest system
    bool hasQuest;               // has a quest for the player
    int questType;               // 0=catch wild pets, 1=defeat trainers
    int questTarget;             // number of captures/defeats needed
    int questProgress;           // current progress
    int questReward;             // gold reward
    bool questComplete;          // quest completed (reward given)
    bool questAccepted;          // player has accepted the quest via dialog
} NPC;

typedef struct {
    float stepCounter;
    float encounterRate;
    float interactionDistance;
    bool warningActive;      // "!" warning before random encounter
    float warningTimer;      // countdown to battle
    int warningSpecies;
    int warningLevel;
} EncounterSystem;

void InitEncounterSystem(EncounterSystem *es);
bool TryWildEncounter(EncounterSystem *es, float deltaDist, float terrainHeight);
bool EncounterWarningTick(EncounterSystem *es, float delta);
void ClearEncounterWarning(EncounterSystem *es);
int GetWildPetForTerrain(float terrainHeight);
bool CheckNPCInteraction(Vector3 playerPos, const NPC *npc, float dist);
int GetNPCCount(void);
NPC *GetNPCs(void);

#endif
