#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>

typedef struct {
    int potions;
    int superPotions;
    int ethers;       // PP restore (30% shared PP)
    int petBalls;
    int greatBalls;
    int revives;
    int gold;
} Inventory;

void InitInventory(Inventory *inv);
bool UsePotion(Inventory *inv);
bool UseSuperPotion(Inventory *inv);
bool UseEther(Inventory *inv);
bool UsePetBall(Inventory *inv);
bool UseGreatBall(Inventory *inv);
bool UseRevive(Inventory *inv);

#endif
