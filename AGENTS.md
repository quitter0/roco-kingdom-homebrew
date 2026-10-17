# AGENTS.md — Roco Kingdom 3D Pet RPG

## 项目概览

洛克王国风格 3D 宠物收集回合制 RPG。C GNU17 + raylib 6.0，单线程主循环，程序化地形，GLB 角色模型。

**窗口**: 1280×720, 60 FPS, 可拖拽调整大小
**工具链**: MinGW-w64 (D:/mingw64/bin/) + CMake + raylib 6.0
**状态机**: MENU → STARTER_SELECT → WORLD ↔ BATTLE (↔ PET_BAG, BESTIARY, GAME_OVER)

---

## 构建与运行

```bash
cd build
cmake -G "MinGW Makefiles" .. && mingw32-make
./roco-kingdom.exe
```

必须在 `build/` 目录运行（程序以 CWD 加载 `resources/`）。
修改 `CMakeLists.txt` 的源文件列表后必须重新运行 cmake。

---

## 硬约束（违反会导致崩溃/bug）

### 内存与缓冲区
- **严禁 `TextFormat`** — raylib 仅 4 个共用缓冲区，会覆盖。必须用 `snprintf(buf, sizeof(buf), ...)`
- **`IsKeyPressed` 每帧只能调用一次** — 多次调用第二次返回 false。必须捕获到变量后用 `if/else if` 链
- **`MAX_NPCS=8`** — 不能超过，多余的零值槽位导致原点黑色 NPC 崩溃

### 渲染
- **GLB 旋转**: `rlRotatef(-facingAngle * RAD2DEG, 0, 1, 0)` — OpenGL Y 轴与游戏 R() 符号相反
- **GLB Y 偏移**: 绘制时提升 `-bb.min.y * scale` 防止陷入地面
- **角色模型缩放**: 目标高度 `2.0f / maxDim`，配合 scale 参数约 3.2u 高
- **模型脚底在 y=0**，调用方传入地面高度作为 `pos.y`
- **朝向公式**: `θ = atan2f(-dx, dz)`，模型 +Z 面朝世界 `(-sin(θ), cos(θ))`

### 战斗系统
- **moveIds=0 是有效招式**（"撞击"），未学习槽 `movePPs=0`，不依赖 `< 0` 判断
- **自动换宠在 `BattleNeedsUpdate` 块外执行** — 昏厥后 `waitingForInput=true` 导致返回 false
- **战斗结束标志在 `STATE_BATTLE` 入口重置**，不在 battle-over 块内
- **B 键切换背包**: 主循环顶部 `bJustPressed` 手动边缘检测，绕过 raygui 干扰

### 字体与文本
- **`DrawTextEx` 非格式化函数** — `%` 直接写单 `%`，不用 `%%`（仅 `snprintf` 需要）
- **`GAME_CJK[]` 仅 482 字符** — 新增中文需同步更新该数组
- **simhei.ttf 缺 `↔` 字形** — 用 `/` 替代

### 资源加载
- **GLB 加载用 `LoadModelSilent()`** — 包装 `SetTraceLogLevel(LOG_ERROR)` 抑制 u32→u16 警告
- **资源文件名必须用 ASCII** — MinGW `FileExists`/`fopen` 不支持中文路径（已重命名 `王国城堡.mp3` → `world.mp3`）
- **CMake `file(COPY)` 仅 configure 时运行** — 新增资源需重新 cmake

---

## 安全边界

### 禁止操作
- 不要删除 `resources/` 中的文件（GLB 模型、背景图、字体）
- 不要修改 `vendor/raylib/` 源码（上游依赖）
- 不要修改 `pet_data.h` 的种族值（影响全游戏平衡，需用户确认）
- 不要超过 `MAX_NPCS=8`、`MAX_PARTY_PETS=6`、`MAX_WILD_PETS=5`

### 允许操作
- 修改 UI 布局、文本内容、战斗数值微调
- 新增 `.c/.h` 文件（需同步更新 `CMakeLists.txt`）
- 调整 GLB 模型朝向/缩放参数

### 破坏性操作需确认
- `git push`（无远程配置，需用户提供 URL）
- 删除存档逻辑修改
- 状态机结构变更

---

## 确定性保障

