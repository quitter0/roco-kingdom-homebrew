#ifndef BATTLE_H
#define BATTLE_H

#include "pet.h"
#include "inventory.h"
#include <stdbool.h>

typedef struct {
    int moveId;
    PetType moveType;
    bool isPlayerAttack;
    float duration;
} BattleAnimInfo;

typedef struct {
    Pet *playerTeam;
    int teamSize;
    int activePetIdx;
    Pet enemyPet;
    bool isPlayerTurn;
    bool waitingForInput;
    bool battleOver;
    bool playerWon;
    bool playerRan;
    bool caught;
    bool isTrainerBattle;  // trainer battles reject capture attempts
    bool needsRevive;   // all pets fainted, waiting for revive decision
    int selectedMove;
    char message[256];
    float messageTimer;
    int rewardExp;
    double animStartTime;
    BattleAnimInfo animInfo;
    // Two-phase attack animation
    bool counterPending;          // true when first attack anim done, waiting for counter
    float counterDelay;           // gap between animations (seconds)
    int enemySlot;                // enemy's chosen move slot (persisted for counter phase)
    bool playerActs;               // whether player selected a move this turn
    bool playerFirst;             // whether player goes first
    bool enemyActedFirst;         // enemy attacked first this turn, player responds next
} BattleState;

void InitBattle(BattleState *b, Pet *team, int teamSize, int activeIdx, Pet *enemyPet);
void BattleSelectMove(BattleState *b, int moveSlot);
void BattleTryCatch(BattleState *b, Inventory *inv);
void BattleTryRun(BattleState *b);
// Item types for battle item menu
typedef enum { BITEM_POTION, BITEM_SUPERPOTION, BITEM_ETHER } BattleItem;
void BattleUseItem(BattleState *b, Inventory *inv, BattleItem item);
bool BattleNeedsUpdate(BattleState *b);
void BattleExecuteTurn(BattleState *b);
bool BattleIsPlayerTurn(const BattleState *b);
bool BattleIsOver(const BattleState *b);
void BattleSwitchPet(BattleState *b, int newIdx);

#endif
