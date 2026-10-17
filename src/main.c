#include "raylib.h"
#include "raygui.h"
#include "player.h"
#include "camera.h"
#include "terrain.h"
#include "game_state.h"
#include "pet.h"
#include "pet_data.h"
#include "inventory.h"
#include "battle.h"
#include "npc.h"
#include "ui_menu.h"
#include "audio.h"
#include "save.h"
#include "shop.h"
#include "models_3d.h"
#include "magic_spring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SAVE_PATH "resources/save.dat"

static unsigned int g_defeatedTrainerHashes[MAX_DEFEATED_TRAINERS];
static int g_defeatedTrainerCount = 0;

static bool IsTrainerDefeated(unsigned int hash) {
    for (int i = 0; i < g_defeatedTrainerCount; i++)
        if (g_defeatedTrainerHashes[i] == hash) return true;
    return false;
}

static void MarkTrainerDefeated(unsigned int hash) {
    if (g_defeatedTrainerCount >= MAX_DEFEATED_TRAINERS) return;
    if (IsTrainerDefeated(hash)) return;
    g_defeatedTrainerHashes[g_defeatedTrainerCount++] = hash;
}

static void AutoSave(const char *path, Body *player, Pet *team, int teamSize,
                     int activeIdx, Inventory *inv, double playTime) {
    SaveData sd;
    memset(&sd, 0, sizeof(sd));
    sd.playerPosition = player->position;
    sd.teamSize = teamSize;
    sd.activePetIndex = activeIdx;
    sd.inventory = *inv;
    sd.playTime = (int)playTime;
    for (int i = 0; i < teamSize; i++) sd.team[i] = team[i];
    NPC *npcs = GetNPCs();
    int nc = GetNPCCount();
    for (int i = 0; i < nc && i < MAX_SAVE_NPCS; i++)
        sd.npcDefeated[i] = npcs[i].defeated;
    sd.defeatedTrainerCount = g_defeatedTrainerCount;
    for (int i = 0; i < g_defeatedTrainerCount; i++)
        sd.defeatedTrainerHashes[i] = g_defeatedTrainerHashes[i];
    SaveGame(path, &sd);
}

