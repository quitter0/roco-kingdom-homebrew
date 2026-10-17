#ifndef PET_DATA_H
#define PET_DATA_H

#include "raylib.h"

typedef enum {
    TYPE_NORMAL = 0,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_GRASS,
    TYPE_LIGHT,
    TYPE_DARK
} PetType;

// Move flags
#define MOVE_FLAG_NONE      0
#define MOVE_FLAG_PRIORITY  (1 << 0)
#define MOVE_FLAG_HIGH_CRIT (1 << 1)
#define MOVE_FLAG_HEAL      (1 << 2)

typedef struct {
    const char *name;
    int basePower;
    int accuracy;
    PetType type;
    int ppCost;          // shared PP consumed per use
    int flags;
    int statusChance;
    int statusType;      // 0=none, 1=burn, 2=poison, 3=freeze
    int statChance;
    int statEffect;      // 0=none, 1=speedDown
    const char *description;
} MoveData;

typedef struct {
    const char *name;
    PetType type;
    int baseHP;
    int baseAttack;
    int baseDefense;
    int baseSpeed;
    int learnableMoves[6];
    int learnLevels[6];
    Color color;
} PetSpecies;

// 0=none, 1=burn, 2=poison, 3=freeze
#define STATUS_NONE   0
#define STATUS_BURN   1
#define STATUS_POISON 2
#define STATUS_FREEZE 3

// PP system
#define BASE_SHARED_PP   150
#define PP_PER_LEVEL      5
#define SKILLPACK_MAX     5

// ============ MOVE DATABASE (16 moves) ============

static const MoveData MOVE_DATABASE[] = {
    // id  name         power acc  type         cost flags                  status% stat%  effect
    { "撞击",          40,   100, TYPE_NORMAL, 15,  MOVE_FLAG_NONE,       0, 0, 0, 0, "用身体冲撞对手" },
    { "火焰拳",        70,   95,  TYPE_FIRE,   25,  MOVE_FLAG_NONE,      10, STATUS_BURN,   0, 0, "包裹火焰的拳击" },
    { "烈焰风暴",      110,  80,  TYPE_FIRE,   40,  MOVE_FLAG_NONE,       0, 0,              0, 0, "毁灭性的火焰旋涡" },
    { "水炮",          80,   90,  TYPE_WATER,  30,  MOVE_FLAG_NONE,       0, 0,              0, 0, "高压水炮攻击" },
    { "泡沫光线",      60,   100, TYPE_WATER,  20,  MOVE_FLAG_NONE,       0, 0,             10, 1, "泡沫光线，可能减速" },
    { "飞叶快刀",      65,   95,  TYPE_GRASS,  25,  MOVE_FLAG_HIGH_CRIT,  0, 0,              0, 0, "锋利叶片，容易暴击" },
    { "阳光烈焰",      120,  85,  TYPE_GRASS,  40,  MOVE_FLAG_NONE,       0, 0,              0, 0, "聚集阳光的强力光束" },
    { "暗影球",        90,   90,  TYPE_DARK,   30,  MOVE_FLAG_NONE,       0, 0,              0, 0, "黑暗能量球" },
    { "光之箭",        85,   95,  TYPE_LIGHT,  30,  MOVE_FLAG_NONE,       0, 0,              0, 0, "光之箭矢" },
    { "影袭",          50,   100, TYPE_DARK,   15,  MOVE_FLAG_PRIORITY,   0, 0,              0, 0, "先手暗影袭击" },
    { "头槌",          70,   95,  TYPE_NORMAL, 20,  MOVE_FLAG_NONE,       0, 0,              0, 0, "用头部撞击" },
    { "泰山压顶",      85,   90,  TYPE_NORMAL, 30,  MOVE_FLAG_NONE,       0, 0,              0, 0, "沉重的身体压向对手" },
    { "毒刺",          45,   95,  TYPE_GRASS,  15,  MOVE_FLAG_NONE,      20, STATUS_POISON,  0, 0, "带毒刺的攻击" },
    { "回复",           0,   100, TYPE_NORMAL, 30,  MOVE_FLAG_HEAL,       0, 0,              0, 0, "回复50%最大HP" },
    { "寒冰刺",        80,   90,  TYPE_WATER,  30,  MOVE_FLAG_NONE,      10, STATUS_FREEZE,  0, 0, "冰刺攻击，可能冰冻" },
    { "泥巴炸弹",      55,   95,  TYPE_NORMAL, 20,  MOVE_FLAG_NONE,       0, 0,             20, 1, "泥巴炸弹，可能减速" },
};

