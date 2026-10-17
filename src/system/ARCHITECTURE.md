# ARCHITECTURE.md — System（存档/音频）

## 模块文件

- `src/save.c` / `include/save.h` — 存档系统
- `src/audio.c` / `include/audio.h` — 音频系统

## 架构决策

### 存档版本化

`SAVE_VERSION=6`。`SaveData` 结构体包含完整游戏状态快照。
每次新增字段递增版本号，`LoadGame` 处理旧版本兼容迁移。

### 训练师击败持久化

`defeatedTrainerHashes[64]` 存储被击败的程序化训练师哈希值。
`GetTrainerHash(cellX, cellZ)` 确定性计算 — 同一坐标永远返回相同值。

### 音频系统

**资源位置**:
- BGM: `resources/bgms/world.mp3`（世界）、`resources/bgms/battle.mp3`（战斗）、`resources/audio/menu.wav`（菜单）
- 文件名必须用 ASCII（MinGW `FileExists` 不支持中文路径）
- SFX: `resources/audio/*.wav`（6 个：select/damage/heal/catch/win/run）

**全局指针**: `g_audio`（`audio.h`），供任意模块调用 `PlaySFX`

**SFX 触发点**:
| 事件 | SFX | 位置 |
|------|-----|------|
| 伤害结算 | `SFX_DAMAGE` | `battle.c` `defender->currentHP -= dmg` 之后 |
| 道具治疗 | `SFX_HEAL` | `battle.c` `BattleUseItem` 各 case |
| 回复招式 | `SFX_HEAL` | `battle.c` `MOVE_FLAG_HEAL` 分支 |
| 捕捉成功 | `SFX_CATCH` | `battle.c` `caught=true` |
| 战斗胜利 | `SFX_WIN` | `battle.c` `playerWon=true` |
| 逃跑成功 | `SFX_RUN` | `battle.c` `playerRan=true` |

BGM 和 SFX 通过 `*Loaded[]` 布尔数组追踪加载状态。
缺失文件不 crash，`FileExists` 检查失败静默跳过。

## 接口约定

| 函数 | 说明 |
|------|------|
| `SaveFileExists` | 检查存档文件 |
| `SaveGame/LoadGame` | 读写存档 |
| `InitAudioManager` | 加载全部音频 |
| `PlayBGM` | 切换背景音乐（MENU/WORLD/BATTLE） |
| `PlaySFX` | 播放音效（SELECT/DAMAGE/HEAL/CATCH/WIN/RUN） |
| `UpdateAudioManager` | 每帧更新 BGM 流 |
| `UnloadAudioManager` | 释放音频资源 |

## 特殊约束

- SAVE_VERSION 递增规则：新字段兼容旧存档时 +1
- 旧存档迁移：逐字段填充默认值
- 游戏失败：存档立即标记无效（下次启动自动删除）
- SaveData 为全量快照（位置/队伍/道具/击败记录），不支持增量
