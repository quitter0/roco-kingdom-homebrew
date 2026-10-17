#include "ui_menu.h"
#include "pet_data.h"
// raygui's implementation does not annotate unused parameters; keep -Wextra noise out.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include "models_3d.h"
#include "terrain.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Font g_uiFont = {0};
static Texture2D g_petTextures[8];
static bool g_petTexLoaded[8] = {false};
static Texture2D g_charTextures[CHAR_MODEL_COUNT];
static bool g_charTexLoaded[CHAR_MODEL_COUNT] = {false};
static Texture2D g_bgTexture = {0};
static bool g_bgLoaded = false;

// All CJK characters used in game text (auto-extracted from src/ and include/)
// 482 unique codepoints — 97% reduction from full CJK block (0x4E00-0x9FFF)
static const int GAME_CJK[] = {
    0x4E00, 0x4E0A, 0x4E0D, 0x4E0E, 0x4E14, 0x4E1B, 0x4E2A, 0x4E2D, 0x4E3A, 0x4E45, 0x4E48, 0x4E4B,
    0x4E50, 0x4E5F, 0x4E60, 0x4E70, 0x4E86, 0x4E92, 0x4EA1, 0x4EA4, 0x4EAB, 0x4EBA, 0x4EC0, 0x4EE4,
    0x4EE5, 0x4EEC, 0x4EF7, 0x4EFB, 0x4F11, 0x4F18, 0x4F19, 0x4F1A, 0x4F20, 0x4F24, 0x4F34, 0x4F3C,
    0x4F46, 0x4F4D, 0x4F4E, 0x4F4F, 0x4F53, 0x4F55, 0x4F59, 0x4F5C, 0x4F60, 0x4F7F, 0x4F9B, 0x4FDD,
    0x503C, 0x50CF, 0x5148, 0x5149, 0x514B, 0x514D, 0x5165, 0x5168, 0x516C, 0x5170, 0x5171, 0x5173,
    0x5177, 0x518D, 0x5192, 0x51B0, 0x51B2, 0x51B3, 0x51BB, 0x51CF, 0x51E0, 0x51FA, 0x51FB, 0x5200,
    0x5206, 0x5217, 0x5219, 0x521A, 0x521D, 0x5229, 0x5230, 0x5236, 0x523A, 0x5242, 0x524A, 0x524D,
    0x5269, 0x529B, 0x529F, 0x52A0, 0x52A1, 0x52A8, 0x52A9, 0x52B1, 0x5305, 0x5339, 0x5347, 0x5355,
    0x5361, 0x538B, 0x539A, 0x539F, 0x53BB, 0x53D1, 0x53D6, 0x53D7, 0x53EA, 0x53EF, 0x53F6, 0x5404,
    0x5408, 0x540C, 0x540D, 0x540E, 0x5411, 0x5417, 0x5426, 0x5427, 0x5458, 0x547D, 0x548C, 0x54E5,
    0x54E6, 0x552E, 0x5546, 0x5584, 0x559C, 0x55B5, 0x56DE, 0x56FD, 0x56FE, 0x5728, 0x5730, 0x573A,
    0x5757, 0x57CE, 0x57FA, 0x5821, 0x589E, 0x58EB, 0x58EE, 0x58F0, 0x5904, 0x590D, 0x5916, 0x591A,
    0x5927, 0x5929, 0x5931, 0x5934, 0x5956, 0x597D, 0x59CB, 0x59D0, 0x5A01, 0x5B50, 0x5B66, 0x5B8C,
    0x5B9E, 0x5BA0, 0x5BB3, 0x5BB6, 0x5BB9, 0x5BD2, 0x5BF9, 0x5C04, 0x5C0F, 0x5C11, 0x5C31, 0x5C3D,
    0x5C45, 0x5C5E, 0x5C71, 0x5DC5, 0x5DE8, 0x5DEE, 0x5DF2, 0x5DF4, 0x5E01, 0x5E08, 0x5E26, 0x5E2E,
    0x5E38, 0x5E74, 0x5E76, 0x5E7D, 0x5E8F, 0x5E97, 0x5EA6, 0x5F00, 0x5F03, 0x5F0F, 0x5F31, 0x5F39,
    0x5F3A, 0x5F53, 0x5F62, 0x5F71, 0x5F88, 0x5F97, 0x5FA1, 0x5FC3, 0x5FEB, 0x5FFD, 0x6001, 0x6015,
    0x6025, 0x6027, 0x603B, 0x6062, 0x606F, 0x6108, 0x610F, 0x6148, 0x61A8, 0x61C2, 0x620F, 0x6210,
    0x6211, 0x6218, 0x6240, 0x624B, 0x624E, 0x6253, 0x6263, 0x6280, 0x62A2, 0x62A4, 0x62D4, 0x62DB,
    0x62E9, 0x62F3, 0x6301, 0x6309, 0x6323, 0x6349, 0x6355, 0x6377, 0x63A5, 0x63D0, 0x649E, 0x64C5,
    0x653B, 0x653E, 0x6548, 0x654C, 0x654F, 0x6570, 0x6574, 0x6597, 0x65B0, 0x65B9, 0x65CB, 0x65CF,
    0x65E0, 0x65F6, 0x660E, 0x6613, 0x662F, 0x666E, 0x6697, 0x66B4, 0x66F4, 0x6700, 0x6709, 0x670D,
    0x6717, 0x672A, 0x672C, 0x6735, 0x673A, 0x6740, 0x675F, 0x6765, 0x6781, 0x679C, 0x67D4, 0x683C,
    0x69CC, 0x6B21, 0x6B22, 0x6B63, 0x6B64, 0x6BC1, 0x6BCF, 0x6BD2, 0x6BD4, 0x6C14, 0x6C34, 0x6C38,
    0x6C89, 0x6CA1, 0x6CAB, 0x6CBB, 0x6CC9, 0x6CD5, 0x6CE1, 0x6CE5, 0x6CE8, 0x6CF0, 0x6D3B, 0x6D6E,
    0x6D88, 0x6DA1, 0x6E29, 0x6E38, 0x6EDA, 0x6EE1, 0x706B, 0x706D, 0x7075, 0x707C, 0x70AE, 0x70B8,
    0x70B9, 0x70C8, 0x70ED, 0x7130, 0x7237, 0x7247, 0x724C, 0x7269, 0x72B6, 0x72EC, 0x732B, 0x7387,
    0x738B, 0x73A9, 0x73B0, 0x7403, 0x751F, 0x7528, 0x7597, 0x7684, 0x76F8, 0x770B, 0x7740, 0x7761,
    0x778C, 0x77E2, 0x7814, 0x7840, 0x795E, 0x7965, 0x79CD, 0x79D2, 0x79D8, 0x79FB, 0x7A00, 0x7A76,
    0x7ACB, 0x7AD9, 0x7B49, 0x7BA1, 0x7BAD, 0x7CBE, 0x7CFB, 0x7EA2, 0x7EA7, 0x7EBF, 0x7EC3, 0x7ECA,
    0x7ECF, 0x7ED9, 0x7EE7, 0x7EED, 0x7F6E, 0x7F81, 0x7FA4, 0x8001, 0x8005, 0x8010, 0x8017, 0x805A,
    0x80CC, 0x80DC, 0x80FD, 0x8131, 0x81EA, 0x81F3, 0x826F, 0x8272, 0x82B1, 0x8349, 0x836F, 0x83B7,
    0x83DC, 0x84DD, 0x85CF, 0x8840, 0x884C, 0x8868, 0x88AB, 0x88AD, 0x88F9, 0x8981, 0x89C1, 0x89C6,
    0x89E3, 0x8BA9, 0x8BAD, 0x8BB8, 0x8BBE, 0x8BD5, 0x8BDD, 0x8BF4, 0x8D25, 0x8D27, 0x8D2D, 0x8D39,
    0x8D60, 0x8D62, 0x8D70, 0x8D85, 0x8DD1, 0x8DEF, 0x8E2A, 0x8E81, 0x8EAB, 0x8F6E, 0x8F7B, 0x8F83,
    0x8FB9, 0x8FC7, 0x8FCE, 0x8FD1, 0x8FD4, 0x8FD9, 0x8FDC, 0x9000, 0x9001, 0x9002, 0x9003, 0x9009,
    0x901A, 0x901F, 0x9020, 0x9047, 0x9053, 0x90A3, 0x90CE, 0x90E8, 0x914D, 0x91CA, 0x91CC, 0x91CD,
    0x91CE, 0x91CF, 0x91D1, 0x9274, 0x949F, 0x950B, 0x952E, 0x957F, 0x95ED, 0x961F, 0x9632, 0x9633,
    0x9635, 0x963F, 0x964D, 0x9669, 0x9677, 0x968F, 0x9694, 0x96C6, 0x96E8, 0x96EA, 0x9700, 0x975E,
    0x9760, 0x9762, 0x97F3, 0x9876, 0x987A, 0x989D, 0x98CE, 0x98D8, 0x98DE, 0x9996, 0x9A8C, 0x9AD8,
    0x9B54, 0x9ED1,
};
#define GAME_CJK_COUNT 482

static int *BuildCJKCodepoints(int *outCount) {
    // 256 ASCII + game-used CJK subset (482 chars) + extra symbols (12)
    int capacity = 256 + GAME_CJK_COUNT + 16;
    int *cps = (int *)malloc(capacity * sizeof(int));
    if (!cps) { *outCount = 0; return NULL; }
    int idx = 0;
    for (int i = 0; i < 256; i++) cps[idx++] = i;
    for (int i = 0; i < GAME_CJK_COUNT; i++) cps[idx++] = GAME_CJK[i];
    static const int extras[] = {0x2014, 0x2026, 0x2192, 0x2194,
        0x3001, 0x3002, 0xFF01, 0xFF08, 0xFF09, 0xFF0C, 0xFF1A, 0xFF1F};
    for (int i = 0; i < (int)(sizeof(extras)/sizeof(extras[0])); i++) cps[idx++] = extras[i];
    *outCount = idx;
    return cps;
}

void InitUI(void) {
    int cpCount;
    int *cps = BuildCJKCodepoints(&cpCount);

    const char *fontPaths[] = {
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/msyh.ttc",
        "resources/simhei.ttf",
        NULL
    };

    bool loaded = false;
    for (int i = 0; fontPaths[i]; i++) {
        if (FileExists(fontPaths[i])) {
            Font f = LoadFontEx(fontPaths[i], 32, cps, cpCount);
            // raylib cannot load .ttc and returns the default font on failure
            if (IsFontValid(f)) {
                g_uiFont = f;
                loaded = true;
                break;
            }
            UnloadFont(f);
        }
    }

    if (!loaded) {
        g_uiFont = GetFontDefault();
    }

    free(cps);

    // Load pet pictures for bestiary (ASCII filenames: pet_pic_0.png ~ pet_pic_7.png)
    for (int i = 0; i < (int)PET_COUNT; i++) {
        char path[64];
        snprintf(path, sizeof(path), "resources/pictures/pet_pic_%d.png", i);
        if (FileExists(path)) {
            g_petTextures[i] = LoadTexture(path);
            g_petTexLoaded[i] = true;
        }
    }

    // Load character pictures for bestiary NPC tab
    static const char *charPicFiles[CHAR_MODEL_COUNT] = {
        "resources/characters/protagonist.png",  // CHAR_MODEL_PLAYER
        "resources/characters/nurse.png",        // CHAR_MODEL_NURSE
        "resources/characters/merchant.png",     // CHAR_MODEL_MERCHANT
        "resources/characters/old.png",          // CHAR_MODEL_OLD
        "resources/characters/researcher.png",   // CHAR_MODEL_RESEARCHER
    };
    for (int i = 0; i < CHAR_MODEL_COUNT; i++) {
        if (FileExists(charPicFiles[i])) {
            g_charTextures[i] = LoadTexture(charPicFiles[i]);
            g_charTexLoaded[i] = true;
        }
    }

    // Load menu background
    if (FileExists("resources/background.png")) {
        g_bgTexture = LoadTexture("resources/background.png");
        g_bgLoaded = true;
    }

    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 22);
    GuiSetFont(g_uiFont);
}

