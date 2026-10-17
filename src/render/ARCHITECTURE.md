# ARCHITECTURE.md — Render（3D 渲染/模型）

## 模块文件

- `src/models_3d.c` / `include/models_3d.h`

## 架构决策

### 双轨渲染策略

所有角色/宠物使用 **GLB 模型优先 + 程序化 fallback**：
- GLB 加载成功 → 使用 `DrawCharModelGLB()` / `DrawPetBattleModel` GLB 分支
- GLB 加载失败/缺失 → 使用 `DrawCylinder/DrawSphere/DrawCube` 程序化几何体

### GLB 加载

raylib 6.0 原生 `LoadModel()` 支持 `.glb`。大模型（>65K 顶点）触发 u32→u16 索引转换警告，用 `LoadModelSilent()` 包装 `SetTraceLogLevel(LOG_ERROR)` 抑制。

### 渲染管线

```
BeginMode3D(camera)
  rlPushMatrix()
    rlTranslatef(pos.x, pos.y + yOff, pos.z)   // Y 偏移防陷入
    rlRotatef(-facingAngle * RAD2DEG, 0, 1, 0) // 负号！OpenGL 约定相反
    rlScalef(ms, ms, ms)
    DrawModel(model, origin, 1.0f, tint)
  rlPopMatrix()
EndMode3D()
```

### 预旋转顶点

`CloneMeshRotatedY` 在加载时烘焙朝向到顶点法线，避免每帧计算旋转矩阵。

## 接口约定

| 函数 | 说明 |
|------|------|
| `DrawPlayerModel` | 玩家渲染（GLB + walk 动画） |
| `DrawNPCModel` | NPC 渲染（charType≥0 用 GLB，-1 用程序化） |
| `DrawPetBattleModel` | 战斗宠物渲染（攻击/闲置动画） |
| `DrawBattleArena` | 战斗场地圆形绘制 |
| `InitPetModels/InitCharacterModels` | 启动时加载 |
| `UnloadPetModels/UnloadCharacterModels` | 退出时释放 |

## 关键参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 角色目标高度 | `2.0f / maxDim` | 约 3.2u 配合 scale 1.6f |
| GLB Y 偏移 | `-bb.min.y * scale` | 脚底对齐地面 |
| GLB 旋转 | `-facingAngle * RAD2DEG` | OpenGL Y 轴符号相反 |
| 朝向公式 | `atan2f(-dx, dz)` | 模型 +Z 面朝世界 |

## 特殊约束

- `g_charModelFacing[ct]` 按角色独立调整预旋转角度（rad）
- `g_charModelYOff[ct]` 存储 `bb.min.y`，绘制时提升防止陷入
- `RAD2DEG` raylib 已定义，不要重复 `#define`
- 模型脚底在 y=0，调用方传入地面高度作为 `pos.y`
- `g_charModelFiles[]` 文件名对应 `resources/characters/*.glb`
- `resources/characters/*.png` 在图鉴 NPC 标签展示