- **程序化地形**: 4 层 value-noise + `HashInt`/`ShouldPlace*`，同一坐标永远生成相同内容
- **训练师击败状态**: `defeatedTrainerHashes[]` 持久化，`GetTrainerHash` 确定性计算
- **存档版本**: `SAVE_VERSION=6`，支持旧版本迁移
- **音频缺失**: 静默跳过，不影响游戏流程

---

## 可观测性

### 日志级别
- raylib trace log 默认 WARNING，模型加载时临时切 ERROR
- 构建警告需关注（尤其是未使用变量、类型转换）

### 调试建议
- 地形问题: 检查 `GetTerrainHeight` 与渲染 heightmap 是否一致
- 战斗问题: 检查 `battleEndTime`/`battleResolved` 重置时机
- NPC 问题: 检查 `npcCount` 是否超过 8

---

## 项目结构

```
roco-kingdom/
├── AGENTS.md               # 本文件：入口文档
├── CLAUDE.md               # Claude Code 指令（与 AGENTS.md 互补）
├── PROGRESS.md             # 进度追踪
├── BESTIARY.md             # 游戏设计文档（宠物/技能/战斗机制）
├── CMakeLists.txt          # 构建配置
├── include/                # 头文件
── src/                    # 源文件 + 模块文档
│   ├── core/ARCHITECTURE.md     # 主循环/状态机
│   ├── world/ARCHITECTURE.md    # 地形/玩家/摄像机
│   ├── render/ARCHITECTURE.md   # 3D 渲染/模型
│   ├── battle/ARCHITECTURE.md   # 战斗/宠物数据
│   ├── npc/ARCHITECTURE.md      # NPC/遇敌/商店/泉水
│   ├── ui/ARCHITECTURE.md       # UI/图鉴/HUD
│   └── system/ARCHITECTURE.md   # 存档/音频
── resources/         # 资源文件
│   ├── characters/    # GLB 模型 + PNG 立绘
│   ├── models/        # 宠物 GLB 模型
│   ├── pictures/      # 宠物立绘
│   ├── shaders/       # GLSL 着色器
│   └── *.png, *.ttf   # 背景图、字体
├── vendor/raylib/     # raylib 源码（静态库）
└── build/             # CMake 输出（gitignore）
```

---

## 模块职责速查

| 模块 | 文件 | 职责 |
|------|------|------|
| 主循环 | `main.c` | 状态机分发、世界/NPC/遇敌逻辑 |
| 玩家 | `player.c` | WASD 移动、地形交互（台阶/墙壁/悬崖/斜坡） |
| 摄像机 | `camera.c` | 第三人称轨道、鼠标 yaw/pitch、滚轮距离 3-20 |
| 地形 | `terrain.c` | 4 层噪声、512×512 mesh、程序化训练师放置 |
| 角色模型 | `models_3d.c` | GLB 加载/渲染、程序化 fallback、预旋转顶点 |
| 战斗 | `battle.c` | 回合制、动画（冲刺/挤压/震动/粒子/闪白）、PP 耗尽挣扎 |
| NPC | `npc.c` | 8 固定 NPC（5 训练师 + 3 平民）、对话树、任务系统 |
| 野生精灵 | `main.c` | MAX=5 只、0.7x 模型、<2.5u 触发战斗、遇敌率 0.015 |
| 宠物 | `pet.c` + `pet_data.h` | 8 种 16 招式 6 属性、种族值公式 |
| 背包 | `inventory.c` | 7 种道具、金币 |
| 商店 | `shop.c` | 护士/行商购买界面、价格浮动 0.6x-1.0x |
| 魔法泉水 | `magic_spring.c` | 免费全队恢复、10 秒读条、位置 (15,0,-20) |
| UI | `ui_menu.c` | 2D 界面、战斗技能卡片、图鉴、初始选择 |
| 存档 | `save.c` | SAVE_VERSION=6、训练师击败持久化 |
| 音频 | `audio.c` | BGM + 6 SFX、缺失静默跳过 |

---

## 当前进度

详见 `PROGRESS.md`

---

## 编码规范

- **文件名**: snake_case（`models_3d.c`）
- **函数名**: PascalCase（`DrawPlayerModel`）
- **结构体**: PascalCase（`BattleState`）
- **常量**: UPPER_SNAKE（`MAX_SPEED`）
- **缩进**: 4 空格
- **注释**: 仅当 WHY 非显而易见时（隐藏约束、特定 bug workaround）
