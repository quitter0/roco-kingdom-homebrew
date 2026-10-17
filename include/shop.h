#ifndef SHOP_H
#define SHOP_H

#include "inventory.h"
#include "pet.h"
#include <stdbool.h>

// Shop item types
typedef enum {
    SHOP_PETBALL,
    SHOP_GREATBALL,
    SHOP_POTION,
    SHOP_SUPERPOTION,
    SHOP_ETHER,
    SHOP_REVIVE,
    SHOP_PACK_SMALL,   // primary skill pack (power 40-60)
    SHOP_PACK_MEDIUM,  // intermediate (65-85)
    SHOP_PACK_LARGE,   // advanced (90-120)
} ShopItemType;

typedef struct {
    ShopItemType type;
    const char *name;
    int price;
    const char *desc;
} ShopEntry;

// Nurse shop: fixed inventory
#define NURSE_SHOP_SIZE 6
extern const ShopEntry NURSE_SHOP[NURSE_SHOP_SIZE];

// Merchant: 3 random skill packs with random discounts
#define MERCHANT_SIZE 3
typedef struct {
    ShopEntry packs[MERCHANT_SIZE];
    float centerX, centerZ;  // world position
} MerchantShop;

// Returns true if player can afford, then deducts gold and uses item
bool ShopBuy(ShopItemType type, int price, Inventory *inv);
bool ShopBuySkillPack(ShopItemType type, int price, Inventory *inv, Pet *pet);
void GenerateMerchant(MerchantShop *m, int speciesId, float cx, float cz);
int MerchantPrice(int basePrice, float discount);

#endif
