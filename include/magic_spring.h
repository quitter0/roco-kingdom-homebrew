#ifndef MAGIC_SPRING_H
#define MAGIC_SPRING_H

#include "raylib.h"
#include <stdbool.h>

#define MAGIC_SPRING_HEAL_DURATION 10.0f
#define MAGIC_SPRING_INTERACT_DIST 4.0f

typedef struct {
    Vector3 position;
    bool isHealing;
    float healTimer;
} MagicSpring;

void InitMagicSpring(MagicSpring *spring, float x, float z);
void DrawMagicSpring(const MagicSpring *spring);
void DrawHealingProgressBar(const MagicSpring *spring);
bool IsNearMagicSpring(const MagicSpring *spring, Vector3 playerPos);

#endif