// ==================== Main Menu ====================

int UpdateMenuButtons(bool canContinue) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scl = sh / 720.0f;
    float bw = 200 * scl, bh = 45 * scl, gap = 50 * scl;
    float baseY = sh / 2 - gap * 1.5f;

    Rectangle btnNew = {sw / 2.0f - bw / 2, baseY, bw, bh};
    if (GuiButton(btnNew, "New Game")) return 1;

    Rectangle btnLoad = {sw / 2.0f - bw / 2, baseY + gap, bw, bh};
    if (canContinue) {
        if (GuiButton(btnLoad, "Continue")) return 2;
    } else {
        GuiDisable();
        GuiButton(btnLoad, "Continue");
        GuiEnable();
    }

    Rectangle btnBestiary = {sw / 2.0f - bw / 2, baseY + gap * 2, bw, bh};
    if (GuiButton(btnBestiary, "图鉴")) return 4;

    Rectangle btnQuit = {sw / 2.0f - bw / 2, baseY + gap * 3, bw, bh};
    if (GuiButton(btnQuit, "Quit")) return 3;

    return 0;
}

static void DrawFullscreenBG(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (g_bgLoaded) {
        // Scale to fill screen preserving aspect ratio
        float scale = (float)sw / g_bgTexture.width;
        float hScale = (float)sh / g_bgTexture.height;
        if (hScale > scale) scale = hScale;
        int drawW = (int)(g_bgTexture.width * scale);
        int drawH = (int)(g_bgTexture.height * scale);
        int drawX = (sw - drawW) / 2;
        int drawY = (sh - drawH) / 2;
        DrawTexturePro(g_bgTexture,
            (Rectangle){0, 0, (float)g_bgTexture.width, (float)g_bgTexture.height},
            (Rectangle){(float)drawX, (float)drawY, (float)drawW, (float)drawH},
            (Vector2){0, 0}, 0.0f, WHITE);
        // Dark overlay for text readability
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 100});
    } else {
        ClearBackground((Color){20, 40, 80, 255});
    }
}

void DrawMenu(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawFullscreenBG();

    float scl = sh / 720.0f;
    int titleSize = (int)(50 * scl), subSize = (int)(18 * scl), hintSize = (int)(14 * scl);

    const char *title = "Roco Kingdom";
    Vector2 titleSz = MeasureTextEx(g_uiFont, title, titleSize, 1);
    DrawTextEx(g_uiFont, title, (Vector2){(float)(sw - titleSz.x) / 2, (float)sh / 2 - 170 * scl}, titleSize, 1, (Color){255, 220, 50, 255});

    const char *subtitle = "A pet-collecting adventure";
    Vector2 subSz = MeasureTextEx(g_uiFont, subtitle, subSize, 1);
    DrawTextEx(g_uiFont, subtitle, (Vector2){(float)(sw - subSz.x) / 2, (float)sh / 2 - 115 * scl}, subSize, 1, (Color){200, 200, 220, 255});

    const char *hint = "Use mouse to click buttons";
    Vector2 hintSz = MeasureTextEx(g_uiFont, hint, hintSize, 1);
    DrawTextEx(g_uiFont, hint, (Vector2){(float)(sw - hintSz.x) / 2, (float)sh - 30 * scl}, hintSize, 1, (Color){150, 150, 180, 255});
}

// ==================== World HUD ====================

void DrawWorldHUD(const Pet *team, int teamSize, int activeIdx, const Inventory *inv, float playerY, float stamina) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scl = sh / 720.0f;
    char buf[128];

    // Height (top-right)
    int topW = (int)(177 * scl), topH = (int)(30 * scl);
    DrawRectangle(sw - topW - 8, 8, topW, topH, (Color){0, 0, 0, 120});
    snprintf(buf, sizeof(buf), "Height: %.1f", playerY);
    DrawTextEx(g_uiFont, buf, (Vector2){(float)(sw - topW), 12 * scl}, 18 * scl, 1, WHITE);

    // Stamina bar
    DrawRectangle(sw - topW - 8, (int)(42 * scl), topW, (int)(22 * scl), (Color){0, 0, 0, 120});
    DrawRectangle(sw - topW - 6, (int)(44 * scl), (int)(173 * scl), (int)(18 * scl), (Color){40, 40, 60, 255});
    float stamRatio = stamina / 100.0f;
    DrawRectangle(sw - topW - 6, (int)(44 * scl), (int)(173 * scl * stamRatio), (int)(18 * scl), (Color){255, 200, 50, 255});
    snprintf(buf, sizeof(buf), "SP: %.0f", stamina);
    DrawTextEx(g_uiFont, buf, (Vector2){(float)(sw - topW + 2), 43 * scl}, 14 * scl, 1, WHITE);

    // Bottom-left: active pet HP + Energy + EXP
    if (teamSize > 0) {
        const Pet *pet = &team[activeIdx];
        int panelH = (int)(130 * scl);
        int baseY = sh - panelH - 8;
        DrawRectangle(8, baseY, (int)(230 * scl), panelH, (Color){0, 0, 0, 140});

        snprintf(buf, sizeof(buf), "%s  Lv.%d", pet->nickname, pet->level);
        DrawTextEx(g_uiFont, buf, (Vector2){16 * scl, (float)(baseY + 4)}, 18 * scl, 1, WHITE);

        snprintf(buf, sizeof(buf), "HP: %d/%d", pet->currentHP, pet->maxHP);
        DrawTextEx(g_uiFont, buf, (Vector2){16 * scl, (float)(baseY + 24 * scl)}, 14 * scl, 1, (Color){100, 255, 100, 255});
        DrawRectangle((int)(16 * scl), (int)(baseY + 42 * scl), (int)(200 * scl), (int)(7 * scl), (Color){60, 60, 60, 255});
        float hpRatio = (float)pet->currentHP / (float)pet->maxHP;
        DrawRectangle((int)(16 * scl), (int)(baseY + 42 * scl), (int)(200 * scl * hpRatio), (int)(7 * scl), (Color){50, 200, 50, 255});

        snprintf(buf, sizeof(buf), "Energy: %d/%d", pet->sharedPP, pet->maxSharedPP);
        DrawTextEx(g_uiFont, buf, (Vector2){16 * scl, (float)(baseY + 52 * scl)}, 14 * scl, 1, (Color){100, 180, 255, 255});
        DrawRectangle((int)(16 * scl), (int)(baseY + 70 * scl), (int)(200 * scl), (int)(7 * scl), (Color){60, 60, 60, 255});
        float ppRatio = (float)pet->sharedPP / (float)pet->maxSharedPP;
        DrawRectangle((int)(16 * scl), (int)(baseY + 70 * scl), (int)(200 * scl * ppRatio), (int)(7 * scl), (Color){60, 140, 220, 255});

        snprintf(buf, sizeof(buf), "EXP: %d/%d", pet->currentExp, pet->expToNext);
        DrawTextEx(g_uiFont, buf, (Vector2){16 * scl, (float)(baseY + 80 * scl)}, 13 * scl, 1, (Color){255, 220, 100, 255});
        DrawRectangle((int)(16 * scl), (int)(baseY + 96 * scl), (int)(200 * scl), (int)(6 * scl), (Color){60, 60, 60, 255});
        float expRatio = (float)pet->currentExp / (float)pet->expToNext;
        DrawRectangle((int)(16 * scl), (int)(baseY + 96 * scl), (int)(200 * scl * expRatio), (int)(6 * scl), (Color){80, 120, 255, 255});
    }

    // Bottom-right: inventory
    int invW = (int)(205 * scl), invH = (int)(95 * scl);
    DrawRectangle(sw - invW - 8, sh - invH - 8, invW, invH, (Color){0, 0, 0, 140});
    snprintf(buf, sizeof(buf), "球: %d  复活: %d", inv->petBalls + inv->greatBalls,
             inv->revives);
    DrawTextEx(g_uiFont, buf, (Vector2){(float)(sw - invW), (float)(sh - invH)}, 16 * scl, 1, WHITE);
    snprintf(buf, sizeof(buf), "治疗: %d  超级: %d", inv->potions, inv->superPotions);
    DrawTextEx(g_uiFont, buf, (Vector2){(float)(sw - invW), (float)(sh - invH + 22 * scl)}, 16 * scl, 1, WHITE);
    snprintf(buf, sizeof(buf), "能量: %d  金币: %d", inv->ethers, inv->gold);
    DrawTextEx(g_uiFont, buf, (Vector2){(float)(sw - invW), (float)(sh - invH + 44 * scl)}, 16 * scl, 1, WHITE);

    // Quest tracker - show active quests from NPCs
    {
        NPC *qnpcs = GetNPCs();
        int qY = (int)(8 + 160.0f * scl + 16);
        for (int qi = 0; qi < GetNPCCount(); qi++) {
            if (!qnpcs[qi].hasQuest || !qnpcs[qi].questAccepted || qnpcs[qi].questComplete) continue;
            char qbuf[80];
            if (qnpcs[qi].questType == 0)
                snprintf(qbuf, sizeof(qbuf), "%s: 捕捉 %d/%d", qnpcs[qi].name,
                         qnpcs[qi].questProgress, qnpcs[qi].questTarget);
            else
                snprintf(qbuf, sizeof(qbuf), "%s: 击败 %d/%d", qnpcs[qi].name,
                         qnpcs[qi].questProgress, qnpcs[qi].questTarget);
            int qh = (int)(22 * scl);
            DrawRectangle(8, qY, (int)(220 * scl), qh, (Color){0, 0, 0, 140});
            Color qc = qnpcs[qi].questProgress >= qnpcs[qi].questTarget
                ? (Color){100, 255, 100, 255} : (Color){255, 220, 100, 255};
            DrawTextEx(g_uiFont, qbuf, (Vector2){14, (float)qY + 2}, 14 * scl, 1, qc);
            qY += qh + 4;
        }
    }

    // Bottom hint
    DrawTextEx(g_uiFont, "[B] Bag  [E] Talk  [Shift] Sprint  [ESC] Menu",
             (Vector2){(float)sw / 2 - 180 * scl, (float)sh - 22 * scl}, 14 * scl, 1, (Color){220, 220, 220, 200});
}

// ==================== Minimap ====================

#define MMAP_BLOCK 5    // each sampled block covers 5x5 minimap pixels

static float MmapSize(void) {
    int sh = GetScreenHeight();
    return 160.0f * (sh / 720.0f);
}

static void DrawMinimapEdgeMarker(float mmX, float mmY, Vector2 wp, Color col) {
    // Clamp world position to minimap edge and draw a small circle
    float mmSize = MmapSize();
    float cx = wp.x, cy = wp.y;
    float mmCX = mmX + mmSize * 0.5f;
    float mmCY = mmY + mmSize * 0.5f;
    float dirX = cx - mmCX, dirY = cy - mmCY;
    float len = sqrtf(dirX * dirX + dirY * dirY);
    if (len < 1.0f) return;
    dirX /= len; dirY /= len;
    // Project to edge
    float t = 1.0f;
    if (dirX > 0) { float tt = ((mmX + mmSize - 3) - mmCX) / dirX; if (tt < t) t = tt; }
    if (dirX < 0) { float tt = ((mmX + 3) - mmCX) / dirX; if (tt < t) t = tt; }
    if (dirY > 0) { float tt = ((mmY + mmSize - 3) - mmCY) / dirY; if (tt < t) t = tt; }
    if (dirY < 0) { float tt = ((mmY + 3) - mmCY) / dirY; if (tt < t) t = tt; }
    float ex = mmCX + dirX * t, ey = mmCY + dirY * t;
    // Draw small circle marker
    DrawCircle((int)ex, (int)ey, 3, col);
    DrawCircleLines((int)ex, (int)ey, 3, WHITE);
}

