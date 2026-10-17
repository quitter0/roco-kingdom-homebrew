#include "shop.h"
#include "pet.h"
#include <stdlib.h>

const ShopEntry NURSE_SHOP[NURSE_SHOP_SIZE] = {
    { SHOP_PETBALL,     "宠物球",   200, "捕捉野生宠物" },
    { SHOP_GREATBALL,   "高级球",   600, "更高捕捉率" },
    { SHOP_POTION,      "治疗药水", 150, "回复 20% HP" },
    { SHOP_SUPERPOTION, "超级药水", 400, "回复 40% HP" },
    { SHOP_ETHER,       "能量剂",       300, "回复 30% Energy" },
    { SHOP_REVIVE,      "复活令牌",     800, "全队阵亡时选择一只宠物满血复活" },
};

bool ShopBuy(ShopItemType type, int price, Inventory *inv) {
    if (inv->gold < price) return false;
    inv->gold -= price;
    switch (type) {
        case SHOP_PETBALL:     inv->petBalls++;     break;
        case SHOP_GREATBALL:   inv->greatBalls++;   break;
        case SHOP_POTION:      inv->potions++;      break;
        case SHOP_SUPERPOTION: inv->superPotions++; break;
        case SHOP_ETHER:       inv->ethers++;       break;
        case SHOP_REVIVE:      inv->revives++;      break;
        default: break;
    }
    return true;
}

int MerchantPrice(int basePrice, float discount) {
    int p = (int)((float)basePrice * discount);
    if (p < 50) p = 50;
    return (p / 10) * 10; // round to 10s
}

void GenerateMerchant(MerchantShop *m, int speciesId, float cx, float cz) {
    (void)speciesId;  // reserved for future species-specific stock
    m->centerX = cx;
    m->centerZ = cz;

    // 1-3 random skill packs with random discounts
    int packIds[3] = { SHOP_PACK_SMALL, SHOP_PACK_MEDIUM, SHOP_PACK_LARGE };
    float discounts[4] = { 0.6f, 0.75f, 0.85f, 1.0f };

    int count = 1 + rand() % 3; // 1 to 3 packs
    for (int i = 0; i < count && i < MERCHANT_SIZE; i++) {
        int pi = rand() % 3;
        float disc = discounts[rand() % 4];
        int basePrice = (pi == 0) ? 300 : (pi == 1) ? 600 : 1200;
        int price = MerchantPrice(basePrice, disc);

        const char *tierName = (pi == 0) ? "初级技能包" : (pi == 1) ? "中级技能包" : "高级技能包";
        m->packs[i].type = packIds[pi];
        m->packs[i].price = price;
        m->packs[i].name = tierName;
        m->packs[i].desc = "随机获得威力匹配的技能并增加独立次数";
    }
    // Fill remaining slots
    for (int i = count; i < MERCHANT_SIZE; i++) {
        m->packs[i].type = -1;
        m->packs[i].name = NULL;
        m->packs[i].price = 0;
    }
}

bool ShopBuySkillPack(ShopItemType type, int price, Inventory *inv, Pet *pet) {
    if (inv->gold < price) return false;

    int pMin, pMax, packCount;
    switch (type) {
        case SHOP_PACK_SMALL:  pMin = 40; pMax = 60; packCount = 3; break;
        case SHOP_PACK_MEDIUM: pMin = 65; pMax = 85; packCount = 2; break;
        case SHOP_PACK_LARGE:  pMin = 90; pMax = 120; packCount = 1; break;
        default: return false;
    }

    int moveId = RandomMoveForPet(pet->speciesId, pMin, pMax);
    if (moveId < 0) return false; // no move available in pet's pool
    if (!ApplySkillPack(pet, moveId, packCount)) return false; // no free slot

    inv->gold -= price;
    return true;
}
