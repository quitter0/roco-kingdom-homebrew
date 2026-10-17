#ifndef UI_MENU_H
#define UI_MENU_H

#include "raylib.h"
#include "game_state.h"
#include "pet.h"
#include "inventory.h"
#include "battle.h"
#include "npc.h"
#include "save.h"
#include "models_3d.h"
#include "shop.h"
#include "terrain.h"

extern Font g_uiFont;

void InitUI(void);

// Main Menu - returns: 0=nothing, 1=new game, 2=continue, 3=quit
int UpdateMenuButtons(bool canContinue);
void DrawMenu(void);

// World HUD
void DrawWorldHUD(const Pet *team, int teamSize, int activeIdx, const Inventory *inv, float playerY, float stamina);

// Battle UI - returns: -1=nothing, 0-3=move, 4=catch, 5=run, 6=potion
int UpdateBattleButtons(BattleState *b);
void DrawBattleScene(const BattleState *b);

// Pet Bag UI - returns: -1=nothing, 0-5=new active index
int UpdatePetBagButtons(Pet *team, int teamSize, int activeIdx);
void DrawPetBagScene(const Pet *team, int teamSize, int activeIdx);

// Game Over - returns: 0=nothing, 1=back to menu
int UpdateGameOverButtons(void);
void DrawGameOverScene(bool playerWon);

// NPC Dialog
void DrawNPCDialogBox(const char *name, const char *dialog);

// Shop UI — returns: >=0 item index, 100+=merchant pack index, -1=nothing
int DrawShopUI(int shopType, Inventory *inv, const MerchantShop *merchant);

// Minimap — rendered in world state, top-left corner
void DrawMinimap(Terrain *t, Vector3 playerPos, float playerFacing,
                 const NPC *npcs, int npcCount,
                 float merchantCX, float merchantCZ, bool merchantActive,
                 float springX, float springZ);

// Notification toast system
#define NOTIFY_MSG_LEN 128
#define NOTIFY_DURATION 2.5f

typedef struct {
    char message[NOTIFY_MSG_LEN];
    float timer;
    bool active;
} Notification;

void ShowNotification(Notification *n, const char *msg);
void DrawNotification(const Notification *n);

// Bestiary UI
void DrawBestiaryScene(void);
int UpdateBestiaryButtons(void);

// Starter selection UI — returns: -1=nothing, 0-7=selected species
void DrawStarterSelectScene(void);
int UpdateStarterSelectButtons(void);

#endif
