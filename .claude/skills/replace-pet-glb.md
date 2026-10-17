---
name: replace-pet-glb
description: Replace procedural pet model with GLB file (vertex pre-rotation, auto-scale, 16-bit index fix)
metadata:
  type: reference
---

# Replace Pet Model with GLB

Fully automated pipeline: load GLB → fix 16-bit index overflow → auto-scale from bounding box → pre-rotate 180° for player-facing copy.

## How it works

1. **Load**: `LoadModel("resources/models/pet_N.glb")` via raylib's cgltf backend
2. **Fix 16-bit indices**: If vertexCount > 65535, rebuild as non-indexed mesh via `LoadGLBNonIndexed()`
3. **Auto-scale**: `scale = 1.5 / max(boundingBox.width, height, depth)`, clamped to [0.01, 1.5]
4. **Pre-rotate**: Deep-clone mesh, negate all vertex X/Z and normal X/Z (180° Y rotation). Enemy uses default model, player pet uses rotated copy.
5. **Render**: Simple `DrawModel(model, drawPos, scale, WHITE)` — no runtime rotation needed.

## Adding a new model

### Step 1: Place the GLB file

```
resources/models/pet_N.glb   (N = GLB file index, not species ID)
```

### Step 2: Add mapping in `src/models_3d.c`

Edit the `glbToSpecies` array to map GLB file index → species ID:

```c
// GLB file index → species ID
static int glbToSpecies[10] = {0, 2, 1, 4, -1, -1, 5, 7, 3, 6};
//                              ↑  ↑  ↑  ↑   ↑   ↑  ↑  ↑  ↑  ↑
//                            pet_0 → species 0 (火花)
//                                 pet_1 → species 2 (喵喵)
//                                      pet_2 → species 1 (水蓝蓝)
//                                           pet_3 → species 4 (格兰花)
//                                                pet_4,5 → unused
//                                                      pet_6 → species 5 (雪精灵)
//                                                           pet_7 → species 7 (幽灵)
//                                                                pet_8 → species 3 (瞌睡王)
//                                                                     pet_9 → species 6 (小包子)
```

### Step 3: Build

```bash
cd build && mingw32-make
```

That's it. No code changes needed beyond the mapping array — `InitPetModels()` auto-loads all GLBs, `DrawPetBattleModel` auto-detects and renders them.

## Key functions

| Function | File | Purpose |
|----------|------|---------|
| `InitPetModels()` | `src/models_3d.c` | Load all GLBs, fix indices, compute scale, create rotated copies |
| `LoadOneGLB(glbIdx)` | `src/models_3d.c` | Load single GLB + create rotated copy |
| `CloneMeshRotated(src)` | `src/models_3d.c` | Deep-clone mesh with 180° Y rotation on vertices+normals |
| `LoadGLBNonIndexed(path)` | `src/models_3d.c` | Reload GLB via cgltf, expand to non-indexed (fixes 16-bit overflow) |

## Current model state

| GLB file | Species | Name | Status |
|----------|---------|------|--------|
| pet_0.glb | 0 | 火花 | loaded |
| pet_1.glb | 2 | 喵喵 | loaded |
| pet_2.glb | 1 | 水蓝蓝 | loaded |
| pet_3.glb | 4 | 格兰花 | loaded |
| pet_6.glb | 5 | 雪精灵 | loaded |
| pet_7.glb | 7 | 幽灵 | loaded |
| pet_8.glb | 3 | 瞌睡王 | loaded |
| pet_9.glb | 6 | 小包子 | loaded |
| pet_4.glb | - | unused | loaded (ignored) |
| pet_5.glb | - | unused | loaded (ignored) |

## Debug

After running the game, check `build/glb_debug.txt` for per-species load status, vertex counts, and computed scales.

## Rollback

To revert a species to procedural: set its entry in `glbToSpecies` to `-1` and rebuild.
