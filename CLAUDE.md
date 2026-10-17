## Role

你是一名精通 C 语言和 raylib 游戏引擎的高级游戏工程师。你的任务是帮助用户完成 3D 宠物收集 RPG（洛克王国风格）的开发。

**项目文档入口**: 详见 `AGENTS.md`（项目概览、硬约束、安全边界、模块职责）
**进度追踪**: 详见 `PROGRESS.md`
**游戏设计**: 详见 `BESTIARY.md`（宠物/技能/战斗机制）

## 运行过程中

- 无需生成代码的介绍文件 .md，也无需运行任何命令运行进程（除非用户要求构建验证）
- 为了最大效率，当需要执行多个独立操作时，同时调用所有相关工具，而不是按顺序调用
- 修改过程中持续给用户提供选项，确认后再改代码
- 先输出 Harness（设计/代码框架），确认后再实施
- 修改 `pet_data.h` 会影响全游戏平衡，谨慎对待

## 提交时机

- 完成每个逻辑工作单元后，请自动使用 `git add` 和 `git commit` 保存进度
- 除非我明确要求，否则不要推送到远程仓库

## 技术架构

- **C GNU17** + **raylib 6.0** — 游戏引擎（3D 渲染、输入、音频）
- **raygui** — 即时模式 UI 库
- **MinGW-w64** (`D:/mingw64/bin/`) — 编译工具链
- **CMake** — 构建系统
- 窗口 1280×720，可拖拽调整大小，60 FPS

## 构建与运行

```bash
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
./roco-kingdom.exe
```

必须在 `build/` 目录下运行，程序以当前工作目录加载 `resources/`。

## 项目结构

```
roco-kingdom/
├── AGENTS.md          # LLM Agent 入口文档（项目概览/约束/模块）
├── CLAUDE.md          # Claude Code 操作指令
├── PROGRESS.md        # 当前进度追踪
├── BESTIARY.md        # 游戏设计文档（宠物/技能/机制）
├── CMakeLists.txt     # 构建配置
├── include/           # 头文件 (.h)
├── src/               # 源文件 (.c)
├── resources/
│   ├── characters/    # GLB 模型 + PNG 立绘
│   ├── models/        # 宠物 GLB 模型
│   ├── pictures/      # 宠物立绘
│   ├── shaders/       # GLSL 着色器
│   ── *.png, *.ttf   # 背景图、字体
├── vendor/raylib/     # raylib 源码（静态库）
└── build/             # CMake 输出（gitignore）
```

## 命名规范

- 文件名：snake_case（`models_3d.c`、`pet_data.h`）
- 函数名：PascalCase（`DrawPlayerModel`、`GetTerrainHeight`）
- 结构体：PascalCase（`BattleState`、`Body`）
- 常量：UPPER_SNAKE（`MAX_SPEED`、`SAVE_VERSION`）
- 缩进：4 空格

## 状态机

```
STATE_MENU → STATE_STARTER_SELECT → STATE_WORLD ↔ STATE_BATTLE
                ↓                       ↓              ↑
           STATE_BESTIARY          STATE_PET_BAG       |
                                        ↓              |
                                  STATE_GAME_OVER ←----┘
```

`main.c` 单主循环 + `switch (gameState)` 分发。`prevState != gameState` 时执行状态入口逻辑。

## 模块速查

