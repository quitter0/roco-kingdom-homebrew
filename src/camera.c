#include "camera.h"
#include "raymath.h"
#include <math.h>

#define EYE_HEIGHT 1.5f

void UpdateCameraFollow(Camera *camera, Vector3 playerPos, Vector2 *lookRotation, float camDist) {
    const Vector3 up = (Vector3){0.0f, 1.0f, 0.0f};

    // Clamp pitch
    float maxAngleUp = 1.4f;
    float maxAngleDown = -0.8f;
    if (lookRotation->y > maxAngleUp) lookRotation->y = maxAngleUp;
    if (lookRotation->y < maxAngleDown) lookRotation->y = maxAngleDown;

    // Clamp camera distance
    if (camDist < 3.0f) camDist = 3.0f;
    if (camDist > 20.0f) camDist = 20.0f;

    // Camera orbits around player at distance camDist
    float yaw = lookRotation->x;
    float pitch = lookRotation->y;

    Vector3 offset;
    offset.x = sinf(yaw) * cosf(pitch) * camDist;
    offset.y = -sinf(pitch) * camDist;
    offset.z = cosf(yaw) * cosf(pitch) * camDist;

    camera->position = (Vector3){
        playerPos.x - offset.x,
        playerPos.y + EYE_HEIGHT - offset.y,
        playerPos.z - offset.z
    };
    camera->target = (Vector3){
        playerPos.x,
        playerPos.y + EYE_HEIGHT,
        playerPos.z
    };
    camera->up = up;
}
