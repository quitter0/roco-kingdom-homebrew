#include "inventory.h"

void InitInventory(Inventory *inv) {
    inv->potions = 3;
    inv->superPotions = 1;
    inv->ethers = 2;
    inv->petBalls = 5;
    inv->greatBalls = 2;
    inv->revives = 1;
    inv->gold = 500;
}

bool UsePotion(Inventory *inv) {
    if (inv->potions <= 0) return false;
    inv->potions--;
    return true;
}

bool UseSuperPotion(Inventory *inv) {
    if (inv->superPotions <= 0) return false;
    inv->superPotions--;
    return true;
}

bool UseEther(Inventory *inv) {
    if (inv->ethers <= 0) return false;
    inv->ethers--;
    return true;
}

bool UsePetBall(Inventory *inv) {
    if (inv->petBalls <= 0) return false;
    inv->petBalls--;
    return true;
}

bool UseGreatBall(Inventory *inv) {
    if (inv->greatBalls <= 0) return false;
    inv->greatBalls--;
    return true;
}

bool UseRevive(Inventory *inv) {
    if (inv->revives <= 0) return false;
    inv->revives--;
    return true;
}