void DrawMinimap(Terrain *t, Vector3 playerPos, float playerFacing,
                 const NPC *npcs, int npcCount,
                 float merchantCX, float merchantCZ, bool merchantActive,
                 float springX, float springZ) {
    float mmSize = MmapSize();
    int mmX = 8, mmY = 8;  // top-left corner
    float halfSpan = t->meshSpan * 0.5f;

    // Background
    DrawRectangle(mmX - 2, mmY - 2, (int)mmSize + 4, (int)mmSize + 4, (Color){0, 0, 0, 180});
    DrawRectangleLines(mmX - 2, mmY - 2, (int)mmSize + 4, (int)mmSize + 4, (Color){255, 255, 255, 100});

    // Sample terrain in blocks
    int blocks = (int)mmSize / MMAP_BLOCK;
    for (int bz = 0; bz < blocks; bz++) {
        for (int bx = 0; bx < blocks; bx++) {
            float wx = t->centerX - halfSpan + ((float)bx + 0.5f) * (float)MMAP_BLOCK * (t->meshSpan / mmSize);
            float wz = t->centerZ - halfSpan + ((float)bz + 0.5f) * (float)MMAP_BLOCK * (t->meshSpan / mmSize);
            float water, grass, rock;
            GetTerrainType(t, wx, wz, &water, &grass, &rock);
            Color col;
            if (water > 0.5f)      col = (Color){40, 100, 200, 140};
            else if (rock > 0.5f)  col = (Color){160, 150, 140, 140};
            else                    col = (Color){50, 160, 50, 140};
            DrawRectangle(mmX + bx * MMAP_BLOCK, mmY + bz * MMAP_BLOCK, MMAP_BLOCK, MMAP_BLOCK, col);
        }
    }

    // Player dot
    Vector2 pp = {
        mmX + (playerPos.x - (t->centerX - halfSpan)) / t->meshSpan * mmSize,
        mmY + (playerPos.z - (t->centerZ - halfSpan)) / t->meshSpan * mmSize
    };
    DrawCircle((int)pp.x, (int)pp.y, 4, WHITE);

    // Player facing direction arrow
    float arrowLen = 8.0f * (mmSize / 160.0f);
    float fx = pp.x - sinf(playerFacing) * arrowLen;
    float fy = pp.y + cosf(playerFacing) * arrowLen;
    DrawLine((int)pp.x, (int)pp.y, (int)fx, (int)fy, (Color){255, 255, 100, 255});
    DrawCircle((int)fx, (int)fy, 2, (Color){255, 255, 100, 255});

    // Magic Spring (blue dot or edge marker)
    {
        Vector2 sp = {
            mmX + (springX - (t->centerX - halfSpan)) / t->meshSpan * mmSize,
            mmY + (springZ - (t->centerZ - halfSpan)) / t->meshSpan * mmSize
        };
        float dx = sp.x - (float)mmX, dy = sp.y - (float)mmY;
        if (dx > 0 && dx < mmSize && dy > 0 && dy < mmSize) {
            DrawCircle((int)sp.x, (int)sp.y, 5, (Color){60, 160, 255, 255});
            DrawCircle((int)sp.x, (int)sp.y, 2, (Color){30, 80, 200, 255});
        } else {
            DrawMinimapEdgeMarker((float)mmX, (float)mmY, sp, (Color){60, 160, 255, 255});
        }
    }

    // Merchant (gold dot or edge marker)
    if (merchantActive) {
        Vector2 mp = {
            mmX + (merchantCX - (t->centerX - halfSpan)) / t->meshSpan * mmSize,
            mmY + (merchantCZ - (t->centerZ - halfSpan)) / t->meshSpan * mmSize
        };
        float dx = mp.x - (float)mmX, dy = mp.y - (float)mmY;
        if (dx > 0 && dx < mmSize && dy > 0 && dy < mmSize) {
            DrawCircle((int)mp.x, (int)mp.y, 5, (Color){255, 200, 50, 255});
            DrawCircle((int)mp.x, (int)mp.y, 2, (Color){180, 130, 20, 255});
        } else {
            DrawMinimapEdgeMarker((float)mmX, (float)mmY, mp, (Color){255, 200, 50, 255});
        }
    }

    // Nurse NPCs (red cross or edge marker)
    for (int i = 0; i < npcCount; i++) {
        if (!npcs[i].isTrainer && strcmp(npcs[i].name, "护士") == 0) {
            Vector2 np = {
                mmX + (npcs[i].position.x - (t->centerX - halfSpan)) / t->meshSpan * mmSize,
                mmY + (npcs[i].position.z - (t->centerZ - halfSpan)) / t->meshSpan * mmSize
            };
            float dx = np.x - (float)mmX, dy = np.y - (float)mmY;
            if (dx > 0 && dx < mmSize && dy > 0 && dy < mmSize) {
                DrawCircle((int)np.x, (int)np.y, 5, (Color){255, 80, 80, 255});
                DrawLine((int)np.x - 3, (int)np.y, (int)np.x + 3, (int)np.y, WHITE);
                DrawLine((int)np.x, (int)np.y - 3, (int)np.x, (int)np.y + 3, WHITE);
            } else {
                DrawMinimapEdgeMarker((float)mmX, (float)mmY, np, (Color){255, 80, 80, 255});
            }
        }
    }
}

// ==================== Battle UI ====================

static Color TypeUIColor(PetType t) {
    switch (t) {
        case TYPE_FIRE:  return (Color){255, 120, 40, 255};
        case TYPE_WATER: return (Color){60, 160, 255, 255};
        case TYPE_GRASS: return (Color){60, 210, 60, 255};
        case TYPE_LIGHT: return (Color){255, 220, 50, 255};
        case TYPE_DARK:  return (Color){160, 80, 220, 255};
        default:         return (Color){200, 200, 200, 255};
    }
}

int UpdateBattleButtons(BattleState *b) {
    if (!BattleIsPlayerTurn(b) || BattleIsOver(b)) return -1;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Pet *pp = &b->playerTeam[b->activePetIdx];

    // 2x2 card grid
    int cardW = 210, cardH = 88;
    int gapX = 8, gapY = 6;
    int gridX = sw - (cardW * 2 + gapX) - 12;
    int gridY = sh - 260;

    int action = -1;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 4; i++) {
        int col = i % 2, row = i / 2;
        Rectangle card = {
            (float)gridX + col * (cardW + gapX),
            (float)gridY + row * (cardH + gapY),
            (float)cardW, (float)cardH
        };

        int moveId = pp->moveIds[i];
        bool hasMove = (moveId >= 0);
        bool hasRes = hasMove && (pp->skillPacks[i] > 0 || pp->sharedPP >= MOVE_DATABASE[moveId].ppCost);
        bool hover = CheckCollisionPointRec(mouse, card);

        // Card background
        Color bg;
        if (!hasMove)       bg = (Color){50, 50, 55, 230};
        else if (!hasRes)   bg = (Color){70, 25, 25, 230};
        else if (hover)     bg = (Color){45, 55, 90, 240};
        else                bg = (Color){28, 35, 60, 230};

        DrawRectangleRec(card, bg);
        Color border = hover && hasRes ? (Color){140, 170, 240, 220}
                                       : (Color){80, 90, 130, 160};
        DrawRectangleLinesEx(card, 1, border);
        DrawRectangleLinesEx((Rectangle){card.x+1, card.y+1, card.width-2, card.height-2},
                             1, (Color){255,255,255,20});

        if (hasMove) {
            const MoveData *move = &MOVE_DATABASE[moveId];
            Color tc = TypeUIColor(move->type);

            // Type pill
            DrawRectangle(card.x + 6, card.y + 5, 28, 16, tc);
            char keyLabel[4];
            snprintf(keyLabel, sizeof(keyLabel), "%d", i + 1);
            DrawTextEx(g_uiFont, keyLabel, (Vector2){card.x + 10, card.y + 3}, 12, 1, WHITE);

            // Move name + cost
            DrawTextEx(g_uiFont, move->name, (Vector2){card.x + 40, card.y + 3}, 16, 1, tc);
            char costLabel[16];
            snprintf(costLabel, sizeof(costLabel), "-%d", move->ppCost);
            DrawTextEx(g_uiFont, costLabel, (Vector2){card.x + cardW - 32, card.y + 3}, 13, 1,
                       (Color){180, 200, 240, 255});

            // Power / effect + flags on same line
            char pwrLine[64] = "";
            float pwrX = card.x + 6;
            if (move->basePower > 0) {
                snprintf(pwrLine, sizeof(pwrLine), "威力 %d", move->basePower);
            } else if (move->flags & MOVE_FLAG_HEAL) {
                snprintf(pwrLine, sizeof(pwrLine), "回复 50%% HP");
            }
            if (move->flags & MOVE_FLAG_PRIORITY) {
                int len = (int)strlen(pwrLine);
                snprintf(pwrLine + len, sizeof(pwrLine) - len, "  [先手]");
            }
            if (move->flags & MOVE_FLAG_HIGH_CRIT) {
                int len = (int)strlen(pwrLine);
                snprintf(pwrLine + len, sizeof(pwrLine) - len, "  [高暴]");
            }
            if (pwrLine[0])
                DrawTextEx(g_uiFont, pwrLine, (Vector2){pwrX, card.y + 24}, 13, 1, (Color){255,220,100,255});

            // Skill pack dots
            int dotY = card.y + cardH - 22;
            DrawTextEx(g_uiFont, "额外", (Vector2){card.x + 6, dotY}, 11, 1, (Color){180,180,180,200});
            for (int d = 0; d < SKILLPACK_MAX; d++) {
                float dx = card.x + 36.0f + (float)d * 14.0f;
                Color dc = (d < pp->skillPacks[i]) ? (Color){255, 220, 50, 255} : (Color){60, 60, 60, 200};
                DrawCircle((int)(dx + 5), dotY + 6, 5, dc);
            }

            // Click
            if (hasRes && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                action = i;
        } else {
            // Unlearned slot
            DrawTextEx(g_uiFont, "未学习", (Vector2){card.x + cardW/2 - 24, card.y + cardH/2 - 10},
                       18, 1, (Color){120,120,120,180});
        }
    }

    // Item buttons: catch / item row + run full width
    int bottomY = sh - 38;
    int halfW = cardW - 2;
    int totalW = cardW * 2 + gapX;
    int rowX = gridX;

    Rectangle btnCatch = {(float)rowX, (float)bottomY - 39, (float)halfW, 34};
    Rectangle btnItem  = {(float)rowX + halfW + 4, (float)bottomY - 39, (float)halfW, 34};
    Rectangle btnRun   = {(float)rowX, (float)bottomY, (float)totalW, 34};

    if (GuiButton(btnCatch, "捕捉 [C]")) return 4;
    if (GuiButton(btnItem,  "道具 [H]")) return 6;
    if (GuiButton(btnRun,   "逃跑 [R]")) return 5;

    return action;
}