| 模块     | 文件                 | 职责                                                                                                                                                                                                               |
| -------- | -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 玩家移动 | `player.c`           | 镜头相对 WASD 移动。朝向=`-atan2f(vel.x, vel.z)`。冲刺体力。完整地形交互：台阶攀爬(0.1~1.5u)、墙壁阻挡、悬崖边缘保护、斜坡法线速度调节                                                                             |
| 摄像机   | `camera.c`           | 第三人称轨道。鼠标控制 yaw/pitch，滚轮控制距离 3-20。target 直接跟随 `player.position.y + EYE_HEIGHT`                                                                                                              |
| 地形     | `terrain.c`          | 4 层 value-noise 程序化无限地形。手动构建 512×512 mesh + 共享 `float* heightmap`。`GetTerrainHeight` 双线性插值查询，与渲染完全一致。`HashInt`/`ShouldPlace*` 程序化生成                                           |
| 宠物模型 | `models_3d.c`        | `DrawPlayerModel`/`DrawNPCModel`/`DrawBattleArena`/`DrawPetBattleModel`。角色GLB模型（`resources/characters/`）+ 程序化fallback。`InitCharacterModels`/`InitPetModels` 加载GLB，`CloneMeshRotatedY` 预旋转顶点朝向 |
| 战斗     | `battle.c`           | 回合制。攻击动画（冲刺+挤压+震动+粒子+闪白）。敌方 PP 耗尽→挣扎。结束 1.5s 自动退出。复活令牌系统：全队阵亡时可选择一只满血复活。速度高者先手，敌方先手时先出招再选技能                                            |
| NPC      | `npc.c`              | 8 个固定 NPC（5 训练师 + 3 平民：护士/研究员/老爷爷）。平民支持多页对话树、赠礼、任务系统。野生遇敌率 0.015，遇敌前 1 秒警告提示                                                                                   |
| 训练师   | `terrain.c`+`main.c` | `ShouldPlaceTrainer` ~2.5% 密度程序化。随机漫步。`GetTrainerHash` 击败持久化                                                                                                                                       |
| 野生精灵 | `main.c`             | MAX=5 只可见游荡。0.7x 模型。接近 <2.5u 触发战斗。随机遇敌率 0.015                                                                                                                                                 |
| 魔法泉水 | `magic_spring.c`     | 世界中的免费全队恢复点。10 秒读条，期间不可移动。泉水位置 (15, 0, -20)，小地图蓝点标记                                                                                                                             |
| 商店     | `shop.c`             | 护士/行商全屏购买界面。商品：治疗药水、超级药水、能量剂、宠物球、高级球、复活令牌。行商价格浮动 0.6x~1.0x                                                                                                          |
| UI       | `ui_menu.c`          | 全部 2D 界面。战斗技能卡片 210×88 + 属性标签。图鉴4标签（宠物/道具/NPC/机制）含卡片点击详情+图片。初始选择界面。中文标签。B 键边缘检测切换背包。底部 HUD（HP/Energy/EXP 条）                                       |
| 背包     | `inventory.c`        | 7 种道具：治疗药水、超级药水、能量剂、宠物球、高级球、复活令牌、金币                                                                                                                                               |
| 宠物     | `pet.c`+`pet_data.h` | 8 种 16 招式 6 属性。种族值×等级计算属性。HP=`baseHP*3 + level*2`，Attack=`baseAttack*level/20+8`                                                                                                                  |
| 存档     | `save.c`             | SAVE_VERSION=6。`defeatedTrainerHashes[]` 持久化击败的程序化训练师。支持旧版本迁移                                                                                                                                 |
| 音频     | `audio.c`            | BGM + 6 SFX，缺失文件静默跳过                                                                                                                                                                                      |

## 当前建模方式

| 对象          | 当前方案                                                                  | 工具         |
| ------------- | ------------------------------------------------------------------------- | ------------ |
| 玩家          | `protagonist.glb` GLB模型（fallback: 程序化几何体）                       | GLB + C 代码 |
| NPC（居民）   | `nurse.glb`/`merchant.glb`/`old.glb`/`researcher.glb`（fallback: 程序化） | GLB + C 代码 |
| NPC（训练师） | `DrawCylinder`+`DrawSphere`+`DrawCube` 组合（纯代码）                     | C 代码       |
| 宠物          | GLB模型 + 程序化fallback，按属性分6种造型                                 | GLB + C 代码 |
| 地形          | 程序化噪声 + `GenMeshHeightmap` + 渐变纹理                                | C 代码       |
| 树/石头       | 基础几何体（`DrawCylinder`+`DrawSphere`）                                 | C 代码       |

**Harness 限制**：raylib 6.0 原生支持 `.glb`/`.gltf`/`.obj` 加载（`LoadModel`），无需额外依赖或插件，模型文件放 `resources/` 。

## 渲染管线

- 世界渲染：标准 `BeginMode3D(camera)` → 绘制 → `EndMode3D()`
- 战斗渲染：同上，使用独立战斗镜头
- 光照使用 raylib 默认光照模型
- 角色/宠物：GLB 模型 + rlgl 矩阵栈旋转（`rlPushMatrix`/`rlTranslatef`/`rlRotatef`/`rlScalef`），fallback 程序化几何体
- `DrawPetBattleModel` 在 GLB 模型可用时使用预旋转副本（`g_petModels`/`g_petModelsRot`），否则使用 `switch` 中的程序化分支

## 关键约束

