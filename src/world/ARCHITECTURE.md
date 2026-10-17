# ARCHITECTURE.md — World（地形/玩家/摄像机）

## 模块文件

- `src/terrain.c` / `include/terrain.h` — 程序化地形
- `src/player.c` / `include/player.h` — 玩家移动
- `src/camera.c` / `include/camera.h` — 第三人称摄像机

## 架构决策

### 程序化无限地形

4 层 value-noise + `HashInt(x,z)` 确定性哈希。512×512 mesh 随玩家移动 `RecenterTerrain`。
`GetTerrainHeight` 双线性插值查询，与渲染完全一致。

### 地形类型分层

`GetTerrainType` 输出 water/grass/rock 三个比例值，用于：
- 遇敌率判断（grass 高→遇敌，water/rock 低→不遇敌）
- 树/石/训练师放置过滤（水上不放置）

### 玩家移动

镜头相对 WASD。朝向 = `-atan2f(vel.x, vel.z)`。完整地形交互：
- 台阶攀爬：0.1~1.5u 高度差自动上台阶
- 墙壁阻挡：地形坡度 > 阈值时停止
- 悬崖边缘保护：脚下无地面时减速
- 斜坡法线速度调节：上坡减速/下坡加速

### 摄像机

第三人称轨道。鼠标控制 yaw/pitch，滚轮控制距离 3-20。
target 直接跟随 `player.position.y + EYE_HEIGHT`（不做地形高度平滑）。

## 接口约定

| 函数 | 签名 | 说明 |
|------|------|------|
| `LoadTerrain` | `Terrain(void)` | 创建地形 mesh + heightmap |
| `RecenterTerrain` | `void(Terrain*, float cx, float cz)` | 移动 mesh 中心 |
| `GetTerrainHeight` | `float(Terrain*, float x, float z)` | 查询地面高度 |
| `ShouldPlaceTree/Rock` | `bool(Terrain*, int, int, float*, float*)` | 程序化放置 |
| `InitPlayer` | `void(Body*, Vector3)` | 初始化玩家 |
| `UpdatePlayer` | `void(Body*, yaw, sideway, forward, ...)` | 每帧更新 |
| `UpdateCameraFollow` | `void(Camera*, Vector3, Vector2*, float)` | 跟随玩家 |

## 特殊约束

- `HashInt` 保证同一 (x,z) 永远返回相同值 — 训练师击败状态依赖此确定性
- `MAX_NPCS=8` — 训练师占 NPC 槽位，超过会崩溃
- 地形 mesh 中心与玩家距离 > `meshSpan/2` 时才触发 `RecenterTerrain`
