#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"

void UpdateCameraFollow(Camera *camera, Vector3 playerPos, Vector2 *lookRotation, float camDist);

#endif