int main(void) {
    srand((unsigned int)time(NULL));
    InitWindow(1280, 720, "Roco Kingdom");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // disable ESC as window close key
    InitUI();

    // ---- Game state ----
    GameState gameState = STATE_MENU;
    GameState prevState = -1;  // force state entry on first frame

    // ---- Player ----
    Body player;
    InitPlayer(&player, (Vector3){0, 10, 0});

    // ---- Terrain ----
    Terrain terrain = LoadTerrain();

    // ---- Camera ----
    Camera camera = {0};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (Vector3){0, 1, 0};
    Vector2 lookRotation = {0};
    float camDist = 8.0f;  // third-person camera distance

    // ---- Pets ----
    Pet playerTeam[6];
    int teamSize = 1;
    int activePetIdx = 0;
    playerTeam[0] = CreatePet(0, 5);

    // ---- Inventory ----
    Inventory inventory;
    InitInventory(&inventory);

    // ---- NPC & Encounters ----
    EncounterSystem encounter;
    InitEncounterSystem(&encounter);

    // ---- Battle ----
    BattleState battle = {0};

    // ---- Audio ----
    AudioManager audio;
    InitAudioManager(&audio);

    // ---- GLB models ----
    InitPetModels();
    InitCharacterModels();

    // ---- NPC dialog state ----
    int talkingToNPC = -1;
    bool trainerBattle = false;
    int trainerCellX = 0, trainerCellZ = 0;
    bool showDialog = false;
    int dialogPage = 0;
    bool showShop = false;           // nurse shop or merchant shop
    int shopNpcType = 0;             // 0=nurse, 1=merchant
    MerchantShop merchantShop = {0};
    double merchantTimer = 0.0;
    float merchantCX = 0, merchantCZ = 0;
    bool merchantActive = false;

    // ---- Magic Spring ----
    MagicSpring magicSpring;
    InitMagicSpring(&magicSpring, 0.0f, -40.0f);

    // ---- Menu/Save ----
    bool saveExists = SaveFileExists(SAVE_PATH);
    bool saveValid = false;

    // ---- Play time ----
    double playTime = 0.0;
    float walkAnimTime = 0.0f;
    // Wild pet tracking
    #define MAX_WILD_PETS 5
    typedef struct { Vector3 pos; int species; float dir; float speed; float timer; } WildPet;
    WildPet wildPets[MAX_WILD_PETS];
    int wildPetCount = 0;
    float wildPetSpawnTimer = 0.0f;
    double battleEndTime = 0.0;
    bool battleResolved = false;

    DisableCursor();
    SetTargetFPS(60);

    // B-key edge detector (runs once per frame, before state switch)
    static bool prevB = false;

    // Battle item menu
    static bool showItemMenu = false;
    static bool wasItemMenuOpen = false;
    // Revive selection
    static bool showReviveSelect = false;
    // Notification toast
    static Notification g_notification = {0};

    // ============ MAIN LOOP ============
    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        // Manual B-key edge detection (avoids raygui interference)
        bool currB = IsKeyDown(KEY_B);
        bool bJustPressed = (currB && !prevB);
        prevB = currB;

        // Capture E-key once per frame + track shop state at frame start
        bool ePressed = IsKeyPressed(KEY_E);
        bool wasShopOpen = showShop;

        // ---- State Machine ----
        switch (gameState) {

        // ======== MENU ========
        case STATE_MENU: {
            if (prevState != STATE_MENU) {
                prevState = STATE_MENU;
                EnableCursor();
                saveExists = SaveFileExists(SAVE_PATH);
                saveValid = false;
                if (saveExists) {
                    SaveData sd;
                    if (LoadGame(SAVE_PATH, &sd)) {
                        bool hasAlive = false;
                        for (int si = 0; si < sd.teamSize; si++)
                            if (sd.team[si].currentHP > 0) { hasAlive = true; break; }
                        saveValid = hasAlive || sd.inventory.revives > 0;
                    }
                }
                PlayBGM(&audio, BGM_MENU);
            }

            BeginDrawing();
            DrawMenu();
            int action = UpdateMenuButtons(saveExists && saveValid);
            EndDrawing();

            if (action == 1) {  // New Game → starter selection
                playTime = 0.0;
                g_defeatedTrainerCount = 0;
                NPC *npcs = GetNPCs();
                int totalNpcs = GetNPCCount();
                for (int i = 0; i < totalNpcs; i++) {
                    npcs[i].defeated = false;
                    npcs[i].position.x = npcs[i].baseX;
                    npcs[i].position.z = npcs[i].baseZ;
                }
                gameState = STATE_STARTER_SELECT;
            } else if (action == 2) {  // Continue
                SaveData sd;
                if (LoadGame(SAVE_PATH, &sd)) {
                    player.position = sd.playerPosition;
                    player.velocity = (Vector3){0};
                    player.direction = (Vector3){0};
                    player.isGrounded = false;
                    lookRotation = (Vector2){0};
                    teamSize = sd.teamSize;
                    activePetIdx = sd.activePetIndex;
                    for (int i = 0; i < sd.teamSize; i++) playerTeam[i] = sd.team[i];
                    inventory = sd.inventory;
                    InitEncounterSystem(&encounter);
                    playTime = (double)sd.playTime;
                    g_defeatedTrainerCount = sd.defeatedTrainerCount;
                    for (int i = 0; i < sd.defeatedTrainerCount; i++)
                        g_defeatedTrainerHashes[i] = sd.defeatedTrainerHashes[i];
                    {
                        NPC *npcs2 = GetNPCs();
                        int nc2 = GetNPCCount();
                        for (int i = 0; i < nc2 && i < MAX_SAVE_NPCS; i++)
                            npcs2[i].defeated = sd.npcDefeated[i];
                    }
                    gameState = STATE_WORLD;
                    DisableCursor();
                }
            } else if (action == 4) {  // Bestiary
                gameState = STATE_BESTIARY;
            } else if (action == 3) {  // Quit
                CloseWindow();
                break;
            }
            break;
        }

        // ======== STARTER SELECT ========
        case STATE_STARTER_SELECT: {
            if (prevState != STATE_STARTER_SELECT) {
                prevState = STATE_STARTER_SELECT;
                EnableCursor();
            }

            BeginDrawing();
            DrawStarterSelectScene();
            int selected = UpdateStarterSelectButtons();
            EndDrawing();

            if (selected >= 0 && selected < (int)PET_COUNT) {
                // Initialize player and team with chosen starter
                InitPlayer(&player, (Vector3){0, 10, 0});
                lookRotation = (Vector2){0};
                playerTeam[0] = CreatePet(selected, 5);
                teamSize = 1;
                activePetIdx = 0;
                InitInventory(&inventory);
                InitEncounterSystem(&encounter);
                gameState = STATE_WORLD;
                DisableCursor();
                camera.position = player.position;
                PlayBGM(&audio, BGM_WORLD);
            } else if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = STATE_MENU;
            }
            break;
        }

        // ======== WORLD ========
        case STATE_WORLD: {
            if (prevState != STATE_WORLD) {
                GameState comingFrom = prevState;
                prevState = STATE_WORLD;
                DisableCursor();
                camera.position = player.position;
                if (comingFrom == STATE_BATTLE || comingFrom == STATE_PET_BAG) {
                    ResumeBGM(&audio, BGM_WORLD);
                } else {
                    PlayBGM(&audio, BGM_WORLD);
                }
            }

            // ---- Input ----
            Vector2 mouseDelta = GetMouseDelta();
            if (!showShop && !showDialog) {
                lookRotation.x -= mouseDelta.x * 0.001f;
                lookRotation.y += mouseDelta.y * 0.001f;
                camDist -= GetMouseWheelMove() * 1.5f;
                if (camDist < 3.0f) camDist = 3.0f;
                if (camDist > 20.0f) camDist = 20.0f;
            }

            char sideway = (char)(IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
            char forward = (char)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
            bool jump = IsKeyPressed(KEY_SPACE);
            bool sprint = IsKeyDown(KEY_LEFT_SHIFT);
            if (showShop || showDialog) {
                sideway = 0;
                forward = 0;
                jump = false;
            }
            bool isMoving = (sideway != 0 || forward != 0);

            Vector3 prevPos = player.position;

            // ---- Notification timer ----
        if (g_notification.active) {
            g_notification.timer -= delta;
            if (g_notification.timer <= 0.0f) g_notification.active = false;
        }

        // ---- Magic Spring healing timer ----
            if (magicSpring.isHealing) {
                magicSpring.healTimer += delta;
                if (magicSpring.healTimer >= MAGIC_SPRING_HEAL_DURATION) {
                    // Heal all pets: full HP + Energy
                    for (int j = 0; j < teamSize; j++) {
                        playerTeam[j].currentHP = playerTeam[j].maxHP;
                        playerTeam[j].sharedPP = playerTeam[j].maxSharedPP;
                        playerTeam[j].status = STATUS_NONE;
                        playerTeam[j].statusTimer = 0;
                        playerTeam[j].speedDown = false;
                    }
                    magicSpring.isHealing = false;
                    magicSpring.healTimer = 0.0f;
                }
            } else {
                // Movement uses player's independent facing, camera orbits separately
                UpdatePlayer(&player, lookRotation.x, sideway, forward, jump, sprint, &terrain);
            }
            UpdateCameraFollow(&camera, player.position, &lookRotation, camDist);

            // Infinite world: recenter terrain mesh when player walks too far
            if (TerrainNeedsRecenter(&terrain, player.position.x, player.position.z)) {
                float nx = roundf(player.position.x / 50.0f) * 50.0f;
                float nz = roundf(player.position.z / 50.0f) * 50.0f;
                RecenterTerrain(&terrain, nx, nz);
            }

            playTime += delta;
            if (isMoving) walkAnimTime += delta;

            // ---- Tree/Rock collision ----
            {
                Vector3 colPos;
                float colR;
                // Push out of trees
                if (GetTreeCollision(&terrain, player.position.x, player.position.z, &colPos, &colR)) {
                    float dx = player.position.x - colPos.x;
                    float dz = player.position.z - colPos.z;
                    float dist = sqrtf(dx * dx + dz * dz);
                    float pushDist = colR + 0.6f;
                    if (dist < pushDist && dist > 0.001f) {
                        player.position.x = colPos.x + dx / dist * pushDist;
                        player.position.z = colPos.z + dz / dist * pushDist;
                        player.velocity.x *= 0.3f;
                        player.velocity.z *= 0.3f;
                    }
                }
                // Rocks: step on top or push out (sphere at ry+0.9, radius 1.2)
                if (GetRockCollision(&terrain, player.position.x, player.position.z, &colPos, &colR)) {
                    float dx = player.position.x - colPos.x;
                    float dz = player.position.z - colPos.z;
                    float distSq = dx * dx + dz * dz;
                    float rockY = GetTerrainHeight(&terrain, colPos.x, colPos.z);
                    float rockCenterY = rockY + 0.9f;
                    float rockTop = rockCenterY + 1.2f;  // sphere top
                    if (distSq < colR * colR && distSq > 0.0001f) {
                        float dist = sqrtf(distSq);
                        if (player.position.y >= rockTop - 0.6f) {
                            // On top of rock — snap to surface
                            float surfaceY = rockCenterY + sqrtf(fmaxf(0.0f, colR * colR - distSq));
                            if (player.position.y < surfaceY + 0.2f) {
                                player.position.y = surfaceY;
                                player.velocity.y = 0;
                                player.isGrounded = true;
                            }
                        } else {
                            float pushDist = colR + 0.6f;
                            player.position.x = colPos.x + dx / dist * pushDist;
                            player.position.z = colPos.z + dz / dist * pushDist;
                            player.velocity.x *= 0.3f;
                            player.velocity.z *= 0.3f;
                        }
                    }
                }
            }

            // ---- Fall safety ----
            if (player.position.y < -20.0f) {
                player.position = (Vector3){0, 10, 0};
                player.velocity = (Vector3){0, 0, 0};
            }

            // ---- Wild pet management ----
            wildPetSpawnTimer -= delta;
            if (wildPetSpawnTimer <= 0.0f && wildPetCount < MAX_WILD_PETS) {
                // Spawn a new wild pet near the player
                float angle = (float)rand() / RAND_MAX * PI * 2.0f;
                float dist = 15.0f + (float)rand() / RAND_MAX * 30.0f;
                float sx = player.position.x + cosf(angle) * dist;
                float sz = player.position.z + sinf(angle) * dist;
                float sy = GetTerrainHeight(&terrain, sx, sz);
                if (sy > 0.5f) {
                    wildPets[wildPetCount].pos = (Vector3){sx, sy, sz};
                    wildPets[wildPetCount].species = rand() % PET_COUNT;
                    wildPets[wildPetCount].dir = (float)rand() / RAND_MAX * PI * 2.0f;
                    wildPets[wildPetCount].speed = 0.3f + (float)rand() / RAND_MAX * 0.7f;
                    wildPets[wildPetCount].timer = 3.0f + (float)rand() / RAND_MAX * 4.0f;
                    wildPetCount++;
                }
                wildPetSpawnTimer = 2.0f + (float)rand() / RAND_MAX * 3.0f;
            }

            // Merchant spawn
            merchantTimer -= delta;
            if (merchantTimer <= 0.0) {
                merchantTimer = 300.0;  // every 5 minutes
                float ang = (float)rand() / RAND_MAX * PI * 2.0f;
                float dist = 50.0f + (float)rand() / RAND_MAX * 30.0f;
                merchantCX = player.position.x + cosf(ang) * dist;
                merchantCZ = player.position.z + sinf(ang) * dist;
                GenerateMerchant(&merchantShop, playerTeam[activePetIdx].speciesId,
                                 merchantCX, merchantCZ);
                merchantActive = true;
            }
            // Merchant interaction (only if shop was not already open)
            if (merchantActive && !wasShopOpen && !showDialog) {
                float mdx = player.position.x - merchantCX;
                float mdz = player.position.z - merchantCZ;
                if (sqrtf(mdx * mdx + mdz * mdz) < 5.0f && ePressed) {
                    showShop = true;
                    shopNpcType = 1;
                    talkingToNPC = -1;
                }
            }

            // Update wild pets
            for (int wp = 0; wp < wildPetCount; wp++) {
                wildPets[wp].timer -= delta;
                if (wildPets[wp].timer <= 0.0f) {
                    // Change direction
                    wildPets[wp].dir = (float)rand() / RAND_MAX * PI * 2.0f;
                    wildPets[wp].timer = 2.0f + (float)rand() / RAND_MAX * 4.0f;
                }
                float nx = wildPets[wp].pos.x + cosf(wildPets[wp].dir) * wildPets[wp].speed * delta;
                float nz = wildPets[wp].pos.z + sinf(wildPets[wp].dir) * wildPets[wp].speed * delta;
                float ny = GetTerrainHeight(&terrain, nx, nz);
                if (ny > 0.3f) {
                    wildPets[wp].pos.x = nx;
                    wildPets[wp].pos.z = nz;
                    wildPets[wp].pos.y = ny;
                } else {
                    wildPets[wp].dir += PI;  // bounce off water
                }
                // Despawn if too far from player
                float wdx = wildPets[wp].pos.x - player.position.x;
                float wdz = wildPets[wp].pos.z - player.position.z;
                if (sqrtf(wdx * wdx + wdz * wdz) > 60.0f) {
                    wildPets[wp] = wildPets[wildPetCount - 1];
                    wildPetCount--;
                    wp--;
                }
                // Close enough to trigger battle
                if (sqrtf(wdx * wdx + wdz * wdz) < 2.5f && gameState == STATE_WORLD && !magicSpring.isHealing) {
                    Pet wildPet = CreatePet(wildPets[wp].species, 2 + rand() % 5);
                    InitBattle(&battle, playerTeam, teamSize, activePetIdx, &wildPet);
                    talkingToNPC = -1;
                    trainerBattle = false;
                    showDialog = false;
                    showShop = false;
                    gameState = STATE_BATTLE;
                    // Remove this pet
                    wildPets[wp] = wildPets[wildPetCount - 1];
                    wildPetCount--;
                    wp--;
                    break;
                }
            }

            // ---- Wild encounter check (random, low probability) ----
            float dx = player.position.x - prevPos.x;
            float dz = player.position.z - prevPos.z;
            float moveDist = sqrtf(dx * dx + dz * dz);
            float terrainH = GetTerrainHeight(&terrain, player.position.x, player.position.z);

            // Random encounter — warning phase first (blocked during spring healing)
            if (!magicSpring.isHealing) TryWildEncounter(&encounter, moveDist, terrainH);
            if (EncounterWarningTick(&encounter, delta)) {
                ClearEncounterWarning(&encounter);
                Pet wildPet = CreatePet(encounter.warningSpecies, encounter.warningLevel);
                InitBattle(&battle, playerTeam, teamSize, activePetIdx, &wildPet);
                talkingToNPC = -1;
                trainerBattle = false;
                showDialog = false;
                showShop = false;
                gameState = STATE_BATTLE;
                break;
            }

            // ---- Dialog advancement (E key, always handled regardless of shop) ----
            if (ePressed && showDialog) {
                if (talkingToNPC >= 0) {
                    NPC *allNpcs = GetNPCs();
                    NPC *dnpc = &allNpcs[talkingToNPC];
                    bool hasQuestPage = dnpc->hasQuest;
                    int totalPages = dnpc->dialogCount + (hasQuestPage ? 1 : 0);
                    if (!dnpc->isTrainer && totalPages > 1) {
                        dialogPage++;
                        if (dialogPage >= totalPages) {
                            // Dialog complete - give gift if applicable
                            if (!dnpc->givenItem && dnpc->giftItemType > 0) {
                                dnpc->givenItem = true;
                                char giftMsg[128];
                                if (dnpc->giftItemType == 1) {
                                    inventory.potions += dnpc->giftCount;
                                    snprintf(giftMsg, sizeof(giftMsg), "获得了 治疗药水 x%d!", dnpc->giftCount);
                                } else if (dnpc->giftItemType == 2) {
                                    inventory.superPotions += dnpc->giftCount;
                                    snprintf(giftMsg, sizeof(giftMsg), "获得了 超级药水 x%d!", dnpc->giftCount);
                                } else if (dnpc->giftItemType == 3) {
                                    inventory.ethers += dnpc->giftCount;
                                    snprintf(giftMsg, sizeof(giftMsg), "获得了 能量剂 x%d!", dnpc->giftCount);
                                } else if (dnpc->giftItemType == 4) {
                                    inventory.petBalls += dnpc->giftCount;
                                    snprintf(giftMsg, sizeof(giftMsg), "获得了 宠物球 x%d!", dnpc->giftCount);
                                } else if (dnpc->giftItemType == 5) {
                                    inventory.gold += dnpc->giftCount * 50;
                                    snprintf(giftMsg, sizeof(giftMsg), "获得了 %d 金币!", dnpc->giftCount * 50);
                                }
                                ShowNotification(&g_notification, giftMsg);
                            }
                            showDialog = false;
                            talkingToNPC = -1;
                            dialogPage = 0;
                            // For nurse: open shop after dialog completes
                            if (strcmp(dnpc->name, "护士") == 0) {
                                showShop = true;
                                shopNpcType = 0;
                            }
                        }
                    } else {
                        showDialog = false;
                        talkingToNPC = -1;
                        dialogPage = 0;
                    }
                }
            } else if (ePressed && !wasShopOpen && !showDialog) {

            // ---- NPC / Spring interaction (E key, start new interaction) ----
                if (!magicSpring.isHealing) {
                    // Magic spring — start healing
                    if (IsNearMagicSpring(&magicSpring, player.position)) {
                        magicSpring.isHealing = true;
                        magicSpring.healTimer = 0.0f;
                    } else {
                        NPC *npcs = GetNPCs();
                        int npcCount = GetNPCCount();
                        for (int i = 0; i < npcCount; i++) {
                            if (CheckNPCInteraction(player.position, &npcs[i], 6.0f)) {
                                // Nurse NPC - dialog + shop only (no restore)
                                if (!npcs[i].isTrainer && strcmp(npcs[i].name, "护士") == 0) {
                                    talkingToNPC = i;
                                    showDialog = true;
                                    dialogPage = 0;
                                } else if (npcs[i].isTrainer && !npcs[i].defeated) {
                                Pet trainerPet = CreatePet(npcs[i].petSpeciesId, npcs[i].petLevel);
                                InitBattle(&battle, playerTeam, teamSize, activePetIdx, &trainerPet);
                                battle.isTrainerBattle = true;
                                talkingToNPC = i;
                                trainerBattle = false;
                                gameState = STATE_BATTLE;
                            } else {
                                talkingToNPC = i;
                                showDialog = true;
                                dialogPage = 0;
                            }
                            break;
                        }
                    }

                    // Check procedural trainers
                    if (!showDialog) {
                        int ptcx = (int)floorf(player.position.x / 6.0f);
                        int ptcz = (int)floorf(player.position.z / 6.0f);
                        for (int dcx = -2; dcx <= 2 && !showDialog; dcx++) {
                            for (int dcz = -2; dcz <= 2; dcz++) {
                                int cellX = ptcx + dcx;
                                int cellZ = ptcz + dcz;
                                int speciesId, level, nameIdx, colorIdx;
                                if (!ShouldPlaceTrainer(cellX, cellZ, &speciesId, &level,
                                                        &nameIdx, &colorIdx))
                                    continue;
                                unsigned int thash = GetTrainerHash(cellX, cellZ);
                                if (IsTrainerDefeated(thash)) continue;

                                unsigned int ph = HashInt(cellX * 7 + 123, cellZ * 11 + 456);
                                float offX = ((float)(ph & 0xffff) / 65536.0f - 0.5f) * 5.0f;
                                float offZ = ((float)((ph >> 16) & 0xffff) / 65536.0f - 0.5f) * 5.0f;
                                float wt = (float)GetTime() * (0.5f + ((float)(ph & 0xf) / 16.0f) * 0.8f);
                                offX += sinf(wt + (float)cellX) * 2.5f;
                                offZ += cosf(wt + (float)cellZ) * 2.5f;
                                float ptx = (float)cellX * 6.0f + offX;
                                float ptz = (float)cellZ * 6.0f + offZ;
                                float pdx = player.position.x - ptx;
                                float pdz = player.position.z - ptz;
                                if (sqrtf(pdx * pdx + pdz * pdz) < 6.0f) {
                                    Pet trainerPet = CreatePet(speciesId, level);
                                    InitBattle(&battle, playerTeam, teamSize, activePetIdx, &trainerPet);
                                    battle.isTrainerBattle = true;
                                    talkingToNPC = -1;
                                    trainerBattle = true;
                                    trainerCellX = cellX;
                                    trainerCellZ = cellZ;
                                    gameState = STATE_BATTLE;
                                    showDialog = false;
                                    showShop = false;
                                    break;
                                }
                            }
                        }
                    }
                }
                }
            }

            // ---- Open pet bag ----
            if (bJustPressed) {
                gameState = STATE_PET_BAG;
                break;
            }

            // ---- Back to menu ----
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = STATE_MENU;
                break;
            }

            // ---- NPC patrol movement ----
            {
                NPC *npcs = GetNPCs();
                int npcCount = GetNPCCount();
                for (int i = 0; i < npcCount; i++) {
                    if (!npcs[i].isTrainer || npcs[i].defeated) continue;
                    // Patrol in a small circle around base position
                    float t = (float)GetTime();
                    float radius = 4.0f + (float)i * 0.5f;
                    float speed = 0.3f + (float)i * 0.1f;
                    float phase = t * speed + (float)i;
                    npcs[i].position.x = npcs[i].baseX + cosf(phase) * radius;
                    npcs[i].position.z = npcs[i].baseZ + sinf(phase) * radius;
                }
            }

            // ---- Render ----
            BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                DrawTerrain(&terrain);

                // Encounter warning "!" above player
                if (encounter.warningActive) {
                    float bob = sinf((float)GetTime() * 6.0f) * 0.3f;
                    float wy = player.position.y + 3.5f + bob;
                    DrawCylinder((Vector3){player.position.x, wy, player.position.z},
                                 0.04f, 0.04f, 0.8f, 6, (Color){255, 255, 50, 220});
                    DrawSphere((Vector3){player.position.x, wy + 1.0f, player.position.z},
                               0.2f, (Color){255, 50, 50, 230});
                }

                // Player glow ring on ground
                DrawCylinderWires((Vector3){player.position.x, player.position.y + 0.1f, player.position.z},
                                  0.9f, 0.9f, 0.02f, 16, (Color){255, 255, 100, 200});

                // Draw player character model (feet at ground level)
                Color playerColor = teamSize > 0
                    ? PET_DATABASE[playerTeam[activePetIdx].speciesId].color
                    : RED;
                Vector3 playerDrawPos = {player.position.x, player.position.y, player.position.z};
                DrawPlayerModel(playerDrawPos, playerColor, 1.6f,
                                player.facing, isMoving, walkAnimTime);

                // Draw NPCs
                NPC *npcs = GetNPCs();
                int npcCount = GetNPCCount();
                Vector2 npcScreenPos[16];
                const char *npcNames[16];
                Vector3 npcWorldPos[16];
                Color npcColors[16];
                int npcVisibleCount = 0;

                for (int i = 0; i < npcCount; i++) {
                    NPC *npc = &npcs[i];
                    if (npc->isTrainer && npc->defeated) continue;
                    float npcY = GetTerrainHeight(&terrain, npc->position.x, npc->position.z);
                    npc->position.y = npcY;

                    Color bc = npc->color;

                    // Compute NPC facing angle
                    float npcFacing = 0.0f;
                    if (npc->isTrainer) {
                        // Trainer patrols - face tangent to patrol circle (direction of movement)
                        float t2 = (float)GetTime();
                        float sp2 = 0.3f + (float)i * 0.1f;
                        float ph2 = t2 * sp2 + (float)i;
                        npcFacing = ph2;
                    } else {
                        // Civilians face toward player
                        float dx2 = player.position.x - npc->position.x;
                        float dz2 = player.position.z - npc->position.z;
                        npcFacing = atan2f(-dx2, dz2);
                    }

                    // Ground platform disc
                    DrawCylinder((Vector3){npc->position.x, npcY + 0.02f, npc->position.z},
                                 1.2f, 1.2f, 0.06f, 16, bc);

                    // Draw NPC character model (feet at ground level)
                    Vector3 npcDrawPos = {npc->position.x, npcY, npc->position.z};
                    int npcCharType = -1;
                    if (!npc->isTrainer) {
                        if (strcmp(npc->name, "老爷爷") == 0) npcCharType = CHAR_MODEL_OLD;
                        else if (strcmp(npc->name, "研究员") == 0) npcCharType = CHAR_MODEL_RESEARCHER;
                        else if (strcmp(npc->name, "护士") == 0) npcCharType = CHAR_MODEL_NURSE;
                    }
                    DrawNPCModel(npcDrawPos, bc, npc->isTrainer, 1.6f, npcFacing, npcCharType);

                    // Beacon above head (distinct for nurse: taller + heart/cross glow)
                    bool isNurse = (!npc->isTrainer && strcmp(npc->name, "护士") == 0);
                    float beaconBase = isNurse ? npcY + 5.0f : npcY + 4.5f;
                    float beaconTop = isNurse ? npcY + 10.0f : npcY + 8.5f;
                    Color beaconCol = isNurse ? (Color){255, 150, 180, 200} : (Color){bc.r, bc.g, bc.b, 150};
                    float beaconH = beaconTop - beaconBase;
                    DrawCylinder((Vector3){npc->position.x, beaconBase, npc->position.z},
                                 0.06f, 0.06f, beaconH, 6, beaconCol);
                    DrawSphere((Vector3){npc->position.x, beaconTop, npc->position.z},
                               isNurse ? 0.45f : 0.3f, isNurse ? (Color){255, 150, 180, 255} : bc);
                    DrawSphere((Vector3){npc->position.x, beaconTop, npc->position.z},
                               isNurse ? 1.0f : 0.7f, (Color){bc.r, bc.g, bc.b, 40});

                    if (npcVisibleCount < 16) {
                        npcWorldPos[npcVisibleCount] = npc->position;
                        npcScreenPos[npcVisibleCount] = GetWorldToScreen(
                            (Vector3){npc->position.x, npcY + 9.5f, npc->position.z}, camera);
                        npcNames[npcVisibleCount] = npc->name;
                        npcColors[npcVisibleCount] = npc->color;
                        npcVisibleCount++;
                    }
                }

                // ---- Procedural trainers (generated from world cells) ----
                {
                    static const char *trainerNames[] = {
                        "训练师阿明", "训练师小红", "训练师阿力",
                        "训练师小兰", "训练师大壮", "训练师灵灵",
                        "训练师风哥", "训练师雨姐"
                    };
                    static const int trainerNameCount = 8;
                    static Color trainerColors[] = {
                        {255, 180, 50, 255}, {255, 100, 130, 255},
                        {100, 200, 255, 255}, {180, 255, 100, 255},
                        {255, 220, 80, 255}, {200, 150, 255, 255},
                        {100, 255, 200, 255}, {255, 160, 100, 255}
                    };
                    int tcx = (int)floorf(camera.target.x / 6.0f);
                    int tcz = (int)floorf(camera.target.z / 6.0f);
                    int tRange = 8;

                    for (int tcx2 = tcx - tRange; tcx2 <= tcx + tRange; tcx2++) {
                        for (int tcz2 = tcz - tRange; tcz2 <= tcz + tRange; tcz2++) {
                            int speciesId, level, nameIdx, colorIdx;
                            if (!ShouldPlaceTrainer(tcx2, tcz2, &speciesId, &level,
                                                    &nameIdx, &colorIdx))
                                continue;
                            unsigned int thash = GetTrainerHash(tcx2, tcz2);
                            if (IsTrainerDefeated(thash)) continue;

                            // Deterministic position within cell + random walk
                            unsigned int ph = HashInt(tcx2 * 7 + 123, tcz2 * 11 + 456);
                            float offX = ((float)(ph & 0xffff) / 65536.0f - 0.5f) * 5.0f;
                            float offZ = ((float)((ph >> 16) & 0xffff) / 65536.0f - 0.5f) * 5.0f;
                            // Gentle random walk
                            float wt = (float)GetTime() * (0.5f + ((float)(ph & 0xf) / 16.0f) * 0.8f);
                            offX += sinf(wt + (float)tcx2) * 2.5f;
                            offZ += cosf(wt + (float)tcz2) * 2.5f;

                            float tx = (float)tcx2 * 6.0f + offX;
                            float tz = (float)tcz2 * 6.0f + offZ;
                            float ty = GetTerrainHeight(&terrain, tx, tz);

                            Color tc = trainerColors[colorIdx % 8];
                            Vector3 tpos = {tx, ty, tz};

                            // Platform disc
                            DrawCylinder((Vector3){tx, ty + 0.02f, tz},
                                         1.2f, 1.2f, 0.06f, 16, tc);
                            // Trainer model
                            float tfacing = wt * 0.7f;
                            DrawNPCModel(tpos, tc, true, 1.6f, tfacing, -1);
                            // Beacon
                            DrawCylinder((Vector3){tx, ty + 4.5f, tz},
                                         0.05f, 0.05f, 4.0f, 6, (Color){tc.r, tc.g, tc.b, 150});
                            DrawSphere((Vector3){tx, ty + 8.5f, tz}, 0.3f, tc);
                            DrawSphere((Vector3){tx, ty + 8.5f, tz}, 0.7f, (Color){tc.r, tc.g, tc.b, 40});

                            Vector2 tScreenPos = GetWorldToScreen(
                                (Vector3){tx, ty + 9.5f, tz}, camera);
                            int tsw = GetScreenWidth();
                            int tsh = GetScreenHeight();
                            if (tScreenPos.x > 0 && tScreenPos.x < tsw &&
                                tScreenPos.y > 0 && tScreenPos.y < tsh) {
                                char tlabel[64];
                                snprintf(tlabel, sizeof(tlabel), "%s Lv.%d",
                                    trainerNames[nameIdx % trainerNameCount], level);
                                DrawTextEx(g_uiFont, tlabel,
                                    (Vector2){tScreenPos.x - 30, tScreenPos.y}, 14, 1, WHITE);
                            }
                        }
                    }
                }

                // ---- Procedural scenery (infinite, generated around camera) ----
                {
                    // Determine visible cell range (~60 unit radius)
                    int camCX = (int)floorf(camera.target.x / 6.0f);
                    int camCZ = (int)floorf(camera.target.z / 6.0f);
                    int cellRange = 14;

                    for (int cx = camCX - cellRange; cx <= camCX + cellRange; cx++) {
                        for (int cz = camCZ - cellRange; cz <= camCZ + cellRange; cz++) {
                            float offX, offZ;

                            if (ShouldPlaceTree(&terrain, cx, cz, &offX, &offZ)) {
                                float tx = (float)cx * 6.0f + offX;
                                float tz = (float)cz * 6.0f + offZ;
                                float ty = GetTerrainHeight(&terrain, tx, tz);
                                if (ty < 1.0f) continue;
                                DrawCylinder((Vector3){tx, ty, tz},
                                             0.6f, 0.7f, 4.5f, 8, (Color){80, 50, 30, 255});
                                DrawSphere((Vector3){tx, ty + 3.8f, tz},
                                           2.0f, (Color){30, 140, 40, 255});
                                DrawSphere((Vector3){tx + 0.7f, ty + 3.0f, tz + 0.5f},
                                           1.4f, (Color){40, 160, 50, 255});
                            }

                            if (ShouldPlaceRock(&terrain, cx, cz, &offX, &offZ)) {
                                float rx = (float)cx * 6.0f + offX;
                                float rz = (float)cz * 6.0f + offZ;
                                float ry = GetTerrainHeight(&terrain, rx, rz);
                                if (ry < 0.5f) continue;
                                DrawSphere((Vector3){rx, ry + 0.9f, rz},
                                           1.2f, (Color){120, 120, 130, 255});
                                DrawSphere((Vector3){rx + 0.6f, ry + 0.6f, rz - 0.4f},
                                           0.7f, (Color){140, 140, 150, 255});
                            }
                        }
                    }
                }

                // ---- Visible wild pets ----
                for (int wp = 0; wp < wildPetCount; wp++) {
                    Vector3 wpos = wildPets[wp].pos;
                    float wdist = sqrtf(
                        (wpos.x - player.position.x) * (wpos.x - player.position.x) +
                        (wpos.z - player.position.z) * (wpos.z - player.position.z));
                    // Exclamation mark when close
                    if (wdist < 4.0f) {
                        float bob2 = sinf((float)GetTime() * 5.0f) * 0.2f;
                        DrawCylinder((Vector3){wpos.x, wpos.y + 1.8f + bob2, wpos.z},
                                     0.02f, 0.02f, 0.6f, 4, (Color){255, 255, 50, 255});
                        DrawSphere((Vector3){wpos.x, wpos.y + 2.5f + bob2, wpos.z},
                                   0.12f, (Color){255, 50, 50, 255});
                    }
                    // Pet model (small scale)
                    DrawPetBattleModel(wpos, wildPets[wp].species,
                                       1.3f, false, false, (float)GetTime(), TYPE_NORMAL, 0);
                }

                // ---- Magic Spring ----
                {
                    float sy = GetTerrainHeight(&terrain, magicSpring.position.x, magicSpring.position.z);
                    magicSpring.position.y = sy + 0.1f;
                    DrawMagicSpring(&magicSpring);

                    // Spring label
                    Vector2 slab = GetWorldToScreen(
                        (Vector3){magicSpring.position.x, sy + 9.5f, magicSpring.position.z}, camera);
                    int ssw = GetScreenWidth(), ssh = GetScreenHeight();
                    if (slab.x > 0 && slab.x < ssw && slab.y > 0 && slab.y < ssh) {
                        DrawTextEx(g_uiFont, "魔法泉水",
                            (Vector2){slab.x - 28, slab.y}, 16, 1, (Color){100, 210, 255, 255});
                    }
                }

                // Merchant — full character model + beacon
                if (merchantActive) {
                    float my = GetTerrainHeight(&terrain, merchantCX, merchantCZ);
                    Color mc = {255, 180, 40, 255};

                    // Ground platform disc
                    DrawCylinder((Vector3){merchantCX, my + 0.02f, merchantCZ},
                                 1.4f, 1.4f, 0.06f, 16, mc);

                    // Merchant character model (gold body, civilian style)
                    Vector3 mpos = {merchantCX, my, merchantCZ};
                    float mfacing = atan2f(
                        player.position.x - merchantCX,
                        -(player.position.z - merchantCZ));
                    DrawNPCModel(mpos, mc, false, 1.6f, mfacing, CHAR_MODEL_MERCHANT);

                    // Merchant beacon — tall gold beam, very visible
                    DrawCylinder((Vector3){merchantCX, my + 3.5f, merchantCZ},
                                 0.07f, 0.07f, 4.0f, 6, (Color){255, 200, 50, 180});
                    DrawSphere((Vector3){merchantCX, my + 7.5f, merchantCZ},
                               0.4f, (Color){255, 200, 50, 255});
                    DrawSphere((Vector3){merchantCX, my + 7.5f, merchantCZ},
                               1.0f, (Color){255, 200, 50, 50});

                    // Merchant name label projected to screen (always show if on screen)
                    Vector2 mlab = GetWorldToScreen(
                        (Vector3){merchantCX, my + 8.5f, merchantCZ}, camera);
                    int msw = GetScreenWidth();
                    int msh = GetScreenHeight();
                    if (mlab.x > 0 && mlab.x < msw && mlab.y > 0 && mlab.y < msh) {
                        DrawTextEx(g_uiFont, "行商",
                            (Vector2){mlab.x - 18, mlab.y}, 18, 1, (Color){255, 220, 50, 255});
                    }
                }

                DrawGrid(24, 10.0f);

            EndMode3D();

            // ---- 2D labels: project 3D positions to screen ----
            // Player label
            Vector2 playerLabelPos = GetWorldToScreen(
                (Vector3){player.position.x, player.position.y + 3.0f, player.position.z}, camera);
            if (teamSize > 0) {
                char plabel[64];
                snprintf(plabel, sizeof(plabel), "YOU  [%s Lv.%d]",
                    playerTeam[activePetIdx].nickname, playerTeam[activePetIdx].level);
                DrawTextEx(g_uiFont, plabel,
                    (Vector2){playerLabelPos.x - 40, playerLabelPos.y}, 16, 1, (Color){255, 255, 100, 255});
            }

            // NPC labels and edge-of-screen direction indicators
            int sw2 = GetScreenWidth();
            int sh2 = GetScreenHeight();
            for (int i = 0; i < npcVisibleCount; i++) {
                Vector2 sp = npcScreenPos[i];
                bool onScreen = (sp.x > 0 && sp.x < sw2 && sp.y > 0 && sp.y < sh2);

                if (onScreen) {
                    // Only show name if NPC is in front of camera (not behind)
                    float cfx = camera.target.x - camera.position.x;
                    float cfz = camera.target.z - camera.position.z;
                    float cfLen = sqrtf(cfx * cfx + cfz * cfz);
                    if (cfLen > 0.001f) { cfx /= cfLen; cfz /= cfLen; }
                    float tx = npcWorldPos[i].x - camera.position.x;
                    float tz = npcWorldPos[i].z - camera.position.z;
                    float tLen = sqrtf(tx * tx + tz * tz);
                    if (tLen > 0.001f) { tx /= tLen; tz /= tLen; }
                    if (cfx * tx + cfz * tz > 0.0f) {
                        DrawTextEx(g_uiFont, npcNames[i],
                            (Vector2){sp.x - 20, sp.y}, 14, 1, (Color){255, 255, 255, 255});
                    }
                } else {
                    // Draw edge indicator - arrow pointing toward off-screen NPC
                    Vector2 center = {(float)sw2 / 2, (float)sh2 / 2};
                    Vector2 dir = {sp.x - center.x, sp.y - center.y};
                    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
                    if (len > 0.01f) {
                        dir.x /= len;
                        dir.y /= len;
                    }
                    // Place indicator at screen edge
                    float margin = 40;
                    Vector2 edgePos = center;
                    float maxDist = 0;
                    // Project to each edge
                    if (dir.x > 0) { float t = ((sw2 - margin) - center.x) / dir.x; if (t > maxDist) { maxDist = t; edgePos = (Vector2){center.x + dir.x * t, center.y + dir.y * t}; } }
                    if (dir.x < 0) { float t = (margin - center.x) / dir.x; if (t > maxDist) { maxDist = t; edgePos = (Vector2){center.x + dir.x * t, center.y + dir.y * t}; } }
                    if (dir.y > 0) { float t = ((sh2 - margin) - center.y) / dir.y; if (t > maxDist) { maxDist = t; edgePos = (Vector2){center.x + dir.x * t, center.y + dir.y * t}; } }
                    if (dir.y < 0) { float t = (margin - center.y) / dir.y; if (t > maxDist) { maxDist = t; edgePos = (Vector2){center.x + dir.x * t, center.y + dir.y * t}; } }

                    // Draw triangle arrow
                    float angle = atan2f(dir.y, dir.x);
                    float arrowSize = 14;
                    Vector2 tip = {edgePos.x + cosf(angle) * arrowSize, edgePos.y + sinf(angle) * arrowSize};
                    Vector2 left = {edgePos.x + cosf(angle + 2.5f) * arrowSize, edgePos.y + sinf(angle + 2.5f) * arrowSize};
                    Vector2 right = {edgePos.x + cosf(angle - 2.5f) * arrowSize, edgePos.y + sinf(angle - 2.5f) * arrowSize};
                    DrawTriangle(tip, left, right, npcColors[i]);
                    DrawTriangleLines(tip, left, right, WHITE);

                    // NPC name next to arrow
                    DrawTextEx(g_uiFont, npcNames[i],
                        (Vector2){edgePos.x + cosf(angle) * 30 - 20, edgePos.y + sinf(angle) * 30 - 8},
                        12, 1, npcColors[i]);
                }
            }

            DrawWorldHUD(playerTeam, teamSize, activePetIdx, &inventory, player.position.y, player.stamina);
            {
                NPC *mmNpcs = GetNPCs();
                DrawMinimap(&terrain, player.position, player.facing,
                            mmNpcs, GetNPCCount(),
                            merchantCX, merchantCZ, merchantActive,
                            magicSpring.position.x, magicSpring.position.z);
            }

            // Healing progress bar (2D overlay)
            DrawHealingProgressBar(&magicSpring);

            // Magic spring proximity hint
            if (!magicSpring.isHealing && IsNearMagicSpring(&magicSpring, player.position)) {
                const char *hint2 = "按 E 键使用魔法泉水恢复全部 HP 和 Energy";
                Vector2 hint2Size = MeasureTextEx(g_uiFont, hint2, 18, 1);
                DrawTextEx(g_uiFont, hint2,
                    (Vector2){(GetScreenWidth() - hint2Size.x) / 2, GetScreenHeight() - 110.0f},
                    18, 1, (Color){100, 210, 255, 255});
            }

            // Merchant proximity hint
            if (merchantActive && !showShop) {
                float mdx = player.position.x - merchantCX;
                float mdz = player.position.z - merchantCZ;
                if (sqrtf(mdx * mdx + mdz * mdz) < 5.0f) {
                    const char *hint = "按 E 键与行商交易";
                    Vector2 hintSize = MeasureTextEx(g_uiFont, hint, 18, 1);
                    DrawTextEx(g_uiFont, hint,
                        (Vector2){(GetScreenWidth() - hintSize.x) / 2, GetScreenHeight() - 80.0f},
                        18, 1, (Color){255, 220, 50, 255});
                }
            }

            // Enable cursor when shop opens (transition only)
            if (showShop && !wasShopOpen) EnableCursor();

            if (showShop) {
                int shopAct = DrawShopUI(shopNpcType, &inventory, merchantActive ? &merchantShop : NULL);
                if (shopAct >= 100) {
                    // Merchant skill pack
                    ShopItemType st = merchantShop.packs[shopAct - 100].type;
                    int pr = merchantShop.packs[shopAct - 100].price;
                    ShopBuySkillPack(st, pr, &inventory, &playerTeam[activePetIdx]);
                } else if (shopAct >= 0 && shopNpcType == 0) {
                    // Nurse shop
                    ShopBuy(NURSE_SHOP[shopAct].type, NURSE_SHOP[shopAct].price, &inventory);
                }
                // Only close with E if shop was already open at frame start
                if (ePressed && wasShopOpen) {
                    showShop = false;
                    showDialog = false;
                    talkingToNPC = -1;
                    DisableCursor();
                }
            } else if (showDialog && talkingToNPC >= 0) {
                NPC *myNpcs = GetNPCs();
                NPC *npc = &myNpcs[talkingToNPC];
                // Use multi-page dialog tree if available
                const char *dtext;
                if (npc->dialogCount > 1 && dialogPage < npc->dialogCount) {
                    dtext = npc->dialogs[dialogPage];
                } else if (npc->hasQuest && dialogPage >= npc->dialogCount) {
                    static char questBuf[128];
                    if (npc->questComplete) {
                        snprintf(questBuf, sizeof(questBuf), "任务完成了！再来一次吧。");
                        npc->questProgress = 0;
                        npc->questComplete = false;
                        npc->questAccepted = false;
                        // Randomize next quest
                        if (npc->questType == 0) {
                            npc->questTarget = 2 + rand() % 4;  // 2-5
                        } else {
                            npc->questTarget = 1 + rand() % 4;  // 1-4
                        }
                        npc->questReward = npc->questTarget * 100;
                    } else {
                        npc->questAccepted = true;  // mark quest as accepted
                        if (npc->questType == 0) {
                            snprintf(questBuf, sizeof(questBuf), "任务：捕捉 %d 只野生宠物。[%d/%d]",
                                     npc->questTarget, npc->questProgress, npc->questTarget);
                        } else {
                            snprintf(questBuf, sizeof(questBuf), "任务：击败 %d 名训练师。[%d/%d]",
                                     npc->questTarget, npc->questProgress, npc->questTarget);
                        }
                    }
                    dtext = questBuf;
                } else {
                    dtext = npc->dialog;
                }
                DrawNPCDialogBox(npc->name, dtext);
                // Page indicator
                if (npc->dialogCount > 1) {
                    char pageLabel[64];
                    int total = npc->dialogCount + (npc->hasQuest ? 1 : 0);
                    snprintf(pageLabel, sizeof(pageLabel), "[%d/%d] 按E继续", dialogPage + 1, total);
                    int pageSw = GetScreenWidth(), pageSh = GetScreenHeight();
                    DrawTextEx(g_uiFont, pageLabel, (Vector2){(float)pageSw - 150, (float)pageSh - 30}, 14, 1, (Color){150, 150, 170, 255});
                }
            }

            DrawNotification(&g_notification);
            EndDrawing();
            break;
        }

        // ======== BATTLE ========
        case STATE_BATTLE: {
            if (prevState != STATE_BATTLE) {
                prevState = STATE_BATTLE;
                EnableCursor();
                PlayBGM(&audio, BGM_BATTLE);
                battleEndTime = 0.0;
                battleResolved = false;
                ClearEncounterWarning(&encounter);
                showItemMenu = false;
                wasItemMenuOpen = false;
                showReviveSelect = false;
            }

            int action = -1;

            // Capture H once (edge-triggered, must not call IsKeyPressed twice)
            bool hPressed = IsKeyPressed(KEY_H);

            // Handle keyboard shortcuts for moves
            if (BattleIsPlayerTurn(&battle)) {
                if (!showItemMenu) {
                    for (int k = KEY_ONE; k <= KEY_FOUR; k++) {
                        if (IsKeyPressed(k)) { action = k - KEY_ONE; break; }
                    }
                    if (IsKeyPressed(KEY_C)) action = 4;   // catch
                    if (IsKeyPressed(KEY_R)) action = 5;   // run
                    if (hPressed && !wasItemMenuOpen) {
                        // Count available item types
                        int itemTypes = 0;
                        if (inventory.potions > 0) itemTypes++;
                        if (inventory.superPotions > 0) itemTypes++;
                        if (inventory.ethers > 0) itemTypes++;
                        if (itemTypes == 0) {
                            action = 6; // will show "no items" via BattleUseItem
                        } else {
                            showItemMenu = true;
                        }
                    }
                }
            }

            BeginDrawing();
            DrawBattleScene(&battle);

            // Item selection popup
            if (showItemMenu) {
                int sw = GetScreenWidth(), sh = GetScreenHeight();
                int btnW = 140, btnH = 36, gap = 10;
                int popW = 3 * btnW + 4 * gap;
                int popH = btnH + 10;
                int px = (sw - popW) / 2, py = sh - 130;
                DrawRectangle(px - 2, py - 2, popW + 4, popH + 4, (Color){0,0,0,200});
                DrawRectangle(px, py, popW, popH, (Color){30,35,60,240});

                int bx = px + gap;
                // Potion button
                if (inventory.potions <= 0) GuiDisable();
                char potLabel[32];
                snprintf(potLabel, sizeof(potLabel), "1.治疗药水 x%d", inventory.potions);
                if (GuiButton((Rectangle){(float)bx, (float)py + 5, (float)btnW, (float)btnH}, potLabel)
                    || IsKeyPressed(KEY_ONE)) action = 6;
                if (inventory.potions <= 0) GuiEnable();
                bx += btnW + gap;

                // Super potion button
                if (inventory.superPotions <= 0) GuiDisable();
                char supLabel[32];
                snprintf(supLabel, sizeof(supLabel), "2.超级药水 x%d", inventory.superPotions);
                if (GuiButton((Rectangle){(float)bx, (float)py + 5, (float)btnW, (float)btnH}, supLabel)
                    || IsKeyPressed(KEY_TWO)) action = 7;
                if (inventory.superPotions <= 0) GuiEnable();
                bx += btnW + gap;

                // Ether button
                if (inventory.ethers <= 0) GuiDisable();
                char ethLabel[32];
                snprintf(ethLabel, sizeof(ethLabel), "3.能量剂 x%d", inventory.ethers);
                if (GuiButton((Rectangle){(float)bx, (float)py + 5, (float)btnW, (float)btnH}, ethLabel)
                    || IsKeyPressed(KEY_THREE)) action = 8;
                if (inventory.ethers <= 0) GuiEnable();

                if (IsKeyPressed(KEY_ESCAPE) || (hPressed && wasItemMenuOpen)) {
                    showItemMenu = false;
                }
            }

            // Track item menu state for next frame (prevents same-frame open+close)
            wasItemMenuOpen = showItemMenu;

            if (action < 0 && !showItemMenu && !showReviveSelect) {
                action = UpdateBattleButtons(&battle);
            }

            // ---- Revive selection UI ----
            if (showReviveSelect) {
                int sw2 = GetScreenWidth(), sh2 = GetScreenHeight();
                DrawRectangle(0, 0, sw2, sh2, (Color){0, 0, 0, 140});

                int popW = 380, popH = 40;
                int faintedCount = 0;
                for (int fi = 0; fi < teamSize; fi++)
                    if (IsFainted(&playerTeam[fi])) faintedCount++;
                popH = 50 + faintedCount * 32 + 40;

                int px = (sw2 - popW) / 2, py = (sh2 - popH) / 2;
                DrawRectangle(px - 3, py - 3, popW + 6, popH + 6, (Color){255, 200, 50, 60});
                DrawRectangle(px, py, popW, popH, (Color){15, 20, 40, 245});
                DrawRectangleLines(px, py, popW, popH, (Color){255, 200, 50, 200});

                DrawTextEx(g_uiFont, "使用复活令牌？", (Vector2){(float)px + 10, (float)py + 8}, 20, 1, (Color){255, 220, 50, 255});
                char revLabel[32];
                snprintf(revLabel, sizeof(revLabel), "剩余: %d", inventory.revives);
                DrawTextEx(g_uiFont, revLabel, (Vector2){(float)px + popW - 90, (float)py + 12}, 14, 1, WHITE);

                int btnY = py + 40;
                for (int i = 0; i < teamSize; i++) {
                    if (!IsFainted(&playerTeam[i])) continue;
                    Rectangle btn = {(float)px + 10, (float)btnY, (float)(popW - 20), 28};
                    char petLabel[64];
                    snprintf(petLabel, sizeof(petLabel), "[%d] %s  Lv.%d  HP全满",
                             i + 1, playerTeam[i].nickname, playerTeam[i].level);
                    if (GuiButton(btn, petLabel) || IsKeyPressed(KEY_ONE + i)) {
                        UseRevive(&inventory);
                        HealPet(&playerTeam[i]);
                        activePetIdx = i;
                        battle.activePetIdx = i;
                        battle.needsRevive = false;
                        battle.waitingForInput = true;
                        battle.isPlayerTurn = true;
                        battle.selectedMove = -1;
                        snprintf(battle.message, sizeof(battle.message), "%s 满血复活了！",
                                 playerTeam[i].nickname);
                        showReviveSelect = false;
                    }
                    btnY += 32;
                }

                // Refuse revive
                if (GuiButton((Rectangle){(float)px + 10, (float)py + popH - 34, 160, 28}, "放弃 [ESC]")
                    || IsKeyPressed(KEY_ESCAPE)) {
                    battle.battleOver = true;
                    battle.playerWon = false;
                    showReviveSelect = false;
                }
            }

            EndDrawing();

            // If item menu was open and an action was selected, close it
            if (action >= 6 && showItemMenu) showItemMenu = false;

            // Process battle actions
            if (activePetIdx < 0 || activePetIdx >= teamSize) activePetIdx = 0;
            if (action >= 0 && action <= 3) {
                Pet *pp = &playerTeam[activePetIdx];
                if (pp->moveIds[action] >= 0 &&
                    (pp->skillPacks[action] > 0 || pp->sharedPP >= MOVE_DATABASE[pp->moveIds[action]].ppCost)) {
                    BattleSelectMove(&battle, action);
                }
            } else if (action == 4) {
                BattleTryCatch(&battle, &inventory);
            } else if (action == 5) {
                BattleTryRun(&battle);
            } else if (action == 6) {
                BattleUseItem(&battle, &inventory, BITEM_POTION);
            } else if (action == 7) {
                BattleUseItem(&battle, &inventory, BITEM_SUPERPOTION);
            } else if (action == 8) {
                BattleUseItem(&battle, &inventory, BITEM_ETHER);
            }

            // Execute turn if needed
            if (BattleNeedsUpdate(&battle)) {
                BattleExecuteTurn(&battle);
            }

            // Auto-switch: if active pet fainted but others alive, switch immediately
            if (activePetIdx >= 0 && activePetIdx < teamSize &&
                IsFainted(&playerTeam[activePetIdx]) && !battle.battleOver && !battle.needsRevive) {
                int liveIdx = -1;
                for (int i = 0; i < teamSize; i++) {
                    if (!IsFainted(&playerTeam[i])) { liveIdx = i; break; }
                }
                if (liveIdx >= 0) {
                    BattleSwitchPet(&battle, liveIdx);
                    activePetIdx = liveIdx;
                }
            }

            // Revive check — all pets fainted but player has revive tokens
            if (battle.needsRevive && !battle.battleOver) {
                if (inventory.revives > 0) {
                    showReviveSelect = true;
                } else {
                    battle.battleOver = true;
                    battle.playerWon = false;
                    showReviveSelect = false;
                }
            }

            // Battle over — auto-exit after 1.5s
            if (BattleIsOver(&battle)) {
                if (!battleResolved) {
                    battleEndTime = GetTime();
                    battleResolved = true;
                }
                if (GetTime() - battleEndTime > 1.5) {
                    if (battle.playerWon && !battle.playerRan && activePetIdx >= 0 && activePetIdx < teamSize) {
                        Pet *activePet = &playerTeam[activePetIdx];
                        GainExp(activePet, battle.rewardExp);
                        inventory.gold += 30 + battle.enemyPet.level * 5;
                        if (battle.caught && teamSize < MAX_TEAM_SIZE) {
                            battle.enemyPet.currentHP = battle.enemyPet.maxHP;
                            HealPet(&battle.enemyPet);
                            playerTeam[teamSize] = battle.enemyPet;
                            teamSize++;
                            // Quest progress: catch wild pets
                            NPC *qnpcs = GetNPCs();
                            for (int qi = 0; qi < GetNPCCount(); qi++) {
                                if (qnpcs[qi].hasQuest && qnpcs[qi].questType == 0 && !qnpcs[qi].questComplete) {
                                    if (qnpcs[qi].questProgress < qnpcs[qi].questTarget) {
                                        qnpcs[qi].questProgress++;
                                        if (qnpcs[qi].questProgress >= qnpcs[qi].questTarget) {
                                            inventory.gold += qnpcs[qi].questReward;
                                            qnpcs[qi].questComplete = true;
                                            char notifyBuf[128];
                                            snprintf(notifyBuf, sizeof(notifyBuf), "任务完成! 获得 %d 金币!", qnpcs[qi].questReward);
                                            ShowNotification(&g_notification, notifyBuf);
                                        }
                                    }
                                }
                            }
                        }
                        if (talkingToNPC >= 0) {
                            NPC *npcs = GetNPCs();
                            int nc2 = GetNPCCount();
                            if (talkingToNPC < nc2 && npcs[talkingToNPC].isTrainer)
                                npcs[talkingToNPC].defeated = true;
                        } else if (trainerBattle) {
                            MarkTrainerDefeated(GetTrainerHash(trainerCellX, trainerCellZ));
                        }
                        // Quest progress: defeat trainers (only for trainer battles)
                        {
                            NPC *npcsForCheck = GetNPCs();
                            bool isTrainerBattle = (talkingToNPC >= 0 && talkingToNPC < GetNPCCount() && npcsForCheck[talkingToNPC].isTrainer)
                                                   || trainerBattle;
                            if (isTrainerBattle) {
                                NPC *qnpcs2 = GetNPCs();
                                for (int qi = 0; qi < GetNPCCount(); qi++) {
                                    if (qnpcs2[qi].hasQuest && qnpcs2[qi].questType == 1 && !qnpcs2[qi].questComplete) {
                                        if (qnpcs2[qi].questProgress < qnpcs2[qi].questTarget) {
                                            qnpcs2[qi].questProgress++;
                                            if (qnpcs2[qi].questProgress >= qnpcs2[qi].questTarget) {
                                                inventory.gold += qnpcs2[qi].questReward;
                                                qnpcs2[qi].questComplete = true;
                                                char notifyBuf[128];
                                                snprintf(notifyBuf, sizeof(notifyBuf), "任务完成! 获得 %d 金币!", qnpcs2[qi].questReward);
                                                ShowNotification(&g_notification, notifyBuf);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        talkingToNPC = -1;
                        trainerBattle = false;
                        AutoSave(SAVE_PATH, &player, playerTeam, teamSize,
                                 activePetIdx, &inventory, playTime);
                        gameState = STATE_WORLD;
                    } else if (battle.playerRan) {
                        talkingToNPC = -1;
                        trainerBattle = false;
                        gameState = STATE_WORLD;
                    } else {
                        // True game over — delete save so Continue is unavailable
                        talkingToNPC = -1;
                        trainerBattle = false;
                        remove(SAVE_PATH);
                        gameState = STATE_GAME_OVER;
                    }
                }
            } else if (IsKeyPressed(KEY_ESCAPE)) {
                // Can't escape battle with ESC
            }

            break;
        }

        // ======== PET BAG ========
        case STATE_PET_BAG: {
            if (prevState != STATE_PET_BAG) {
                prevState = STATE_PET_BAG;
                EnableCursor();
            }

            BeginDrawing();
            DrawPetBagScene(playerTeam, teamSize, activePetIdx);
            int newIdx = UpdatePetBagButtons(playerTeam, teamSize, activePetIdx);
            EndDrawing();

            if (newIdx >= 0 && newIdx < teamSize) {
                activePetIdx = newIdx;
            }

            // Release pet (return 100 + index)
            if (newIdx >= 100 && newIdx < 100 + teamSize) {
                int releaseIdx = newIdx - 100;
                if (teamSize > 1) {
                    // Shift array to remove the released pet
                    for (int r = releaseIdx; r < teamSize - 1; r++) {
                        playerTeam[r] = playerTeam[r + 1];
                    }
                    teamSize--;
                    if (activePetIdx >= teamSize) activePetIdx = teamSize - 1;
                    if (activePetIdx > releaseIdx) activePetIdx--;
                    // Update battle's active index
                    battle.activePetIdx = activePetIdx;
                }
            }

            if (bJustPressed) {
                gameState = STATE_WORLD;
                DisableCursor();
            }
            break;
        }

        // ======== GAME OVER ========
        case STATE_GAME_OVER: {
            if (prevState != STATE_GAME_OVER) {
                prevState = STATE_GAME_OVER;
                EnableCursor();
            }

            BeginDrawing();
            DrawGameOverScene(battle.playerWon);
            int goAction = UpdateGameOverButtons();
            EndDrawing();

            if (goAction == 1) {
                gameState = STATE_MENU;
            }
            break;
        }

        // ======== BESTIARY ========
        case STATE_BESTIARY: {
            if (prevState != STATE_BESTIARY) {
                prevState = STATE_BESTIARY;
                EnableCursor();
            }

            BeginDrawing();
            DrawBestiaryScene();
            int bestAction = UpdateBestiaryButtons();
            EndDrawing();

            if (bestAction == 1 || IsKeyPressed(KEY_ESCAPE)) {
                gameState = STATE_MENU;
            }
            break;
        }

        } // end switch

        // ---- Audio update (every frame) ----
        UpdateAudioManager(&audio);

        // ---- Window close handling for non-menu states ----
        if (WindowShouldClose() && gameState != STATE_MENU) {
            AutoSave(SAVE_PATH, &player, playerTeam, teamSize,
                     activePetIdx, &inventory, playTime);
            break;
        }
    }

    // ---- Cleanup ----
    UnloadCharacterModels();
    UnloadPetModels();
    UnloadAudioManager(&audio);
    UnloadTerrain(&terrain);
    CloseWindow();
    return 0;
}
