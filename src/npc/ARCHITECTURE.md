# ARCHITECTURE.md — NPC（NPC/遇敌/任务/商店/泉水）

## 模块文件

- `src/npc.c` / `include/npc.h` — NPC 系统 + 遇敌
- `src/shop.c` / `include/shop.h` — 商店系统
- `src/magic_spring.c` / `include/magic_spring.h` — 魔法泉水

## 架构决策

### NPC 系统

8 个固定 NPC（`MAX_NPCS=8`）：
- 5 训练师：程序化模型，随机漫步，击败后 `GetTrainerHash` 持久化
- 3 平民（护士/研究员/老爷爷）：GLB 模型，多页对话树，赠礼 + 任务

### 任务系统

- 对话后显示在 HUD 追踪面板
- 自动完成：目标达成时立即获得金币 + 金色通知
- 循环接取：再次对话随机生成新目标和奖励
- 研究员：捕捉 2~5 只野生宠物 → 奖励 目标×100 G
- 老爷爷：击败 1~4 名训练师 → 奖励 目标×100 G

### 遇敌系统

`EncounterSystem` 结构体：
- 遇敌率 0.015（每帧移动距离加权）
- 遇敌前 1 秒 "!" 警告提示
- 野生精灵 MAX=5 只，0.7x 模型，<2.5u 触发战斗

### 商店系统

- 护士商店：固定 6 种商品（药水/球/复活令牌）
- 行商：3 个随机技能包，价格浮动 0.6x~1.0x
- 技能包：初级 +3 次（40-60 威力）300G / 中级 +2 次（65-85）600G / 高级 +1 次（90-120）1200G

### 魔法泉水

位置 (15, 0, -20)。按 E 键交互，10 秒读条（期间不可移动）。完成后全队满血满 PP，免费无限制。

## 接口约定

| 函数 | 说明 |
|------|------|
| `InitEncounterSystem` | 初始化遇敌参数 |
| `TryWildEncounter` | 判断是否遇敌 |
| `CheckNPCInteraction` | 检测 NPC 交互距离 |
| `GetNPCCount/GetNPCs` | 获取 NPC 数组 |
| `ShopBuy/ShopBuySkillPack` | 购买商品 |
| `GenerateMerchant` | 生成行商位置和商品 |
| `InitMagicSpring` | 初始化泉水 |
| `IsNearMagicSpring` | 检测交互距离 |

## 特殊约束

- `MAX_NPCS=8` — 不能超过，多余零值槽位导致原点黑色 NPC 崩溃
- 训练师击败状态通过 `defeatedTrainerHashes[]` 持久化
- 泉水仅限战斗外使用
- 行商位置随机生成，远离泉水和护士
