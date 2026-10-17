#include "player.h"
#include "terrain.h"
#include "raymath.h"
#include <math.h>

#define GRAVITY 32.0f
#define JUMP_FORCE 12.0f
#define MAX_ACCEL 150.0f
#define FRICTION 0.86f
#define AIR_DRAG 0.98f
#define CONTROL 15.0f

void InitPlayer(Body *player, Vector3 startPos) {
    player->position = startPos;
    player->velocity = (Vector3){0, 0, 0};
    player->direction = (Vector3){0, 0, 0};
    player->isGrounded = false;
    player->facing = 0.0f;
    player->stamina = MAX_STAMINA;
}

void UpdatePlayer(Body *player, float cameraYaw, char sideway, char forward,
                  bool jump, bool sprint, Terrain *terrain) {
    float delta = GetFrameTime();
    bool hasInput = (sideway != 0 || forward != 0);

    if (!player->isGrounded) player->velocity.y -= GRAVITY * delta;

    if (player->isGrounded && jump) {
        player->velocity.y = JUMP_FORCE;
        player->isGrounded = false;
    }

    // Camera-relative movement direction
    Vector3 camFront = (Vector3){sinf(cameraYaw), 0.f, cosf(cameraYaw)};
    Vector3 camRight = (Vector3){cosf(-cameraYaw), 0.f, sinf(-cameraYaw)};

    Vector3 desiredDir = (Vector3){
        (float)sideway * camRight.x + (float)forward * camFront.x,
        0.0f,
        (float)sideway * camRight.z + (float)forward * camFront.z
    };
    player->direction = Vector3Lerp(player->direction, desiredDir, CONTROL * delta);

    // Sprint / stamina
    float maxSpd = BASE_SPEED;
    if (sprint && player->stamina > 5.0f) {
        maxSpd = SPRINT_SPEED;
        player->stamina -= SPRINT_COST * delta;
        if (player->stamina < 0.0f) player->stamina = 0.0f;
    } else {
        player->stamina += STAMINA_REGEN * delta;
        if (player->stamina > MAX_STAMINA) player->stamina = MAX_STAMINA;
    }

    // Velocity
    float decel = (player->isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel = (Vector3){player->velocity.x * decel, 0.0f, player->velocity.z * decel};
    if (Vector3Length(hvel) < maxSpd * 0.01f) hvel = (Vector3){0};

    float speed = Vector3DotProduct(hvel, player->direction);
    float accel = Clamp(maxSpd - speed, 0.f, MAX_ACCEL * delta);
    hvel.x += player->direction.x * accel;
    hvel.z += player->direction.z * accel;
    player->velocity.x = hvel.x;
    player->velocity.z = hvel.z;

    // Instant facing = movement direction (visual only, no feedback loop)
    if (hasInput) {
        float hSpeed = sqrtf(hvel.x * hvel.x + hvel.z * hvel.z);
        if (hSpeed > 0.3f)
            player->facing = -atan2f(hvel.x, hvel.z);
    }

    // ============ Terrain interaction ============
    float curGround = GetTerrainHeight(terrain, player->position.x, player->position.z);
    float nextX = player->position.x + player->velocity.x * delta;
    float nextZ = player->position.z + player->velocity.z * delta;
    float nextGround = GetTerrainHeight(terrain, nextX, nextZ);
    float step = nextGround - curGround;

    if (player->isGrounded) {
        if (step > 0.1f && step <= 1.5f) {
            // Walkable step — pre-elevate Y
            player->position.y = nextGround;
        } else if (step > 1.5f) {
            // Wall — near-stop velocity, cancel horizontal advance
            player->velocity.x *= 0.05f;
            player->velocity.z *= 0.05f;
            nextX = player->position.x;
            nextZ = player->position.z;
        } else if (step < -3.0f) {
            // Cliff edge — don't walk off
            nextX = player->position.x;
            nextZ = player->position.z;
        }
    }

    // Apply horizontal position
    player->position.x = nextX;
    player->position.z = nextZ;

    // Apply vertical motion
    player->position.y += player->velocity.y * delta;

    // Ground collision
    float groundY = GetTerrainHeight(terrain, player->position.x, player->position.z);
    if (player->position.y <= groundY + 0.15f) {
        player->position.y = groundY;
        if (player->velocity.y < 0.0f) player->velocity.y = 0.0f;
        player->isGrounded = true;
    } else {
        player->isGrounded = false;
    }

    // Slope influence on speed (grounded + moving, sampled from neighbors)
    if (player->isGrounded && hasInput) {
        float ns = 0.4f;
        float gL = GetTerrainHeight(terrain, player->position.x - ns, player->position.z);
        float gR = GetTerrainHeight(terrain, player->position.x + ns, player->position.z);
        float gD = GetTerrainHeight(terrain, player->position.x, player->position.z - ns);
        float gU = GetTerrainHeight(terrain, player->position.x, player->position.z + ns);
        float nx = gL - gR, nz = gD - gU;
        float nLen = sqrtf(nx * nx + 4.0f * ns * ns + nz * nz);
        if (nLen > 0.0001f) {
            nx /= nLen; nz /= nLen;
            float slopeDot = player->direction.x * nx + player->direction.z * nz;
            // Uphill slow / downhill boost (additive, clamped)
            float boost = -slopeDot * maxSpd * 0.25f;
            player->velocity.x += player->direction.x * boost * delta;
            player->velocity.z += player->direction.z * boost * delta;
            // Clamp to max speed after slope adjustment
            float hSpd = sqrtf(player->velocity.x * player->velocity.x +
                               player->velocity.z * player->velocity.z);
            if (hSpd > maxSpd * 1.15f) {
                float scl = maxSpd * 1.15f / hSpd;
                player->velocity.x *= scl;
                player->velocity.z *= scl;
            }
        }
    }
}
