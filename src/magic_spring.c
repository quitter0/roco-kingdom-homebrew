#include "magic_spring.h"
#include "ui_menu.h"
#include <math.h>
#include <stdio.h>

void InitMagicSpring(MagicSpring *spring, float x, float z) {
    spring->position = (Vector3){x, 0.1f, z};
    spring->isHealing = false;
    spring->healTimer = 0.0f;
}

void DrawMagicSpring(const MagicSpring *spring) {
    float x = spring->position.x;
    float y = spring->position.y;
    float z = spring->position.z;
    float t = (float)GetTime();

    // Large stone base platform
    DrawCylinder((Vector3){x, y + 0.01f, z}, 3.0f, 3.0f, 0.2f, 24,
                 (Color){90, 80, 70, 255});
    DrawCylinderWires((Vector3){x, y + 0.21f, z}, 3.05f, 3.05f, 0.02f, 24,
                      (Color){60, 50, 40, 255});

    // Water pool — pulsing blue surface
    float pulse = 1.0f + sinf(t * 2.0f) * 0.04f;
    DrawCylinder((Vector3){x, y + 0.18f, z}, 2.5f * pulse, 2.5f * pulse, 0.2f, 24,
                 (Color){20, 120, 210, 230});
    DrawCylinderWires((Vector3){x, y + 0.38f, z}, 2.3f, 2.3f, 0.02f, 24,
                      (Color){100, 200, 255, 160});

    // 4 stone pillars around the pool
    for (int i = 0; i < 4; i++) {
        float angle = i * PI / 2.0f + PI / 4.0f;
        float px = x + cosf(angle) * 2.8f;
        float pz = z + sinf(angle) * 2.8f;
        // Pillar body
        DrawCylinder((Vector3){px, y + 0.15f, pz}, 0.3f, 0.38f, 2.4f, 12,
                     (Color){150, 140, 130, 255});
        // Glowing orb on top
        float orbGlow = 0.5f + sinf(t * 3.0f + i * 1.5f) * 0.5f;
        DrawSphere((Vector3){px, y + 2.8f, pz}, 0.28f,
                   (Color){(unsigned char)(80 + orbGlow * 120),
                           (unsigned char)(170 + orbGlow * 85),
                           255, 230});
        DrawSphere((Vector3){px, y + 2.8f, pz}, 0.55f,
                   (Color){70, 150, 255, (unsigned char)(orbGlow * 70)});
    }

    // Central glowing column
    float colPulse = 1.0f + sinf(t * 4.0f) * 0.06f;
    DrawCylinder((Vector3){x, y + 0.4f, z}, 0.25f * colPulse, 0.25f * colPulse, 4.5f, 16,
                 (Color){80, 190, 255, 200});
    DrawCylinder((Vector3){x, y + 0.4f, z}, 0.08f, 0.08f, 4.5f, 8,
                 (Color){200, 240, 255, 255});

    // Glow aura — stacked transparent spheres
    for (int i = 0; i < 5; i++) {
        float ay = y + 0.7f + i * 0.85f;
        float auraAlpha = 25.0f + sinf(t * 5.0f + (float)i) * 12.0f;
        DrawSphere((Vector3){x, ay, z}, 0.9f + sinf(t * 3.0f + (float)i) * 0.15f,
                   (Color){80, 190, 255, (unsigned char)auraAlpha});
    }

    // Floating sparkle particles
    for (int i = 0; i < 12; i++) {
        float angle = t * 0.7f + (float)i * PI / 6.0f;
        float radius = 1.5f + sinf(t * 2.0f + (float)i) * 0.5f;
        float py = y + 1.2f + sinf(t * 3.0f + (float)i * 0.7f) * 1.8f;
        float sx = x + cosf(angle) * radius;
        float sz = z + sinf(angle) * radius;
        float sparkleBright = 0.5f + sinf(t * 6.0f + (float)i) * 0.5f;
        DrawSphere((Vector3){sx, py, sz}, 0.06f + sparkleBright * 0.04f,
                   (Color){(unsigned char)(180 + sparkleBright * 75),
                           230, 255,
                           (unsigned char)(150 + sparkleBright * 105)});
    }

    // Top beacon beam
    DrawCylinder((Vector3){x, y + 5.5f, z}, 0.05f, 0.05f, 3.0f, 6,
                 (Color){60, 140, 240, 140});
    DrawSphere((Vector3){x, y + 8.5f, z}, 0.45f, (Color){60, 150, 255, 255});
    DrawSphere((Vector3){x, y + 8.5f, z}, 1.0f, (Color){60, 140, 255, 45});
}

void DrawHealingProgressBar(const MagicSpring *spring) {
    if (!spring->isHealing) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float progress = spring->healTimer / MAGIC_SPRING_HEAL_DURATION;
    if (progress > 1.0f) progress = 1.0f;

    int barW = 400, barH = 32;
    int bx = (sw - barW) / 2, by = sh / 2 - 80;

    // Title
    const char *title = "魔法泉水恢复中...";
    Vector2 titleSize = MeasureTextEx(g_uiFont, title, 20, 1);
    DrawTextEx(g_uiFont, title,
               (Vector2){(sw - titleSize.x) / 2, (float)by - 28}, 20, 1,
               (Color){100, 210, 255, 255});

    // Bar background
    DrawRectangle(bx - 3, by - 3, barW + 6, barH + 6, (Color){0, 0, 0, 180});
    DrawRectangle(bx, by, barW, barH, (Color){20, 30, 50, 240});

    // Progress fill
    int fillW = (int)((float)barW * progress);
    if (fillW > 0) {
        DrawRectangle(bx, by, fillW, barH, (Color){30, 100, 210, 255});
        // Brighter inner stripe
        DrawRectangle(bx, by + 4, fillW, barH - 8, (Color){50, 140, 240, 200});
    }

    // Border
    DrawRectangleLines(bx, by, barW, barH, (Color){80, 170, 255, 255});

    // Time remaining
    float remaining = MAGIC_SPRING_HEAL_DURATION - spring->healTimer;
    if (remaining < 0.0f) remaining = 0.0f;
    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1f 秒", remaining);
    Vector2 timeSize = MeasureTextEx(g_uiFont, timeText, 16, 1);
    DrawTextEx(g_uiFont, timeText,
               (Vector2){(sw - timeSize.x) / 2, (float)(by + barH + 6)}, 16, 1, WHITE);
}

bool IsNearMagicSpring(const MagicSpring *spring, Vector3 playerPos) {
    float dx = playerPos.x - spring->position.x;
    float dz = playerPos.z - spring->position.z;
    return sqrtf(dx * dx + dz * dz) < MAGIC_SPRING_INTERACT_DIST;
}