#define MOVE_COUNT (sizeof(MOVE_DATABASE) / sizeof(MOVE_DATABASE[0]))

// ============ PET DATABASE (8 species) ============

static const PetSpecies PET_DATABASE[] = {
    // 0: 火花 — Fire speed sweeper (total 200)
    {
        .name = "火花", .type = TYPE_FIRE,
        .baseHP = 40, .baseAttack = 60, .baseDefense = 37, .baseSpeed = 63,
        .learnableMoves = {1, 2, 10, 9, -1, -1},
        .learnLevels    = {1, 1,  1, 1,  0,  0},
        .color = {255, 80, 40, 255}
    },
    // 1: 水蓝蓝 — Water balanced (total 198)
    {
        .name = "水蓝蓝", .type = TYPE_WATER,
        .baseHP = 52, .baseAttack = 48, .baseDefense = 53, .baseSpeed = 45,
        .learnableMoves = {3, 4, 14, 13, -1, -1},
        .learnLevels    = {1, 1,  1,  1,  0,  0},
        .color = {40, 100, 255, 255}
    },
    // 2: 喵喵 — Grass speed disruptor (total 200)
    {
        .name = "喵喵", .type = TYPE_GRASS,
        .baseHP = 47, .baseAttack = 46, .baseDefense = 49, .baseSpeed = 58,
        .learnableMoves = {5, 12, 15, 13, -1, -1},
        .learnLevels    = {1,  1,  1,  1,  0,  0},
        .color = {50, 200, 60, 255}
    },
    // 3: 瞌睡王 — Normal tank (total 220)
    {
        .name = "瞌睡王", .type = TYPE_NORMAL,
        .baseHP = 78, .baseAttack = 51, .baseDefense = 56, .baseSpeed = 35,
        .learnableMoves = {11, 10, 13, 15, -1, -1},
        .learnLevels    = { 1,  1,  1,  1,  0,  0},
        .color = {180, 160, 140, 255}
    },
    // 4: 格兰花 — Grass cannon (total 200)
    {
        .name = "格兰花", .type = TYPE_GRASS,
        .baseHP = 50, .baseAttack = 62, .baseDefense = 43, .baseSpeed = 45,
        .learnableMoves = {5, 6, 12, 13, -1, -1},
        .learnLevels    = {1, 1,  1,  1,  0,  0},
        .color = {80, 220, 80, 255}
    },
    // 5: 雪精灵 — Water speed attacker (total 201)
    {
        .name = "雪精灵", .type = TYPE_WATER,
        .baseHP = 42, .baseAttack = 55, .baseDefense = 39, .baseSpeed = 65,
        .learnableMoves = {14, 4, 3, 9, -1, -1},
        .learnLevels    = { 1, 1, 1, 1,  0,  0},
        .color = {180, 220, 255, 255}
    },
    // 6: 小包子 — Fire bruiser (total 200)
    {
        .name = "小包子", .type = TYPE_FIRE,
        .baseHP = 56, .baseAttack = 53, .baseDefense = 52, .baseSpeed = 39,
        .learnableMoves = {1, 2, 11, 13, -1, -1},
        .learnLevels    = {1, 1,  1,  1,  0,  0},
        .color = {255, 120, 60, 255}
    },
    // 7: 幽灵 — Dark glass cannon (total 198)
    {
        .name = "幽灵", .type = TYPE_DARK,
        .baseHP = 36, .baseAttack = 62, .baseDefense = 32, .baseSpeed = 68,
        .learnableMoves = {7, 9, 8, 12, -1, -1},
        .learnLevels    = {1, 1, 1,  1,  0,  0},
        .color = {150, 100, 200, 255}
    },
};

#define PET_COUNT (sizeof(PET_DATABASE) / sizeof(PET_DATABASE[0]))

#endif
