#include "npc.h"
#include "pet_data.h"
#include <stdlib.h>
#include <math.h>

static NPC npcs[MAX_NPCS] = {
    {
        .position = {30, 0, 20}, .baseX = 30, .baseZ = 20,
        .color = {255, 200, 50, 255},
        .name = "训练师小明", .dialog = "来一场热血的宠物对决吧！",
        .isTrainer = true, .petSpeciesId = 1, .petLevel = 6, .defeated = false
    },
    {
        .position = {-40, 0, -30}, .baseX = -40, .baseZ = -30,
        .color = {100, 255, 150, 255},
        .name = "训练师小红", .dialog = "我的草系宠物可是很强的！",
        .isTrainer = true, .petSpeciesId = 2, .petLevel = 6, .defeated = false
    },
    {
        .position = {50, 0, -50}, .baseX = 50, .baseZ = -50,
        .color = {255, 150, 100, 255},
        .name = "训练师小刚", .dialog = "火焰是最强的力量！",
        .isTrainer = true, .petSpeciesId = 6, .petLevel = 7, .defeated = false
    },
    {
        .position = {-60, 0, 40}, .baseX = -60, .baseZ = 40,
        .color = {180, 200, 255, 255},
        .name = "训练师小雪", .dialog = "水能克火，火能克草…懂了吗？",
        .isTrainer = true, .petSpeciesId = 5, .petLevel = 7, .defeated = false
    },
    {
        .position = {10, 0, -70}, .baseX = 10, .baseZ = -70,
        .color = {220, 180, 255, 255},
        .name = "神秘训练师", .dialog = "我的幽灵宠物会让你害怕！",
        .isTrainer = true, .petSpeciesId = 7, .petLevel = 10, .defeated = false
    },
    {
        .position = {-20, 0, 60}, .baseX = -20, .baseZ = 60,
        .color = {200, 200, 200, 255},
        .name = "老爷爷", .dialog = "在草丛里走走，可能会遇到野生宠物哦！",
        .dialogs = {"年轻人，你也喜欢宠物吗？", "草丛里藏着许多野生宠物，多走走就能遇到！", "打败训练师能获得经验和金币，去试试吧！", ""},
        .dialogCount = 3, .isTrainer = false, .petSpeciesId = -1, .petLevel = 0, .defeated = false,
        .givenItem = false, .giftItemType = 3, .giftCount = 1,  // 1 Ether as gift
        .hasQuest = true, .questType = 1, .questTarget = 2, .questProgress = 0,
        .questReward = 200, .questComplete = false   // Defeat 2 trainers → 200G
    },
    {
        .position = {70, 0, 10}, .baseX = 70, .baseZ = 10,
        .color = {150, 220, 150, 255},
        .name = "研究员", .dialog = "战胜训练师可以获得经验和道具。",
        .dialogs = {"欢迎来到宠物研究所！", "我们正在研究宠物与训练师的羁绊...", "宠物的属性克制非常重要，火克草、草克水、水克火。", "来，这个给你，对你的冒险有帮助！"},
        .dialogCount = 4, .isTrainer = false, .petSpeciesId = -1, .petLevel = 0, .defeated = false,
        .givenItem = false, .giftItemType = 1, .giftCount = 2,  // 2 治疗药水 as gift
        .hasQuest = true, .questType = 0, .questTarget = 3, .questProgress = 0,
        .questReward = 300, .questComplete = false   // Catch 3 wild pets → 300G
    },
    {
        .position = {-40, 0, 10}, .baseX = -40, .baseZ = 10,
        .color = {255, 200, 200, 255},
        .name = "护士", .dialog = "去魔法泉水那里恢复吧！泉水可以完全治愈你的宠物。",
        .dialogs = {"需要帮助吗？", "魔法泉水可以恢复所有宠物的HP和Energy，完全免费！", "我这里也出售道具，需要什么尽管看看。", ""},
        .dialogCount = 3, .isTrainer = false, .petSpeciesId = -1, .petLevel = 0, .defeated = false,
        .givenItem = false, .giftItemType = 0, .giftCount = 0
    },
};

void InitEncounterSystem(EncounterSystem *es) {
    es->stepCounter = 0;
    es->encounterRate = 0.015f;  // reduced from 0.03
    es->interactionDistance = 5.0f;
    es->warningActive = false;
    es->warningTimer = 0.0f;
    es->warningSpecies = 0;
    es->warningLevel = 3;
}

bool TryWildEncounter(EncounterSystem *es, float deltaDist, float terrainHeight) {
    if (es->warningActive) return false; // already warning, don't stack
    if (deltaDist < 0.001f) return false;
    es->stepCounter += deltaDist;
    if (es->stepCounter < 3.0f) return false;
    es->stepCounter = 0;

    float rate = es->encounterRate;
    if (terrainHeight > 16.0f) rate *= 0.5f;
    if (terrainHeight < 2.0f) rate *= 0.3f;

    float roll = (float)rand() / (float)RAND_MAX;
    if (roll < rate) {
        // Activate warning — battle triggers after 1 second
        es->warningActive = true;
        es->warningTimer = 1.0f;
        es->warningSpecies = GetWildPetForTerrain(terrainHeight);
        es->warningLevel = 2 + rand() % 5;
        return false;  // don't trigger yet, warning first
    }
    return false;
}

// Returns true when warning timer expires (battle should start now)
bool EncounterWarningTick(EncounterSystem *es, float delta) {
    if (!es->warningActive) return false;
    es->warningTimer -= delta;
    if (es->warningTimer <= 0.0f) {
        es->warningActive = false;
        return true;  // time to fight!
    }
    return false;
}

void ClearEncounterWarning(EncounterSystem *es) {
    es->warningActive = false;
    es->warningTimer = 0.0f;
}

int GetWildPetForTerrain(float terrainHeight) {
    int candidates[4];
    int count = 0;

    for (int i = 0; i < (int)PET_COUNT; i++) {
        bool match = false;
        if (terrainHeight < 2.0f && PET_DATABASE[i].type == TYPE_WATER) match = true;
        if (terrainHeight >= 2.0f && terrainHeight < 14.0f &&
            (PET_DATABASE[i].type == TYPE_GRASS || PET_DATABASE[i].type == TYPE_NORMAL)) match = true;
        if (terrainHeight >= 14.0f && PET_DATABASE[i].type == TYPE_FIRE) match = true;
        if (match && count < 4) candidates[count++] = i;
    }

    if (count == 0) {
        for (int i = 0; i < (int)PET_COUNT && count < 4; i++) {
            candidates[count++] = i;
        }
    }

    return candidates[rand() % count];
}

bool CheckNPCInteraction(Vector3 playerPos, const NPC *npc, float dist) {
    if (npc->isTrainer && npc->defeated) return false;
    float dx = playerPos.x - npc->position.x;
    float dz = playerPos.z - npc->position.z;
    return sqrtf(dx * dx + dz * dz) < dist;
}

int GetNPCCount(void) { return MAX_NPCS; }
NPC *GetNPCs(void) { return npcs; }