void DrawBattleScene(const BattleState *b) {
    ClearBackground((Color){20, 25, 50, 255});

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    char buf[128];

    // ---- 3D Battle Scene ----
    float t = (float)GetTime();
    float animT = t;
    float animDur = b->animInfo.duration > 0 ? b->animInfo.duration : 1.0f;
    bool isAttacking = (b->animStartTime > 0.0 && b->animInfo.duration > 0.0f &&
                        animT - (float)b->animStartTime < animDur);
    float animPhase = 0;
    float animElapsed = 0;
    if (isAttacking) {
        animElapsed = animT - (float)b->animStartTime;
        animPhase = animElapsed / animDur;
    }

    // --- Get move info for particle styling ---
    PetType mType = b->animInfo.moveType;
    int mPower = (b->animInfo.moveId >= 0 && b->animInfo.moveId < (int)MOVE_COUNT)
        ? MOVE_DATABASE[b->animInfo.moveId].basePower : 40;
    float powerScale = mPower < 55 ? 0.7f : mPower < 80 ? 1.0f : 1.4f;

    // --- Particle colors per type ---
    Color pCol; // primary particle color
    Color pCol2; // secondary/glow
    switch(mType){
        case TYPE_FIRE:   pCol=(Color){255,130,20,255}; pCol2=(Color){255,200,50,255}; break;
        case TYPE_WATER:  pCol=(Color){40,150,255,255}; pCol2=(Color){150,220,255,255}; break;
        case TYPE_GRASS:  pCol=(Color){40,200,50,255}; pCol2=(Color){160,255,100,255}; break;
        default:          pCol=(Color){220,220,220,255}; pCol2=(Color){255,255,150,255}; break;
    }

    // --- Camera shake ---
    float shakeX=0, shakeY=0, flashAlpha=0;
    if(isAttacking){
        // Shake peaks during impact (phase 0.5-0.7)
        float shakePhase = animPhase;
        float shakeStr;
        if(shakePhase < 0.25f) shakeStr = shakePhase/0.25f * 0.1f;
        else if(shakePhase < 0.5f) shakeStr = 0.1f + (shakePhase-0.25f)/0.25f * 0.6f;
        else if(shakePhase < 0.7f) shakeStr = 0.7f - (shakePhase-0.5f)/0.2f * 0.55f;
        else shakeStr = 0.15f - (shakePhase-0.7f)/0.3f * 0.15f;
        shakeStr *= powerScale;
        shakeX = sinf(animT*50)*shakeStr;
        shakeY = cosf(animT*47)*shakeStr;
        // Flash peaks at impact
        if(shakePhase >= 0.48f && shakePhase < 0.62f)
            flashAlpha = (1.0f - (shakePhase-0.48f)/0.14f) * 0.55f * powerScale;
    }

    Camera3D battleCam = {0};
    battleCam.position = (Vector3){0.0f, 4.5f, 8.0f};
    battleCam.target = (Vector3){shakeX, 0.8f + shakeY, 0.0f};
    battleCam.up = (Vector3){0.0f, 1.0f, 0.0f};
    battleCam.fovy = 55.0f;
    battleCam.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(battleCam);
    {
        // Battle arena platform
        DrawBattleArena((Vector3){0, 0, 0}, 4.5f);

        // Ground grid around arena
        DrawGrid(12, 2.0f);

        // Ambient decorative orbs floating around
        for (int i = 0; i < 8; i++) {
            float angle = t * 0.4f + (float)i * PI / 4.0f;
            float r = 5.5f;
            float ox = cosf(angle) * r;
            float oz = sinf(angle) * r;
            float oy = 0.3f + sinf(t * 1.5f + i) * 0.4f;
            Color oc = {
                (unsigned char)(60 + sinf(t + i) * 30),
                (unsigned char)(100 + cosf(t + i) * 40),
                (unsigned char)(180 + sinf(t * 0.7f + i) * 40),
                80
            };
            DrawSphere((Vector3){ox, oy, oz}, 0.12f, oc);
        }

        // Player pet
        Vector3 playerPetPos = {-2.0f, 0.18f, 2.2f};
        Vector3 enemyPetPos = {2.0f, 0.18f, -1.5f};

        // ---- Attack lunge: attacker moves toward defender, defender stays ----
        if (isAttacking) {
            Vector3 *attackerPos, *defenderPos;
            if (b->animInfo.isPlayerAttack) {
                attackerPos = &playerPetPos;
                defenderPos = &enemyPetPos;
            } else {
                attackerPos = &enemyPetPos;
                defenderPos = &playerPetPos;
            }
            Vector3 baseAttacker = *attackerPos;

            // Direction from attacker to defender (in XZ plane)
            float dirX = defenderPos->x - baseAttacker.x;
            float dirZ = defenderPos->z - baseAttacker.z;
            float dirLen = sqrtf(dirX * dirX + dirZ * dirZ);
            if (dirLen > 0.01f) { dirX /= dirLen; dirZ /= dirLen; }

            // Phase: Charge forward (0-50%) — attacker lunges straight to defender (no wind-up)
            if (animPhase < 0.5f) {
                float lp = animPhase / 0.5f;
                // Ease-out: fast start, slow into impact
                float ease = 1.0f - (1.0f - lp) * (1.0f - lp);
                float lungeDist = dirLen * 0.9f * ease;
                attackerPos->x = baseAttacker.x + dirX * lungeDist;
                attackerPos->z = baseAttacker.z + dirZ * lungeDist;
                attackerPos->y += 0.25f * sinf(lp * PI);
            }
            // Phase: Impact (50-70%) — contact + defender flinch
            if (animPhase >= 0.5f && animPhase < 0.7f) {
                float ip = (animPhase - 0.5f) / 0.2f;
                float retreat = ip * ip;
                attackerPos->x = baseAttacker.x + dirX * dirLen * 0.9f * (1.0f - retreat * 0.4f);
                attackerPos->z = baseAttacker.z + dirZ * dirLen * 0.9f * (1.0f - retreat * 0.4f);
                float flinch = ip < 0.35f ? ip / 0.35f : (1.0f - ip) / 0.65f;
                defenderPos->x += dirX * flinch * 0.8f;
                defenderPos->z += dirZ * flinch * 0.8f;
            }
            // Phase: Recovery (70-100%) — attacker slides back
            if (animPhase >= 0.7f) {
                float rp = (animPhase - 0.7f) / 0.3f;
                float easeBack = rp * rp;
                attackerPos->x = baseAttacker.x + dirX * dirLen * 0.9f * (0.6f - easeBack * 0.6f);
                attackerPos->z = baseAttacker.z + dirZ * dirLen * 0.9f * (0.6f - easeBack * 0.6f);
            }
        }

        if (b->activePetIdx >= 0 && b->activePetIdx < b->teamSize) {
            const Pet *pp = &b->playerTeam[b->activePetIdx];
            bool playerAttacking = isAttacking && b->animInfo.isPlayerAttack;
            DrawPetBattleModel(playerPetPos, pp->speciesId, 1.5f,
                               false, playerAttacking, animT, mType, animDur);
            DrawCylinderWires((Vector3){playerPetPos.x, 0.17f, playerPetPos.z},
                              0.7f, 0.7f, 0.02f, 16, (Color){100, 255, 100, 180});
        }

        // ---- Attack Effects ----
        if (isAttacking) {
            Vector3 fromPos, toPos;
            if(b->animInfo.isPlayerAttack){
                fromPos = (Vector3){playerPetPos.x, 0.8f, playerPetPos.z};
                toPos   = (Vector3){enemyPetPos.x, 0.8f, enemyPetPos.z};
            } else {
                fromPos = (Vector3){enemyPetPos.x, 0.8f, enemyPetPos.z};
                toPos   = (Vector3){playerPetPos.x, 0.8f, playerPetPos.z};
            }

            // Phase 1: Build-up + lunge projectiles (0-50%) — dash toward enemy
            if(animPhase < 0.5f){
                float lp = animPhase / 0.5f;
                int nProj = (int)(14 * powerScale);
                for(int i=0; i<nProj; i++){
                    float pp = fmodf(lp + (float)i/nProj, 1.0f);
                    float sx = fromPos.x + (toPos.x-fromPos.x)*pp + sinf(animT*22+i)*0.3f*(1-pp);
                    float sy = fromPos.y + (toPos.y-fromPos.y)*pp + cosf(animT*20+i)*0.25f*(1-pp);
                    float sz = fromPos.z + (toPos.z-fromPos.z)*pp + cosf(animT*22+i)*0.3f*(1-pp);
                    float life = 1-fabsf(pp*2-1);
                    DrawSphere((Vector3){sx,sy,sz}, 0.07f*life+0.03f, (Color){pCol.r,pCol.g,pCol.b,(unsigned char)(230*life)});
                    float tp = pp - 0.03f;
                    if(tp > 0){
                        float tx = fromPos.x + (toPos.x-fromPos.x)*tp + sinf(animT*22+i)*0.2f*(1-tp);
                        float ty = fromPos.y + (toPos.y-fromPos.y)*tp + cosf(animT*20+i)*0.15f*(1-tp);
                        float tz = fromPos.z + (toPos.z-fromPos.z)*tp + cosf(animT*22+i)*0.2f*(1-tp);
                        DrawSphere((Vector3){tx,ty,tz}, 0.03f*life, (Color){pCol2.r,pCol2.g,pCol2.b,(unsigned char)(100*life)});
                    }
                }
            }

            // Phase 2: Impact (50-70%) - burst + shock ring
            if(animPhase >= 0.5f && animPhase < 0.7f){
                float ip = (animPhase-0.5f)/0.2f;
                float iDecay = 1-ip;
                int nBurst = (int)(22 * powerScale);
                for(int i=0; i<nBurst; i++){
                    float ba = (float)i/nBurst*PI*2;
                    float be = (float)(i*73)*PI/180;
                    float br = ip*3.5f;
                    float bx = toPos.x + cosf(ba)*cosf(be)*br;
                    float by = toPos.y + sinf(be)*br*0.7f;
                    float bz = toPos.z + sinf(ba)*cosf(be)*br;
                    float bLife = ip < 0.1f ? ip/0.1f : iDecay/0.9f;
                    if (bLife > 1) bLife = 1;
                    if (bLife < 0) bLife = 0;
                    unsigned char ba2 = (unsigned char)(230*bLife);
                    DrawSphere((Vector3){bx,by,bz}, 0.05f*bLife+0.02f, (Color){pCol.r,pCol.g,pCol.b,ba2});
                    if(i%3==0) DrawSphere((Vector3){bx*1.1f,by*1.05f,bz*1.1f}, 0.03f*bLife, (Color){pCol2.r,pCol2.g,pCol2.b,ba2});
                }
                float srR = ip*3.0f;
                float srAlpha = iDecay*200;
                DrawCylinderWires((Vector3){toPos.x, toPos.y, toPos.z}, srR, srR, 0.01f, 20, (Color){pCol2.r,pCol2.g,pCol2.b,(unsigned char)srAlpha});
                if(srR > 0.2f) DrawCylinderWires((Vector3){toPos.x, toPos.y, toPos.z}, srR*0.7f, srR*0.7f, 0.01f, 16, (Color){pCol.r,pCol.g,pCol.b,(unsigned char)(srAlpha*0.6f)});
                float flashI = ip < 0.2f ? ip/0.2f : (1-ip)/0.8f;
                if(flashI > 0.01f) DrawSphere((Vector3){toPos.x, toPos.y+0.3f, toPos.z}, 0.65f, (Color){255,255,255,(unsigned char)(flashI*180)});
                int nSpark = (int)(12 * powerScale);
                for(int i=0; i<nSpark; i++){
                    float sa2 = (float)i/nSpark*PI*2 + animPhase;
                    float se = (float)(i*37)*PI/180;
                    float sd = ip*2.5f;
                    float sx = toPos.x + cosf(sa2)*cosf(se)*sd;
                    float sy = toPos.y + sinf(se)*sd*0.5f - ip*2.0f;
                    float sz = toPos.z + sinf(sa2)*cosf(se)*sd;
                    if(sy > 0.1f){
                        float sparkLife = iDecay;
                        unsigned char sA = (unsigned char)(180*sparkLife);
                        Color sparkCol = (i%3==0) ? (Color){255,255,200,sA} : (Color){pCol.r,pCol.g,pCol.b,sA};
                        DrawSphere((Vector3){sx,sy,sz}, 0.03f*sparkLife+0.015f, sparkCol);
                    }
                }
            }

            // Phase 3: Recovery (70-100%) - fading sparkles
            if(animPhase >= 0.7f){
                float rp = (animPhase-0.7f)/0.3f;
                float rDecay = 1-rp;
                int nRes = (int)(10 * powerScale);
                for(int i=0; i<nRes; i++){
                    float ra = rp*PI + (float)i/nRes*PI*2;
                    float rr = rp*2.5f;
                    float rx = toPos.x + cosf(ra)*rr;
                    float rz = toPos.z + sinf(ra)*rr;
                    float ry = toPos.y + rr*0.3f;
                    DrawSphere((Vector3){rx,ry,rz}, 0.03f*rDecay+0.01f, (Color){pCol2.r,pCol2.g,pCol2.b,(unsigned char)(120*rDecay)});
                }
            }
        }

        // Enemy pet
        {
            int eSpId = b->enemyPet.speciesId % PET_COUNT;
            bool enemyAttacking = isAttacking && !b->animInfo.isPlayerAttack;
            DrawPetBattleModel(enemyPetPos, eSpId, 1.5f,
                               true, enemyAttacking, animT, mType, animDur);

            // Enemy pet platform highlight
            DrawCylinderWires((Vector3){enemyPetPos.x, 0.17f, enemyPetPos.z},
                              0.7f, 0.7f, 0.02f, 16, (Color){255, 100, 100, 180});
            // Enemy attack indicator
            if(isAttacking && !b->animInfo.isPlayerAttack && animPhase < 0.5f){
                float ep = animPhase < 0.25f ? animPhase/0.25f : (0.5f-animPhase)/0.25f;
                if(ep < 0) ep = 0;
                float indicatorH = 0.28f + ep*0.5f;
                DrawCylinder((Vector3){enemyPetPos.x, enemyPetPos.y+1.3f, enemyPetPos.z},
                             0.03f, 0.03f, indicatorH, 4, (Color){255,50,50,(unsigned char)(200*ep)});
                DrawSphere((Vector3){enemyPetPos.x, enemyPetPos.y+1.3f+indicatorH, enemyPetPos.z},
                           0.05f*ep+0.03f, (Color){255,50,50,(unsigned char)(200*ep)});
            }
        }

    }
    EndMode3D();

    // Screen flash during attack
    if (flashAlpha > 0.01f) {
        DrawRectangle(0, 0, sw, sh, (Color){255, 255, 255, (unsigned char)(flashAlpha * 255)});
    }

    // ---- 2D UI Overlay ----

    // Enemy pet info panel
    {
        DrawRectangle(sw / 2 - 140, 30, 280, 100, (Color){0, 0, 0, 160});
        snprintf(buf, sizeof(buf), "%s  Lv.%d", b->enemyPet.nickname, b->enemyPet.level);
        DrawTextEx(g_uiFont, buf, (Vector2){(float)sw / 2 - 130, 38}, 22, 1, WHITE);
        snprintf(buf, sizeof(buf), "HP: %d/%d", b->enemyPet.currentHP, b->enemyPet.maxHP);
        DrawTextEx(g_uiFont, buf, (Vector2){(float)sw / 2 - 130, 64}, 16, 1, (Color){255, 100, 100, 255});

        DrawRectangle(sw / 2 - 130, 86, 260, 14, (Color){60, 60, 60, 255});
        float hpRatio = (float)b->enemyPet.currentHP / (float)b->enemyPet.maxHP;
        DrawRectangle(sw / 2 - 130, 86, (int)(260 * hpRatio), 14, (Color){220, 40, 40, 255});
    }

    // Player pet info panel
    if (b->activePetIdx < b->teamSize) {
        const Pet *pp = &b->playerTeam[b->activePetIdx];
        DrawRectangle(10, sh - 210, 280, 135, (Color){0, 0, 0, 160});

        // Name + Level
        snprintf(buf, sizeof(buf), "%s  Lv.%d", pp->nickname, pp->level);
        DrawTextEx(g_uiFont, buf, (Vector2){20, (float)sh - 202}, 22, 1, WHITE);

        // HP
        snprintf(buf, sizeof(buf), "HP: %d/%d", pp->currentHP, pp->maxHP);
        DrawTextEx(g_uiFont, buf, (Vector2){20, (float)sh - 176}, 14, 1, (Color){100, 255, 100, 255});
        DrawRectangle(20, sh - 158, 255, 10, (Color){40, 40, 40, 255});
        float hpRatio = (float)pp->currentHP / (float)pp->maxHP;
        DrawRectangle(20, sh - 158, (int)(255 * hpRatio), 10, (Color){50, 200, 50, 255});

        // Shared Energy
        snprintf(buf, sizeof(buf), "Energy: %d/%d", pp->sharedPP, pp->maxSharedPP);
        DrawTextEx(g_uiFont, buf, (Vector2){20, (float)sh - 144}, 14, 1, (Color){100, 180, 255, 255});
        DrawRectangle(20, sh - 126, 255, 10, (Color){40, 40, 40, 255});
        float ppRatio = (float)pp->sharedPP / (float)pp->maxSharedPP;
        Color ppColor = ppRatio > 0.5f ? (Color){60, 160, 240, 255}
                      : ppRatio > 0.25f ? (Color){200, 180, 40, 255}
                      : (Color){220, 60, 60, 255};
        DrawRectangle(20, sh - 126, (int)(255 * ppRatio), 10, ppColor);

        // HP number
        DrawRectangle(20, sh - 112, 255, 5, (Color){60, 60, 60, 255});
        float expRatio = (float)pp->currentExp / (float)pp->expToNext;
        DrawRectangle(20, sh - 112, (int)(255 * expRatio), 5, (Color){80, 120, 255, 255});
    }

    // Battle message
    if (b->message[0] != '\0') {
        Vector2 msgSz = MeasureTextEx(g_uiFont, b->message, 20, 1);
        DrawRectangle(sw / 2 - (int)msgSz.x / 2 - 18, sh - 56, (int)msgSz.x + 36, 44, (Color){0, 0, 0, 180});
        DrawTextEx(g_uiFont, b->message, (Vector2){(float)sw / 2 - msgSz.x / 2, (float)sh - 48}, 20, 1, WHITE);
    }

    // Battle result overlay
    if (b->battleOver) {
        DrawRectangle(0, sh / 2 - 50, sw, 100, (Color){0, 0, 0, 180});
        if (b->playerWon && !b->playerRan) {
            DrawTextEx(g_uiFont, "YOU WIN!", (Vector2){(float)sw / 2 - 80, (float)sh / 2 - 30}, 40, 1, (Color){255, 220, 50, 255});
            DrawTextEx(g_uiFont, "Auto-exiting...", (Vector2){(float)sw / 2 - 60, (float)sh / 2 + 20}, 16, 1, (Color){180, 180, 180, 255});
        } else if (!b->playerRan) {
            DrawTextEx(g_uiFont, "YOU LOSE...", (Vector2){(float)sw / 2 - 100, (float)sh / 2 - 30}, 40, 1, (Color){255, 80, 80, 255});
            DrawTextEx(g_uiFont, "Auto-exiting...", (Vector2){(float)sw / 2 - 60, (float)sh / 2 + 20}, 16, 1, (Color){180, 180, 180, 255});
        } else {
            DrawTextEx(g_uiFont, "GOT AWAY!", (Vector2){(float)sw / 2 - 80, (float)sh / 2 - 30}, 40, 1, (Color){180, 180, 220, 255});
            DrawTextEx(g_uiFont, "Auto-exiting...", (Vector2){(float)sw / 2 - 60, (float)sh / 2 + 20}, 16, 1, (Color){180, 180, 180, 255});
        }
    }
}

