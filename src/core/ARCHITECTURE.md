# ARCHITECTURE.md — Core（主循环/状态机）

## 模块文件

- `src/main.c` — 主循环入口
- `include/game_state.h` — 状态枚举

## 架构决策

### 单主循环 + 状态分发

```
main() {
    while (!WindowShouldClose()) {
        key = capture keys once per frame;
        switch (gameState) {
            case STATE_MENU:         ...
            case STATE_WORLD:        ...
            case STATE_BATTLE:       ...
            ...
        }
    }
}
```

`prevState != gameState` 时执行状态入口逻辑（初始化/重置）。

### 状态机

```
MENU → STARTER_SELECT → WORLD ↔ BATTLE
         ↓                   ↓        ↑
       BESTIARY          PET_BAG      |
                             ↓        |
                        GAME_OVER ←───┘
```

### 输入捕获原则

`IsKeyPressed` 每帧只能调用一次。必须在主循环顶部捕获到变量后，各状态用 `if/else if` 链判断。同一帧多次调用会导致第二次返回 false。

## 接口约定

| 函数/变量 | 类型 | 说明 |
|-----------|------|------|
| `gameState` | `GameState` | 全局当前状态 |
| `prevState` | `GameState` | 上一帧状态（用于入口检测） |
| `InitPlayer` | `void` | 玩家初始化 |
| `InitTerrain` | `void` | 地形初始化 |

## 特殊约束

- B 键切换背包：主循环顶部 `bJustPressed` 手动边缘检测，绕过 raygui 干扰
- 战斗 `battleEndTime`/`battleResolved` 在 `STATE_BATTLE` 入口重置，不在 battle-over 块内
- 自动换宠必须在 `BattleNeedsUpdate` 块外执行（昏厥后 `waitingForInput=true` 导致返回 false）