- **严禁 `TextFormat`**——raylib 仅 4 个共用缓冲区。必须用 `snprintf(buf, sizeof(buf), ...)`
- `MAX_NPCS=8`——不能超过，多余的零值槽位在原点 (0,0,0) 导致黑色 NPC 崩溃
- **moveIds=0 是有效招式**（"撞击"）。未学习的槽 `movePPs=0`，不要依赖 `moveIds < 0` 判断空槽
- 模型脚底在 y=0，调用方传入地面高度作为 `pos.y`
- 朝向公式：模型 +Z 面朝世界 `(-sin(θ), cos(θ))`。要面朝方向 (dx,dz)，`θ = atan2f(-dx, dz)`
- B 键切换背包：主循环顶部 `bJustPressed` 手动边缘检测，绕过 raygui 干扰
- 战斗 `battleEndTime`/`battleResolved` 在 `STATE_BATTLE` 入口重置（不是 battle-over 块内）
- `IsKeyPressed` 每帧只能调用一次——同一帧多次调用会导致第二次返回 false。必须捕获到变量后用 `if/else if` 链式判断
- 自动换宠必须在 `BattleNeedsUpdate` 块外执行（因为宠物昏厥后 `waitingForInput=true` 导致 `BattleNeedsUpdate` 返回 false）
- 新增 `.c` 文件需同步更新 `CMakeLists.txt` 的 `add_executable` 源文件列表
- **GLB 渲染用 `rlRotatef(-facingAngle * RAD2DEG, 0, 1, 0)`**——OpenGL Y 轴旋转矩阵与游戏 `R()` 辅助函数符号相反，必须加负号
- **GLB 模型 Y 偏移**：`g_charModelYOff[ct]` 存储 `bb.min.y`，绘制时提升 `-minY * scale` 防止陷入地面
- **角色模型缩放**：目标高度 `2.0f / maxDim`，配合 `scale` 参数（玩家1.6f/NPC1.6f），约 3.2u 高匹配程序化 NPC
- **角色朝向**：`g_charModelFacing[]` 按角色独立调整预旋转角度（rad），加载时 `CloneMeshRotatedY` 烘焙到顶点
- **角色图片**：`resources/characters/*.png` 在图鉴 NPC 标签展示，文件名对应 `g_charModelFiles`
- **DrawTextEx 非格式化函数**——字符串中的 `%` 直接写单 `%`，不要用 `%%`（仅 `snprintf` 格式串需要 `%%`）
- **战斗技能卡片**：威力和标签（先手/高暴/回复）拼接成单行字符串，避免固定坐标重叠
- **字体子集**：`GAME_CJK[]` 仅包含 482 个游戏实际使用的 CJK 字符（97% 减少），通过 Perl 脚本从源码提取。新增中文文本需同步更新该数组
- **GLB 模型加载**：使用 `LoadModelSilent()` 包装 `SetTraceLogLevel(LOG_ERROR)` 抑制 u32→u16 索引转换警告
- **菜单背景**：`resources/background.png` 通过 `DrawFullscreenBG()` 在菜单/图鉴/初始选择/游戏结束界面全屏显示，叠加暗色遮罩保证文字可读
- **simhei.ttf 缺少 ↔ (U+2194) 字形**——使用 `/` 替代

## Harness 文档结构

本项目采用分层 Harness 文档体系，供 LLM Agent 代码审查时快速理解项目：

| 文件          | 定位                                                         | 更新频率       |
| ------------- | ------------------------------------------------------------ | -------------- |
| `AGENTS.md`   | **入口文档**：项目概览、构建命令、硬约束、安全边界、模块职责 | 架构变更时     |
| `CLAUDE.md`   | **操作指令**：Claude Code 行为规范、编码规范、关键约束详情   | 新增约束时     |
| `PROGRESS.md` | **进度追踪**：已完成/进行中/待办/已知问题                    | 每个任务完成后 |
| `BESTIARY.md` | **游戏设计**：宠物/技能/战斗机制/道具/任务系统               | 游戏设计变更时 |

### 设计原则

1. **描述靠近代码** — 模块职责在 `AGENTS.md` 速查表中，详细实现在源码注释
2. **标准化入口** — `AGENTS.md` 为唯一入口，Agent 从此开始理解项目
3. **小且完备** — 每个文档聚焦单一职责，避免巨型文档
4. **随代码更新** — 代码变更后同步更新对应文档（尤其是硬约束和进度）

### 安全/确定性/可观测性

- **安全性**: 禁止删除资源文件、禁止修改 raylib 源码、禁止超数组上限、破坏性操作需确认
- **确定性**: 程序化地形/训练师哈希保证同一坐标永远生成相同内容，存档版本化支持迁移
- **可观测性**: raylib trace log 分级、构建警告需关注、调试建议见 `AGENTS.md` 可观测性节