// ==================== Pet Bag UI ====================

int UpdatePetBagButtons(Pet *team, int teamSize, int activeIdx) {
    for (int i = 0; i < teamSize; i++) {
        int y = 60 + i * 95;
        // Select button (only for non-active, non-fainted pets)
        if (!IsFainted(&team[i]) && i != activeIdx) {
            Rectangle btnSelect = {20, (float)(y + 55), 85, 28};
            if (GuiButton(btnSelect, "出战")) return i;
        }
        // Release button (allow if teamSize > 1, even for fainted pets)
        if (teamSize > 1) {
            Rectangle btnRelease = {115, (float)(y + 55), 85, 28};
            if (GuiButton(btnRelease, i == activeIdx ? "放生(当前)" : "放生")) return 100 + i;
        }
    }
    return -1;
}

void DrawPetBagScene(const Pet *team, int teamSize, int activeIdx) {
    ClearBackground((Color){15, 25, 50, 255});

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "Pet Bag  [%d/6]", teamSize);
    DrawTextEx(g_uiFont, titleBuf, (Vector2){(float)sw / 2 - 70, 10}, 24, 1, (Color){255, 220, 50, 255});
    DrawTextEx(g_uiFont, "B:关闭  出战:设为先锋  放生:释放宠物", (Vector2){(float)sw / 2 - 150, 40}, 13, 1, (Color){180, 180, 200, 255});

    if (teamSize == 0) {
        DrawTextEx(g_uiFont, "No pets yet!", (Vector2){(float)sw / 2 - 60, (float)sh / 2 - 10}, 22, 1, WHITE);
        return;
    }

    int maxShow = teamSize < MAX_TEAM_SIZE ? teamSize : MAX_TEAM_SIZE;
    for (int i = 0; i < maxShow; i++) {
        const Pet *p = &team[i];
        int y = 60 + i * 95;

        Color bgColor = (i == activeIdx) ? (Color){40, 60, 100, 220} : (Color){20, 30, 60, 200};
        DrawRectangle(20, y, sw - 40, 85, bgColor);

        DrawRectangle(30, y + 8, 28, 28, PET_DATABASE[p->speciesId].color);

        char pbuf[128];
        snprintf(pbuf, sizeof(pbuf), "%s  Lv.%d  %s", p->nickname, p->level,
                 PET_DATABASE[p->speciesId].name);
        DrawTextEx(g_uiFont, pbuf, (Vector2){68, (float)y + 6}, 18, 1, WHITE);

        snprintf(pbuf, sizeof(pbuf), "HP: %d/%d  ATK: %d  DEF: %d  SPD: %d",
                 p->currentHP, p->maxHP, p->attack, p->defense, p->speed);
        DrawTextEx(g_uiFont, pbuf, (Vector2){68, (float)y + 28}, 14, 1, (Color){200, 200, 220, 255});

        DrawRectangle(68, y + 48, 150, 8, (Color){60, 60, 60, 255});
        float hpRatio = (float)p->currentHP / (float)p->maxHP;
        DrawRectangle(68, y + 48, (int)(150 * hpRatio), 8,
                      IsFainted(p) ? (Color){150, 50, 50, 255} : (Color){50, 200, 50, 255});

        if (IsFainted(p)) {
            DrawTextEx(g_uiFont, "FAINTED", (Vector2){230, (float)y + 44}, 14, 1, (Color){255, 80, 80, 255});
        }

        DrawRectangle(68, y + 62, 150, 5, (Color){50, 50, 50, 255});
        float expRatio = (float)p->currentExp / (float)p->expToNext;
        DrawRectangle(68, y + 62, (int)(150 * expRatio), 5, (Color){80, 120, 255, 255});

        if (i == activeIdx) {
            DrawTextEx(g_uiFont, "IN BATTLE", (Vector2){(float)sw - 120, (float)y + 30}, 16, 1, (Color){255, 220, 50, 255});
        }
    }
}

// ==================== Game Over ====================

int UpdateGameOverButtons(void) {
    if (IsKeyPressed(KEY_ENTER)) return 1;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Rectangle btn = {(float)sw / 2 - 100, (float)sh / 2 + 50, 200, 45};
    if (GuiButton(btn, "Back to Menu")) return 1;
    return 0;
}

void DrawGameOverScene(bool playerWon) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawFullscreenBG();

    if (playerWon) {
        DrawTextEx(g_uiFont, "VICTORY!", (Vector2){(float)sw / 2 - 100, (float)sh / 2 - 40}, 45, 1, (Color){255, 220, 50, 255});
    } else {
        DrawTextEx(g_uiFont, "GAME OVER", (Vector2){(float)sw / 2 - 130, (float)sh / 2 - 40}, 45, 1, (Color){255, 80, 80, 255});
    }
    DrawTextEx(g_uiFont, "Press ENTER to return to menu", (Vector2){(float)sw / 2 - 140, (float)sh / 2 + 20}, 18, 1, WHITE);
}

// ==================== NPC Dialog ====================

void DrawNPCDialogBox(const char *name, const char *dialog) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, sh - 160, sw, 160, (Color){0, 0, 0, 210});
    DrawRectangle(0, sh - 160, sw, 3, (Color){255, 220, 50, 255});

    DrawTextEx(g_uiFont, name, (Vector2){24, (float)sh - 148}, 34, 1, (Color){255, 220, 50, 255});
    DrawTextEx(g_uiFont, dialog, (Vector2){24, (float)sh - 106}, 26, 1, WHITE);
    DrawTextEx(g_uiFont, "[E] 继续", (Vector2){(float)sw - 110, (float)sh - 30}, 16, 1, (Color){150, 150, 170, 255});
}

