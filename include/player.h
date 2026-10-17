#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct Terrain Terrain;

#define BASE_SPEED 12.0f
#define SPRINT_SPEED 24.0f
#define ROTATION_SPEED 12.0f
#define MAX_STAMINA 100.0f
#define SPRINT_COST 20.0f
#define STAMINA_REGEN 10.0f

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 direction;
    bool isGrounded;
    float facing;
    float stamina;
} Body;

void InitPlayer(Body *player, Vector3 startPos);
void UpdatePlayer(Body *player, float cameraYaw, char sideway, char forward,
                  bool jump, bool sprint, Terrain *terrain);

#endif
