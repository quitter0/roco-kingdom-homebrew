# ARCHITECTURE.md — Battle（战斗系统）

## 模块文件

- `src/battle.c` / `include/battle.h`
- `src/pet.c` / `include/pet.h`
- `include/pet_data.h` — 宠物/招式数据库（静态常量）

## 架构决策

### 回合制战斗流程

```
STATE_BATTLE 入口 → InitBattle()
  → 速度判定先手（先手技能 > 速度高者 > 玩家优先）
  → BattleSelectMove() / AI 选招
  → BattleExecuteTurn()（动画 + 伤害计算 + 状态判定）
  → 1.5s 自动退出 / 换宠 / 道具 / 捕捉
```

### 伤害公式

```
base = (2 × level / 5 + 2) × power × atk / def / 15 + 2
damage = base × 属性克制(0.6~1.6) × 随机(0.85~1.00) × 暴击(1.5x) × 灼伤衰减(0.8x)
```

### 属性克制表

| 攻→防 | 普通 | 火 | 水 | 草 | 光 | 暗 |
|--------|------|-----|-----|-----|-----|-----|
| 火 | 1.0 | 0.6 | 0.6 | 1.6 | 1.0 | 1.0 |
| 水 | 1.0 | 1.6 | 0.6 | 0.6 | 1.0 | 1.0 |
| 草 | 1.0 | 0.6 | 1.6 | 0.6 | 1.0 | 1.0 |
| 光 | 1.0 | 1.0 | 1.0 | 1.0 | 0.6 | 1.6 |
| 暗 | 1.0 | 1.0 | 1.0 | 1.0 | 1.6 | 0.6 |

### 共享 Energy 系统

初始 150 + 每级 5 点。消耗顺序：共享 Energy → 技能包独立次数。用尽且无技能包时无法出招。

### 复活令牌

全队阵亡时弹出复活选择。拒绝或令牌耗尽 → 游戏结束，存档删除。

## 接口约定

| 函数 | 说明 |
|------|------|
| `InitBattle` | 初始化战斗状态 |
| `BattleSelectMove` | 玩家选择招式槽 |
| `BattleTryCatch` | 尝试捕捉 |
| `BattleTryRun` | 尝试逃跑 |
| `BattleUseItem` | 使用道具（药水/能量剂） |
| `BattleNeedsUpdate` | 是否可执行下一回合 |
| `BattleExecuteTurn` | 执行当前回合 |
| `BattleSwitchPet` | 切换出战宠物 |

## 特殊约束

- `moveIds=0` 是有效招式（"撞击"），未学习槽用 `moveIds=-1` 哨兵，空槽判断一律用 `moveIds < 0`
- 自动换宠必须在 `BattleNeedsUpdate` 块外执行
- 战斗结束标志在 `STATE_BATTLE` 入口重置
- AI 逻辑：优先克制属性 → 最高威力 → PP 耗尽挣扎
- 属性公式：`HP=baseHP*3+level*2`，`ATK=baseATK*level/20+8`
- 暴击：基础 1/16，高暴击技能 4/16
- 状态效果：灼伤/中毒每回合扣 1/16 HP，冰冻 25% 解冻，减速 70%