// ==================== Notification Toast ====================

void ShowNotification(Notification *n, const char *msg) {
    snprintf(n->message, NOTIFY_MSG_LEN, "%s", msg);
    n->timer = NOTIFY_DURATION;
    n->active = true;
}

void DrawNotification(const Notification *n) {
    if (!n->active) return;
    int sw = GetScreenWidth();
    float alpha = n->timer < 0.5f ? n->timer / 0.5f : 1.0f;
    int textW = MeasureTextEx(g_uiFont, n->message, 22, 1).x;
    int boxW = textW + 40;
    int boxH = 40;
    int bx = (sw - boxW) / 2;
    int by = 60;
    Color bg = {0, 0, 0, (unsigned char)(200 * alpha)};
    Color fg = {255, 220, 50, (unsigned char)(255 * alpha)};
    DrawRectangle(bx, by, boxW, boxH, bg);
    DrawRectangleLines(bx, by, boxW, boxH, (Color){255, 220, 50, (unsigned char)(180 * alpha)});
    DrawTextEx(g_uiFont, n->message, (Vector2){(float)bx + 20, (float)by + 10}, 22, 1, fg);
}

// ==================== Shop UI ====================

int DrawShopUI(int shopType, Inventory *inv, const MerchantShop *merchant) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int action = -1;

    // Fullscreen dark overlay
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});

    float scl = sh / 720.0f;
    int panelW = (int)(500 * scl);
    int panelH = (int)((shopType == 0 ? 400.0f : 300.0f) * scl);
    int panelX = (sw - panelW) / 2;
    int panelY = (sh - panelH) / 2;

    DrawRectangle(panelX - 4, panelY - 4, panelW + 8, panelH + 8, (Color){255, 200, 50, 60});
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){10, 15, 35, 245});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){255, 200, 50, 180});

    const char *title = (shopType == 0) ? "护士商店" : "行商";
    DrawTextEx(g_uiFont, title, (Vector2){(float)panelX + 15, (float)panelY + 12}, 24, 1, (Color){255, 220, 50, 255});

    char goldLabel[32];
    snprintf(goldLabel, sizeof(goldLabel), "金币: %d", inv->gold);
    DrawTextEx(g_uiFont, goldLabel, (Vector2){(float)panelX + panelW - 130, (float)panelY + 16}, 18, 1, (Color){255, 220, 50, 255});

    int itemY = panelY + 50;
    int btnW = panelW - 30, btnH = 44;

    if (shopType == 0) {
        // Nurse shop: fixed items
        for (int i = 0; i < NURSE_SHOP_SIZE; i++) {
            Rectangle btn = {(float)panelX + 15, (float)itemY + i * (btnH + 8), (float)btnW, (float)btnH};
            char label[80];
            snprintf(label, sizeof(label), "%s    %d G    %s", NURSE_SHOP[i].name, NURSE_SHOP[i].price, NURSE_SHOP[i].desc);
            if (inv->gold >= NURSE_SHOP[i].price && GuiButton(btn, label)) action = i;
            else if (inv->gold < NURSE_SHOP[i].price) {
                GuiDisable();
                GuiButton(btn, label);
                GuiEnable();
            }
        }
    } else if (merchant) {
        // Merchant: skill packs
        const char *shortDesc[3] = {
            "随机技能 +3次使用",
            "随机技能 +2次使用",
            "随机技能 +1次使用"
        };
        for (int i = 0; i < MERCHANT_SIZE; i++) {
            if (!merchant->packs[i].name) continue;
            Rectangle btn = {(float)panelX + 15, (float)itemY + i * (btnH + 8), (float)btnW, (float)btnH};
            char label[80];
            int di = (merchant->packs[i].type == SHOP_PACK_SMALL) ? 0 :
                     (merchant->packs[i].type == SHOP_PACK_MEDIUM) ? 1 : 2;
            snprintf(label, sizeof(label), "%s    %d G    %s",
                     merchant->packs[i].name, merchant->packs[i].price, shortDesc[di]);
            if (inv->gold >= merchant->packs[i].price && GuiButton(btn, label)) action = 100 + i;
            else if (inv->gold < merchant->packs[i].price) {
                GuiDisable();
                GuiButton(btn, label);
                GuiEnable();
            }
        }
    }

    DrawTextEx(g_uiFont, "点击购买  |  E键关闭",
               (Vector2){(float)panelX + 15, (float)panelY + panelH - 28}, 15, 1, (Color){150, 150, 170, 200});

    return action;
}

// ==================== Bestiary UI ====================

static int g_bestiaryTab = 0;
static int g_bestiaryScroll = 0;
static int g_selectedPet = -1;   // -1 = list view, 0-7 = detail view
static int g_selectedNpc = -1;   // -1 = list view, 0-3 = detail view

static const char *PET_TYPE_NAMES[] = {"普通", "火", "水", "草", "光", "暗"};
static const char *TAB_NAMES[] = {"宠物", "道具", "NPC", "机制"};

// Pet flavor text from BESTIARY.md
static const char *PET_FLAVOR[] = {
    "王国最常见的初始伙伴。性格急躁，速度快，适合抢先秒杀。",
    "通体蓝色，性格温和。泡沫光线命中率高且有机率降低对手速度。",
    "外形像猫的草系精灵，动作敏捷。擅长用毒刺和泥巴炸弹削弱对手。",
    "整天打瞌睡的大块头。血量极高，不受任何属性克制。配合回复技能可持久站场。",
    "头顶巨大花朵，能聚集阳光发射强力光束。阳光烈焰是游戏中最强草系招式。",
    "传说中居住在雪山之巅的稀有精灵。速度极快，寒冰刺有10%机率冰冻对手。",
    "看似憨厚的小家伙，生气时背部会冒出火焰。比火花更耐打，自带回复技能。",
    "行踪飘忽的暗影精灵。影袭是先手技，无视速度抢先攻击。防御较低需注意保护。",
};

