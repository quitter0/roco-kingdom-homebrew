# ARCHITECTURE.md — UI（界面/图鉴/背包/HUD）

## 模块文件

- `src/ui_menu.c` / `include/ui_menu.h`

## 架构决策

### 2D 界面统一入口

所有 UI 通过 `ui_menu.c` 的 `Draw*Scene()` / `Update*Buttons()` 函数对管理。
状态机在 `main.c` 中调用对应函数。

### 战斗技能卡片

210×88 像素。威力和属性标签（先手/高暴/回复）拼接成单行字符串，避免固定坐标重叠。

### 图鉴系统

4 标签（宠物/道具/NPC/机制）：
- 宠物：可点击卡片 → 详情页（技能/属性/立绘）
- 道具：价格和效果列表
- NPC：4 个可互动角色卡片 → 详情页（大照片）
- 机制：属性克制表 + 状态效果说明

### 字体子集优化

`GAME_CJK[]` 仅包含 482 个游戏实际使用的 CJK 字符（从 21,262 降至 750 总字符数）。
通过 Perl 脚本从源码提取。新增中文文本需同步更新该数组。

### 菜单背景

`resources/background.png` 通过 `DrawFullscreenBG()` 在菜单/图鉴/初始选择/游戏结束界面全屏显示，叠加暗色遮罩保证文字可读。

## 接口约定

| 函数 | 说明 |
|------|------|
| `InitUI` | 初始化字体/背景纹理 |
| `DrawMenu/UpdateMenuButtons` | 主菜单 |
| `DrawWorldHUD` | 世界 HUD（HP/Energy/EXP/任务） |
| `DrawBattleScene/UpdateBattleButtons` | 战斗界面 |
| `DrawPetBagScene/UpdatePetBagButtons` | 宠物背包 |
| `DrawBestiaryScene/UpdateBestiaryButtons` | 图鉴 |
| `DrawStarterSelectScene/UpdateStarterSelectButtons` | 初始选择 |
| `DrawShopUI` | 商店界面 |
| `DrawMinimap` | 小地图（左上角） |
| `DrawNPCDialogBox` | NPC 对话框 |
| `ShowNotification/DrawNotification` | 通知 toast 系统 |

## 特殊约束

- `DrawTextEx` 非格式化函数 — `%` 直接写单 `%`，不用 `%%`（仅 `snprintf` 需要）
- `simhei.ttf` 缺 `↔` 字形 — 用 `/` 替代
- B 键切换背包：主循环顶部 `bJustPressed` 手动边缘检测，绕过 raygui 干扰
- 严禁 `TextFormat` — raylib 仅 4 个共用缓冲区，会覆盖
- `g_uiFont` 全局字体对象
