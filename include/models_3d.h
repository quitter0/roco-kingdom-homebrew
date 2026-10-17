#ifndef MODELS_3D_H
#define MODELS_3D_H

#include "raylib.h"
#include "pet_data.h"
#include <stdbool.h>

typedef enum {
    CHAR_MODEL_PLAYER = 0,
    CHAR_MODEL_NURSE,
    CHAR_MODEL_MERCHANT,
    CHAR_MODEL_OLD,
    CHAR_MODEL_RESEARCHER,
    CHAR_MODEL_COUNT
} CharacterModelType;

// facingAngle = yaw in radians (0 = +Z, PI/2 = +X)
// isWalking triggers leg/arm swing, walkTime drives the cycle (accumulated delta)
void DrawPlayerModel(Vector3 pos, Color bodyColor, float scale,
                     float facingAngle, bool isWalking, float walkTime);
// charType >= 0 tries GLB model, -1 = procedural fallback
void DrawNPCModel(Vector3 pos, Color bodyColor, bool isTrainer, float scale,
                  float facingAngle, int charType);
// animTime drives idle bob; isAttacking adds attack animation; moveType affects squash/lunge style
void DrawPetBattleModel(Vector3 pos, int speciesId, float scale,
                        bool isEnemy, bool isAttacking, float animTime,
                        PetType moveType, float animDuration);
void DrawBattleArena(Vector3 center, float radius);

// GLB model loading (call once on init, once on cleanup)
void InitPetModels(void);
void UnloadPetModels(void);
void InitCharacterModels(void);
void UnloadCharacterModels(void);

#endif