void DrawBestiaryScene(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawFullscreenBG();
    float scl = sh / 720.0f;

    // Tab bar
    int tabW = (int)(100 * scl), tabH = (int)(36 * scl);
    int tabStartX = 20, tabY = 10;
    int nTabs = 4;
    for (int i = 0; i < nTabs; i++) {
        Rectangle tabBtn = {(float)tabStartX + i * (tabW + 4), (float)tabY, (float)tabW, (float)tabH};
        Color tabBg = (i == g_bestiaryTab) ? (Color){60, 80, 140, 255} : (Color){30, 40, 80, 255};
        DrawRectangleRec(tabBtn, tabBg);
        DrawRectangleLines(tabBtn.x, tabBtn.y, tabBtn.width, tabBtn.height,
                          (i == g_bestiaryTab) ? (Color){255, 220, 50, 255} : (Color){100, 110, 150, 255});
        DrawTextEx(g_uiFont, TAB_NAMES[i],
                   (Vector2){tabBtn.x + 12, tabBtn.y + 8}, 18 * scl, 1,
                   (i == g_bestiaryTab) ? (Color){255, 220, 50, 255} : (Color){180, 190, 210, 255});
    }

    // Content area
    int contentY = (int)(tabY + tabH + 12);
    int contentH = sh - contentY - 60;
    DrawRectangle(10, contentY, sw - 20, contentH, (Color){0, 0, 0, 140});

    int textY = contentY + 8;
    int fontSize = (int)(24 * scl);
    int scrollSpeed = (int)(28 * scl);

    // Pet detail view (when a pet is selected)
    if (g_bestiaryTab == 0 && g_selectedPet >= 0 && g_selectedPet < (int)PET_COUNT) {
        const PetSpecies *sp = &PET_DATABASE[g_selectedPet];
        int scroll = g_bestiaryScroll * scrollSpeed;
        int cx = 20, cy = contentY + 5 - scroll;
        // Back button at top (fixed, doesn't scroll)
        DrawRectangle(10, contentY, sw - 20, 35, (Color){30, 40, 80, 220});
        DrawTextEx(g_uiFont, "[返回] 点击此处返回宠物列表",
                   (Vector2){(float)cx, (float)contentY + 5}, fontSize, 1, (Color){150, 150, 170, 255});

        int detailTop = contentY + 40;
        cy = detailTop - scroll;

        // Pet picture on the right
        int picW = (int)(200 * scl);
        int picX = sw - 30 - picW;
        int picY = detailTop;
        if (g_petTexLoaded[g_selectedPet]) {
            Texture2D tex = g_petTextures[g_selectedPet];
            float ratio = (float)tex.width / tex.height;
            int picH = (int)(picW / ratio);
            DrawTextureEx(tex, (Vector2){(float)picX, (float)picY}, 0, (float)picW / tex.width, WHITE);
            DrawRectangleLines(picX, picY, picW, picH, (Color){255, 220, 100, 120});
        }

        // Color swatch + name + type
        DrawRectangle(cx, cy, 28, 28, sp->color);
        DrawRectangleLines(cx, cy, 28, 28, WHITE);
        char header[128];
        snprintf(header, sizeof(header), "%s  [%s]", sp->name, PET_TYPE_NAMES[sp->type]);
        DrawTextEx(g_uiFont, header, (Vector2){(float)cx + 36, (float)cy + 2}, fontSize + 4, 1, (Color){255, 220, 50, 255});
        cy += 40;

        // Flavor text
        DrawTextEx(g_uiFont, PET_FLAVOR[g_selectedPet], (Vector2){(float)cx + 8, (float)cy}, fontSize - 1, 1, (Color){200, 210, 230, 255});
        cy += fontSize + 14;

        // Stats
        char statLine[128];
        snprintf(statLine, sizeof(statLine), "HP:%d  ATK:%d  DEF:%d  SPD:%d  种族:%d",
                 sp->baseHP, sp->baseAttack, sp->baseDefense, sp->baseSpeed,
                 sp->baseHP + sp->baseAttack + sp->baseDefense + sp->baseSpeed);
        DrawTextEx(g_uiFont, statLine, (Vector2){(float)cx + 8, (float)cy}, fontSize, 1, (Color){100, 255, 150, 255});
        cy += fontSize + 10;

        // Lv5 preview
        char formLine[128];
        snprintf(formLine, sizeof(formLine), "Lv5: HP=%d ATK=%d DEF=%d SPD=%d Energy=%d",
                 sp->baseHP * 3 + 5 * 2, sp->baseAttack * 5 / 20 + 8,
                 sp->baseDefense * 5 / 20 + 8, sp->baseSpeed * 5 / 20 + 8, 150 + 5 * 5);
        DrawTextEx(g_uiFont, formLine, (Vector2){(float)cx + 8, (float)cy}, fontSize - 1, 1, (Color){180, 190, 210, 255});
        cy += fontSize + 16;

        // Skills header
        DrawTextEx(g_uiFont, "可学技能:", (Vector2){(float)cx + 8, (float)cy}, fontSize + 2, 1, (Color){255, 200, 50, 255});
        cy += fontSize + 10;
        for (int m = 0; m < 6 && sp->learnableMoves[m] >= 0; m++) {
            const MoveData *mv = &MOVE_DATABASE[sp->learnableMoves[m]];
            // Line 1: name + stats + flags
            char moveLine[256];
            const char *flags = "";
            if (mv->flags & MOVE_FLAG_PRIORITY) flags = "  [先手]";
            else if (mv->flags & MOVE_FLAG_HIGH_CRIT) flags = "  [高暴击]";
            else if (mv->flags & MOVE_FLAG_HEAL) flags = "  [回复]";
            snprintf(moveLine, sizeof(moveLine), "%s  威力:%d  命中:%d  Energy:%d%s",
                     mv->name, mv->basePower, mv->accuracy, mv->ppCost, flags);
            DrawTextEx(g_uiFont, moveLine, (Vector2){(float)cx + 16, (float)cy}, fontSize - 1, 1, (Color){180, 210, 255, 255});
            cy += fontSize + 6;
            // Line 2: description (indented)
            DrawTextEx(g_uiFont, mv->description, (Vector2){(float)cx + 28, (float)cy}, fontSize - 2, 1, (Color){140, 160, 200, 255});
            cy += fontSize + 2;
            // Line 3: status chance (if any)
            if (mv->statusChance > 0) {
                char statusLine[64];
                const char *stName = mv->statusType == 1 ? "灼伤" : mv->statusType == 2 ? "中毒" : "冰冻";
                snprintf(statusLine, sizeof(statusLine), "%d%% 几率%s", mv->statusChance, stName);
                DrawTextEx(g_uiFont, statusLine, (Vector2){(float)cx + 28, (float)cy}, fontSize - 2, 1, (Color){255, 180, 100, 255});
                cy += fontSize + 4;
            }
            cy += fontSize + 2;  // gap between skills
        }
        // Clamp scroll if content fits
        int maxScroll = (cy + scroll) - (contentY + contentH);
        if (maxScroll < 0) maxScroll = 0;
        int maxSteps = maxScroll / scrollSpeed + 1;
        if (g_bestiaryScroll > maxSteps) g_bestiaryScroll = maxSteps;
        if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;
        // Back hint at bottom
        DrawTextEx(g_uiFont, "滚轮滚动  点击顶部返回",
                   (Vector2){(float)sw - 260, (float)sh - 35}, 14, 1, (Color){150, 150, 170, 255});
        return;
    }

    switch (g_bestiaryTab) {
    case 0: { // 宠物 - clickable cards
        int cardW = (int)(sw - 40);
        int cardH = (int)(90 * scl);
        int cardGap = (int)(8 * scl);
        int totalItems = (int)PET_COUNT;
        int scrollPx = (int)(scrollSpeed);
        int maxScroll = totalItems * (cardH + cardGap) - contentH;
        if (maxScroll < 0) maxScroll = 0;
        int maxScrollSteps = maxScroll / scrollPx;
        if (g_bestiaryScroll > maxScrollSteps) g_bestiaryScroll = maxScrollSteps;
        if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;

        for (int i = 0; i < (int)PET_COUNT; i++) {
            int y = textY + i * (cardH + cardGap) - g_bestiaryScroll * scrollPx;
            if (y < contentY - cardH || y > contentY + contentH) continue;
            const PetSpecies *sp = &PET_DATABASE[i];

            // Card background
            Color cardBg = (Color){25, 35, 60, 255};
            DrawRectangle(15, y, cardW, cardH, cardBg);
            DrawRectangleLines(15, y, cardW, cardH, (Color){60, 75, 110, 255});

            // Color swatch
            DrawRectangle(25, y + 8, 22, 22, sp->color);
            DrawRectangleLines(25, y + 8, 22, 22, WHITE);

            // Name + type
            char nameLine[64];
            snprintf(nameLine, sizeof(nameLine), "%s  [%s]", sp->name, PET_TYPE_NAMES[sp->type]);
            DrawTextEx(g_uiFont, nameLine, (Vector2){55, (float)y + 6}, fontSize + 4, 1, (Color){255, 220, 100, 255});

            // Base stats
            char statLine[128];
            snprintf(statLine, sizeof(statLine), "HP:%d  ATK:%d  DEF:%d  SPD:%d  总和:%d",
                     sp->baseHP, sp->baseAttack, sp->baseDefense, sp->baseSpeed,
                     sp->baseHP + sp->baseAttack + sp->baseDefense + sp->baseSpeed);
            DrawTextEx(g_uiFont, statLine, (Vector2){55, (float)y + fontSize + 10}, fontSize - 1, 1, (Color){100, 255, 150, 255});

            // Flavor preview
            DrawTextEx(g_uiFont, PET_FLAVOR[i], (Vector2){55, (float)y + fontSize * 2 + 8}, fontSize - 2, 1, (Color){160, 170, 190, 255});
        }
        break;
    }
    case 1: { // 道具
        const char *itemNames[] = {"治疗药水", "超级药水", "能量剂", "宠物球", "高级球", "复活令牌", "金币"};
        const char *itemDescs[] = {
            "回复 20% MaxHP（最少 60）  —  价格: 150 G  (护士商店)",
            "回复 40% MaxHP（最少 120）  —  价格: 400 G  (护士商店)",
            "回复 30% Energy（最少 30）  —  价格: 300 G  (护士商店)",
            "捕捉野生宠物  —  价格: 200 G  (护士商店)",
            "更高捕捉率（+25%）  —  价格: 600 G  (护士商店)",
            "全队阵亡时选择一只宠物满血复活  —  价格: 800 G  (护士商店)",
            "游戏货币，通过战斗、完成任务、击败训练师获得"
        };
        for (int i = 0; i < 7; i++) {
            int y = textY + i * (fontSize * 2 + 24);
            DrawTextEx(g_uiFont, itemNames[i], (Vector2){15, (float)y}, fontSize + 2, 1, (Color){255, 220, 100, 255});
            DrawTextEx(g_uiFont, itemDescs[i], (Vector2){25, (float)y + fontSize + 6}, fontSize - 2, 1, (Color){180, 190, 210, 255});
        }
        break;
    }
    case 2: { // NPC — interactive characters only
        // Only show interactive NPCs (no trainers) + merchant
        // Order: nurse, merchant, old man, researcher
        static const char *npcNames[4] = {"护士", "行商", "老爷爷", "研究员"};
        static const char *npcRoles[4] = {"宠物中心护士", "游走商人", "退休训练师", "宠物研究员"};
        static const char *npcDescs[4] = {
            "宠物中心的护士小姐，温柔善良。提供商店服务，出售药水、宠物球和复活令牌。",
            "游走各地的神秘商人，每隔5分钟出现在新位置。出售随机技能包，价格0.6x~1.0x浮动。",
            "喜欢在路边休息的慈祥老人。可接取击败训练师的任务，初次见面赠送能量剂。",
            "宠物研究所的研究员。可接取捕捉野生宠物的任务，初次见面赠送治疗药水x2。",
        };
        static int npcPicIdx[4] = {CHAR_MODEL_NURSE, CHAR_MODEL_MERCHANT, CHAR_MODEL_OLD, CHAR_MODEL_RESEARCHER};
        int nNpcs = 4;

        // NPC detail view
        if (g_selectedNpc >= 0 && g_selectedNpc < nNpcs) {
            int scroll = g_bestiaryScroll * scrollSpeed;
            int cx = 20, cy = contentY + 5 - scroll;
            // Back button (fixed)
            DrawRectangle(10, contentY, sw - 20, 35, (Color){30, 40, 80, 220});
            DrawTextEx(g_uiFont, "[返回] 点击此处返回NPC列表",
                       (Vector2){(float)cx, (float)contentY + 5}, fontSize, 1, (Color){150, 150, 170, 255});

            int detailTop = contentY + 40;
            cy = detailTop - scroll;

            // Large photo on the right
            int picW = (int)(220 * scl);
            int picX = sw - 30 - picW;
            int picY = detailTop;
            int picIdx = npcPicIdx[g_selectedNpc];
            if (picIdx >= 0 && g_charTexLoaded[picIdx]) {
                Texture2D tex = g_charTextures[picIdx];
                float ratio = (float)tex.width / tex.height;
                int picH = (int)(picW / ratio);
                DrawTextureEx(tex, (Vector2){(float)picX, (float)picY}, 0, (float)picW / tex.width, WHITE);
                DrawRectangleLines(picX, picY, picW, picH, (Color){255, 220, 100, 120});
            }

            // Name + role
            char header[128];
            snprintf(header, sizeof(header), "%s — %s", npcNames[g_selectedNpc], npcRoles[g_selectedNpc]);
            DrawTextEx(g_uiFont, header, (Vector2){(float)cx + 4, (float)cy}, fontSize + 4, 1, (Color){255, 220, 50, 255});
            cy += 40;

            // Description
            DrawTextEx(g_uiFont, npcDescs[g_selectedNpc], (Vector2){(float)cx + 8, (float)cy}, fontSize - 1, 1, (Color){200, 210, 230, 255});
            cy += fontSize + 14;

            // Get extra info from NPC data for fixed NPCs
            NPC *npcs = GetNPCs();
            for (int i = 0; i < GetNPCCount(); i++) {
                if (strcmp(npcs[i].name, npcNames[g_selectedNpc]) == 0) {
                    // Quest info
                    if (npcs[i].hasQuest) {
                        char qLine[128];
                        snprintf(qLine, sizeof(qLine), "任务: %s (%d次)  奖励: %d G",
                                 npcs[i].questType == 0 ? "捕捉野生宠物" : "击败训练师",
                                 npcs[i].questTarget, npcs[i].questReward);
                        DrawTextEx(g_uiFont, qLine, (Vector2){(float)cx + 8, (float)cy}, fontSize, 1, (Color){100, 255, 200, 255});
                        cy += fontSize + 8;
                    }
                    // Gift info
                    if (npcs[i].giftItemType > 0) {
                        const char *giftNames[] = {"", "治疗药水", "超级药水", "能量剂", "宠物球", "金币"};
                        char gLine[128];
                        snprintf(gLine, sizeof(gLine), "首次对话赠送: %s x%d",
                                 giftNames[npcs[i].giftItemType], npcs[i].giftCount);
                        DrawTextEx(g_uiFont, gLine, (Vector2){(float)cx + 8, (float)cy}, fontSize, 1, (Color){255, 200, 150, 255});
                        cy += fontSize + 8;
                    }
                    // Dialog
                    DrawTextEx(g_uiFont, "对话:", (Vector2){(float)cx + 8, (float)cy}, fontSize, 1, (Color){200, 200, 200, 255});
                    cy += fontSize + 4;
                    DrawTextEx(g_uiFont, npcs[i].dialog, (Vector2){(float)cx + 16, (float)cy}, fontSize - 1, 1, (Color){160, 170, 190, 255});
                    cy += fontSize + 4;
                    // Multi-page dialogs
                    for (int d = 0; d < npcs[i].dialogCount && npcs[i].dialogs[d][0]; d++) {
                        DrawTextEx(g_uiFont, npcs[i].dialogs[d], (Vector2){(float)cx + 16, (float)cy}, fontSize - 1, 1, (Color){140, 150, 170, 255});
                        cy += fontSize + 4;
                    }
                    break;
                }
            }
            // Merchant extra info
            if (g_selectedNpc == 1) {
                DrawTextEx(g_uiFont, "功能:", (Vector2){(float)cx + 8, (float)cy}, fontSize, 1, (Color){200, 200, 200, 255});
                cy += fontSize + 4;
                DrawTextEx(g_uiFont, "出售随机技能包（初级/中级/高级），价格浮动0.6x~1.0x", (Vector2){(float)cx + 16, (float)cy}, fontSize - 1, 1, (Color){160, 170, 190, 255});
                cy += fontSize + 4;
                DrawTextEx(g_uiFont, "靠近后按E键打开商店，每隔5分钟移动到新位置", (Vector2){(float)cx + 16, (float)cy}, fontSize - 1, 1, (Color){160, 170, 190, 255});
                cy += fontSize + 12;
            }

            // Clamp scroll
            int maxScroll = (cy + scroll) - (contentY + contentH);
            if (maxScroll < 0) maxScroll = 0;
            int maxSteps = maxScroll / scrollSpeed + 1;
            if (g_bestiaryScroll > maxSteps) g_bestiaryScroll = maxSteps;
            if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;
            DrawTextEx(g_uiFont, "滚轮滚动  点击顶部返回",
                       (Vector2){(float)sw - 260, (float)sh - 35}, 14, 1, (Color){150, 150, 170, 255});
            return;
        }

        // NPC list — clickable cards
        int cardW = (int)(sw - 40);
        int cardH = (int)(90 * scl);
        int cardGap = (int)(8 * scl);
        int maxScroll = nNpcs * (cardH + cardGap) - contentH;
        if (maxScroll < 0) maxScroll = 0;
        int scrollPx = (int)scrollSpeed;
        int maxScrollSteps = maxScroll / scrollPx;
        if (g_bestiaryScroll > maxScrollSteps) g_bestiaryScroll = maxScrollSteps;
        if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;

        for (int i = 0; i < nNpcs; i++) {
            int y = textY + i * (cardH + cardGap) - g_bestiaryScroll * scrollPx;
            if (y < contentY - cardH || y > contentY + contentH) continue;

            // Card background
            DrawRectangle(15, y, cardW, cardH, (Color){25, 35, 60, 255});
            DrawRectangleLines(15, y, cardW, cardH, (Color){60, 75, 110, 255});

            // Name + role
            char nameLine[64];
            snprintf(nameLine, sizeof(nameLine), "%s — %s", npcNames[i], npcRoles[i]);
            DrawTextEx(g_uiFont, nameLine, (Vector2){25, (float)y + 8}, fontSize + 2, 1, (Color){255, 220, 100, 255});

            // Brief description
            DrawTextEx(g_uiFont, npcDescs[i], (Vector2){35, (float)y + fontSize + 14}, fontSize - 2, 1, (Color){160, 170, 190, 255});
        }
        break;
    }
    case 3: { // 机制
        const char *mechLines[] = {
            "=== 属性克制 ===",
            "火→草 1.6x  火→水 0.6x  水→火 1.6x  水→草 0.6x  草→水 1.6x  草→火 0.6x",
            "光/暗互克 1.6x  本系自克 0.6x  普通系无克制",
            "",
            "=== 伤害公式 ===",
            "伤害 = ((2*等级/5+2) * 威力 * 攻击/防御 / 15 + 2) * 属性克制 * 随机(0.85~1.0) * 暴击(1.5x)",
            "",
            "=== 属性公式 ===",
            "MaxHP = 种族HP*3 + 等级*2",
            "攻击/防御/速度 = 种族值*等级/20 + 8",
            "Energy = 150 + 等级*5",
            "经验升级 = 等级^2 * 30",
            "获得经验 = 20 + 敌人等级*10 + (敌人-我方等级差)*12",
            "",
            "=== 状态效果 ===",
            "灼伤: 每回合扣 1/16 HP, 普通系伤害*0.8",
            "中毒: 每回合扣 1/16 HP",
            "冰冻: 每回合 25% 解冻, 否则无法行动",
            "减速: 速度降至 70%",
            "",
            "=== 出手顺序 ===",
            "1. 先手技能 (影袭) 永远最先",
            "2. 速度高者先出招, 相同时玩家先出",
            "",
            "=== 暴击 ===",
            "基础暴击率 ~6% (1/16), 高暴击技能 ~25% (4/16)",
            "",
            "=== 共享Energy ===",
            "初始 150 + 每级 5 点, 所有技能共用, 优先消耗 Energy 再消耗技能包",
            "",
            "=== 技能包 ===",
            "初级包 +3次 (40-60威力) 300G | 中级包 +2次 (65-85威力) 600G | 高级包 +1次 (90-120威力) 1200G",
        };
        int nLines = sizeof(mechLines) / sizeof(mechLines[0]);
        for (int i = 0; i < nLines; i++) {
            int y = textY + i * (fontSize + 8) - g_bestiaryScroll * 3;
            if (y < contentY - 20 || y > contentY + contentH) continue;
            Color mc = (mechLines[i][0] == '=') ? (Color){255, 200, 50, 255} : (Color){200, 210, 230, 255};
            DrawTextEx(g_uiFont, mechLines[i], (Vector2){15, (float)y}, fontSize - 1, 1, mc);
        }
        break;
    }
    }

    DrawTextEx(g_uiFont, "[ESC] 返回",
               (Vector2){(float)sw - 140, (float)sh - 35}, 16, 1, (Color){150, 150, 170, 255});
}

int UpdateBestiaryButtons(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scl = sh / 720.0f;
    int tabW = (int)(100 * scl), tabH = (int)(36 * scl);
    int tabStartX = 20, tabY = 10;

    // In pet/NPC detail view, handle back navigation + scroll wheel
    if (g_selectedPet >= 0 || g_selectedNpc >= 0) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            g_bestiaryScroll -= (int)(wheel * 3);
            if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;
        }
        Rectangle backArea = {10, (float)(tabY + tabH + 12), sw - 20, 30};
        if (CheckCollisionPointRec(GetMousePosition(), backArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_selectedPet = -1;
            g_selectedNpc = -1;
            g_bestiaryScroll = 0;
            return 0;
        }
        return 0;
    }

    // Tab clicks
    int nTabs = 4;
    for (int i = 0; i < nTabs; i++) {
        Rectangle tabBtn = {(float)tabStartX + i * (tabW + 4), (float)tabY, (float)tabW, (float)tabH};
        if (CheckCollisionPointRec(GetMousePosition(), tabBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_bestiaryTab = i;
            g_bestiaryScroll = 0;
            g_selectedPet = -1;
            g_selectedNpc = -1;
        }
    }

    // Scroll wheel (all tabs)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        g_bestiaryScroll -= (int)(wheel * 3);
        if (g_bestiaryScroll < 0) g_bestiaryScroll = 0;
    }

    // Keyboard tab navigation (chained with else if for IsKeyPressed safety)
    int key = 0;
    if (IsKeyPressed(KEY_LEFT)) key = -1;
    else if (IsKeyPressed(KEY_RIGHT)) key = 1;
    else if (IsKeyPressed(KEY_UP)) { g_bestiaryScroll--; if (g_bestiaryScroll < 0) g_bestiaryScroll = 0; }
    else if (IsKeyPressed(KEY_DOWN)) g_bestiaryScroll++;

    if (key == -1 && g_bestiaryTab > 0) { g_bestiaryTab--; g_bestiaryScroll = 0; g_selectedPet = -1; g_selectedNpc = -1; }
    if (key == 1 && g_bestiaryTab < nTabs - 1) { g_bestiaryTab++; g_bestiaryScroll = 0; g_selectedPet = -1; g_selectedNpc = -1; }

    // Pet card clicks (tab 0, list view)
    if (g_bestiaryTab == 0 && g_selectedPet < 0) {
        int cardH = (int)(90 * scl);
        int cardGap = (int)(8 * scl);
        int contentY = (int)(tabY + tabH + 12);
        int textY = contentY + 8;
        for (int i = 0; i < (int)PET_COUNT; i++) {
            int y = textY + i * (cardH + cardGap) - g_bestiaryScroll * (int)(28 * scl);
            Rectangle card = {15, (float)y, sw - 40, (float)cardH};
            if (CheckCollisionPointRec(GetMousePosition(), card) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g_selectedPet = i;
                g_bestiaryScroll = 0;
                return 0;
            }
        }
    }

    // NPC card clicks (tab 2, list view)
    if (g_bestiaryTab == 2 && g_selectedNpc < 0) {
        int cardH = (int)(90 * scl);
        int cardGap = (int)(8 * scl);
        int contentY = (int)(tabY + tabH + 12);
        int textY = contentY + 8;
        for (int i = 0; i < 4; i++) {
            int y = textY + i * (cardH + cardGap) - g_bestiaryScroll * (int)(28 * scl);
            Rectangle card = {15, (float)y, sw - 40, (float)cardH};
            if (CheckCollisionPointRec(GetMousePosition(), card) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g_selectedNpc = i;
                g_bestiaryScroll = 0;
                return 0;
            }
        }
    }

    return 0;
}

// ==================== Starter Selection UI ====================

void DrawStarterSelectScene(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawFullscreenBG();
    float scl = sh / 720.0f;

    const char *title = "选择你的初始伙伴";
    int titleSize = (int)(36 * scl);
    Vector2 titleSz = MeasureTextEx(g_uiFont, title, titleSize, 1);
    DrawTextEx(g_uiFont, title,
               (Vector2){(float)(sw - titleSz.x) / 2, 30 * scl},
               titleSize, 1, (Color){255, 220, 50, 255});

    const char *sub = "点击宠物卡片选择，初始等级为 5";
    int subSize = (int)(16 * scl);
    Vector2 subSz = MeasureTextEx(g_uiFont, sub, subSize, 1);
    DrawTextEx(g_uiFont, sub,
               (Vector2){(float)(sw - subSz.x) / 2, 30 * scl + titleSize + 8},
               subSize, 1, (Color){180, 190, 210, 255});

    int cardW = (int)((sw - 60) / 4);
    int cardH = (int)(140 * scl);
    int startX = 15, startY = (int)(30 * scl + titleSize + subSize + 20);
    int gap = 10;

    for (int i = 0; i < (int)PET_COUNT; i++) {
        int col = i % 4;
        int row = i / 4;
        int cx = startX + col * (cardW + gap);
        int cy = startY + row * (cardH + gap);
        const PetSpecies *sp = &PET_DATABASE[i];

        Color cardBg = (Color){25, 38, 65, 255};
        DrawRectangle(cx, cy, cardW, cardH, cardBg);
        DrawRectangleLines(cx, cy, cardW, cardH, (Color){70, 90, 130, 255});

        int swatchSize = (int)(18 * scl);
        DrawRectangle(cx + 8, cy + 8, swatchSize, swatchSize, sp->color);
        DrawRectangleLines(cx + 8, cy + 8, swatchSize, swatchSize, WHITE);

        char nameLine[48];
        snprintf(nameLine, sizeof(nameLine), "%s", sp->name);
        DrawTextEx(g_uiFont, nameLine, (Vector2){(float)cx + swatchSize + 14, (float)cy + 6},
                   (int)(18 * scl), 1, (Color){255, 220, 100, 255});

        char typeLine[32];
        snprintf(typeLine, sizeof(typeLine), "[%s]", PET_TYPE_NAMES[sp->type]);
        DrawTextEx(g_uiFont, typeLine, (Vector2){(float)cx + swatchSize + 14, (float)cy + 6 + 18 * scl + 2},
                   (int)(14 * scl), 1, (Color){180, 200, 240, 255});

        char statLine[96];
        snprintf(statLine, sizeof(statLine), "HP:%d ATK:%d DEF:%d SPD:%d",
                 sp->baseHP, sp->baseAttack, sp->baseDefense, sp->baseSpeed);
        DrawTextEx(g_uiFont, statLine, (Vector2){(float)cx + 8, (float)cy + 38 * scl},
                   (int)(12 * scl), 1, (Color){100, 255, 150, 255});

        if (sp->learnableMoves[0] >= 0) {
            const MoveData *mv = &MOVE_DATABASE[sp->learnableMoves[0]];
            char moveLine[64];
            snprintf(moveLine, sizeof(moveLine), "技能: %s (%d威)", mv->name, mv->basePower);
            DrawTextEx(g_uiFont, moveLine, (Vector2){(float)cx + 8, (float)cy + 54 * scl},
                       (int)(12 * scl), 1, (Color){200, 200, 220, 255});
        }

        DrawTextEx(g_uiFont, PET_FLAVOR[i], (Vector2){(float)cx + 8, (float)cy + 72 * scl},
                   (int)(10 * scl), 1, (Color){150, 160, 180, 255});

        char lv5Line[64];
        snprintf(lv5Line, sizeof(lv5Line), "Lv5: A=%d D=%d S=%d",
                 sp->baseAttack * 5 / 20 + 8, sp->baseDefense * 5 / 20 + 8, sp->baseSpeed * 5 / 20 + 8);
        DrawTextEx(g_uiFont, lv5Line, (Vector2){(float)cx + 8, (float)cy + 88 * scl},
                   (int)(10 * scl), 1, (Color){140, 150, 170, 255});
    }

    DrawTextEx(g_uiFont, "[ESC] 返回菜单",
               (Vector2){(float)sw - 140, (float)sh - 30}, 14, 1, (Color){150, 150, 170, 255});
}

int UpdateStarterSelectButtons(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scl = sh / 720.0f;

    int cardW = (int)((sw - 60) / 4);
    int cardH = (int)(140 * scl);
    int startX = 15, startY = (int)(30 * scl + 36 * scl + 16 * scl + 20);
    int gap = 10;

    for (int i = 0; i < (int)PET_COUNT; i++) {
        int col = i % 4;
        int row = i / 4;
        Rectangle card = {
            (float)(startX + col * (cardW + gap)),
            (float)(startY + row * (cardH + gap)),
            (float)cardW, (float)cardH
        };
        if (CheckCollisionPointRec(GetMousePosition(), card) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            return i;
        }
    }

    return -1;
}
