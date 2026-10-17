#include "battle.h"
#include "pet_data.h"
#include "audio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============ Type chart ============
// FIRE > GRASS (1.6), WATER > FIRE (1.6), GRASS > WATER (1.6)
// LIGHT <> DARK (1.6 both ways)
// Resistance: opposite direction = 0.6
// NORMAL: 1.0 vs all

static float TypeMultiplier(PetType atk, PetType def) {
    if (atk == TYPE_FIRE   && def == TYPE_GRASS) return 1.6f;
    if (atk == TYPE_WATER  && def == TYPE_FIRE)  return 1.6f;
    if (atk == TYPE_GRASS  && def == TYPE_WATER) return 1.6f;
    if (atk == TYPE_LIGHT  && def == TYPE_DARK)  return 1.6f;
    if (atk == TYPE_DARK   && def == TYPE_LIGHT) return 1.6f;

    if (atk == TYPE_FIRE   && def == TYPE_WATER) return 0.6f;
    if (atk == TYPE_WATER  && def == TYPE_GRASS) return 0.6f;
    if (atk == TYPE_GRASS  && def == TYPE_FIRE)  return 0.6f;

    return 1.0f;
}

static const char *TypeName(PetType t) {
    switch (t) {
        case TYPE_FIRE:  return "火";
        case TYPE_WATER: return "水";
        case TYPE_GRASS: return "草";
        case TYPE_LIGHT: return "光";
        case TYPE_DARK:  return "暗";
        default:         return "普通";
    }
}

static const char *StatusName(int s) {
    switch (s) {
        case STATUS_BURN:   return "灼伤";
        case STATUS_POISON: return "中毒";
        case STATUS_FREEZE: return "冰冻";
        default: return "";
    }
}

// ============ Damage formula ============
// damage = ((2*level/5+2) * power * ATK/DEF / 15 + 2) * type * random(0.85~1.0) * crit * burn

static int CalcDamage(int level, int power, int atk, int def,
                      PetType moveType, PetType defType,
                      bool isCrit, bool attackerBurned) {
    if (power <= 0) return 0;
    float base = (2.0f * (float)level / 5.0f + 2.0f) * (float)power * (float)atk / (float)def / 15.0f + 2.0f;
    float typeMod = TypeMultiplier(moveType, defType);
    float rng = 0.85f + (float)(rand() % 16) / 100.0f; // 0.85 ~ 1.00
    float critMod = isCrit ? 1.5f : 1.0f;
    float burnMod = (attackerBurned && moveType == TYPE_NORMAL) ? 0.8f : 1.0f;
    int dmg = (int)(base * typeMod * rng * critMod * burnMod);
    if (dmg < 1) dmg = 1;
    return dmg;
}

// ============ Status tick ============

static void ApplyStatusTick(Pet *p, char *msgBuf, int bufSize) {
    if (p->status == STATUS_BURN || p->status == STATUS_POISON) {
        int dmg = p->maxHP / 16;
        if (dmg < 1) dmg = 1;
        p->currentHP -= dmg;
        char tmp[80];
        snprintf(tmp, sizeof(tmp), " %s 受到%s伤害(%d)！",
                 p->nickname, StatusName(p->status), dmg);
        int remain = bufSize - (int)strlen(msgBuf) - 1;
        if (remain > 0) strncat(msgBuf, tmp, remain);
    }
    if (p->status == STATUS_FREEZE && p->statusTimer > 0) {
        p->statusTimer--;
        if (rand() % 100 < 25) {
            p->status = STATUS_NONE;
            p->statusTimer = 0;
            char tmp[64];
            snprintf(tmp, sizeof(tmp), " %s 解冻了！", p->nickname);
            int remain = bufSize - (int)strlen(msgBuf) - 1;
            if (remain > 0) strncat(msgBuf, tmp, remain);
        }
    }
}

// ============ Effective speed (accounting for speed debuff) ============

static int EffectiveSpeed(const Pet *p) {
    if (p->speedDown) return (int)((float)p->speed * 0.7f);
    return p->speed;
}

// ============ Safe message append ============

static void SafeMsgAppend(char *dst, int dstSize, const char *src) {
    int dstLen = (int)strlen(dst);
    int remain = dstSize - dstLen - 1;
    if (remain > 0) strncat(dst, src, remain);
}

// ============ Init ============

void InitBattle(BattleState *b, Pet *team, int teamSize, int activeIdx, Pet *enemyPet) {
    b->playerTeam = team;
    b->teamSize = teamSize;
    b->activePetIdx = (activeIdx >= 0 && activeIdx < teamSize) ? activeIdx : 0;
    b->enemyPet = *enemyPet;
    b->enemyPet.speciesId = enemyPet->speciesId % PET_COUNT;
    b->enemyPet.status = STATUS_NONE;
    b->enemyPet.statusTimer = 0;
    b->enemyPet.speedDown = false;
    b->isPlayerTurn = true;
    b->waitingForInput = true;
    b->battleOver = false;
    b->playerWon = false;
    b->playerRan = false;
    b->caught = false;
    b->needsRevive = false;
    b->selectedMove = -1;
    b->messageTimer = 0;
    // Difference scaling: bonus for beating higher-level enemies
    int baseExp = 20 + enemyPet->level * 10;
    int levelDiff = enemyPet->level - b->playerTeam[b->activePetIdx].level;
    b->rewardExp = baseExp + levelDiff * 12;
    if (b->rewardExp < 1) b->rewardExp = 1;
    b->animStartTime = 0.0;
    b->animInfo = (BattleAnimInfo){0};
    b->counterPending = false;
    b->enemyActedFirst = false;

    Pet *playerPet = &b->playerTeam[b->activePetIdx];
    // Check priority first, then speed
    bool playerPriority = false, enemyPriority = false;
    // No move selected yet, compare raw speed
    if (EffectiveSpeed(playerPet) >= EffectiveSpeed(&b->enemyPet)) {
        b->isPlayerTurn = true;
        b->waitingForInput = true;
        snprintf(b->message, sizeof(b->message), "野生的 %s 出现了！%s 先手！",
                 b->enemyPet.nickname, playerPet->nickname);
    } else {
        b->isPlayerTurn = false;
        b->waitingForInput = false;
        b->enemyActedFirst = true;
        snprintf(b->message, sizeof(b->message), "野生的 %s 出现了！对方速度更快，先手攻击！",
                 b->enemyPet.nickname);
    }
}

// ============ Move selection ============

void BattleSelectMove(BattleState *b, int moveSlot) {
    if (!b->waitingForInput || b->battleOver) return;
    if (b->activePetIdx < 0 || b->activePetIdx >= b->teamSize) return;
    Pet *playerPet = &b->playerTeam[b->activePetIdx];
    if (IsFainted(playerPet)) return;  // fainted pet cannot act
    if (moveSlot < 0 || moveSlot >= 4) return;
    if (playerPet->moveIds[moveSlot] < 0) return;
    int mid = playerPet->moveIds[moveSlot];
    if (mid < 0) return;
    // Check PP: skill packs first, then shared PP
    if (playerPet->sharedPP < MOVE_DATABASE[mid].ppCost && playerPet->skillPacks[moveSlot] <= 0) return;
    b->selectedMove = moveSlot;
    b->waitingForInput = false;
}

// ============ Catch / Run / Potion ============

void BattleTryCatch(BattleState *b, Inventory *inv) {
    if (!b->waitingForInput || b->battleOver) return;

    int catchRate;
    if (UseGreatBall(inv)) catchRate = GetPetCatchRate(b->enemyPet.speciesId) + 25;
    else if (UsePetBall(inv)) catchRate = GetPetCatchRate(b->enemyPet.speciesId);
    else {
        snprintf(b->message, sizeof(b->message), "没有宠物球了！");
        return;
    }

    int hpFactor = (b->enemyPet.currentHP * 100) / b->enemyPet.maxHP;
    catchRate = catchRate * (200 - hpFactor) / 100;

    if (rand() % 100 < catchRate) {
        b->caught = true;
        b->battleOver = true;
        b->playerWon = true;
        snprintf(b->message, sizeof(b->message), "成功捕捉了 %s！", b->enemyPet.nickname);
        if (g_audio) PlaySFX(g_audio, SFX_CATCH);
    } else {
        snprintf(b->message, sizeof(b->message), "%s 挣脱了宠物球！", b->enemyPet.nickname);
        b->waitingForInput = false;
    }
}

void BattleTryRun(BattleState *b) {
    if (!b->waitingForInput || b->battleOver) return;
    if (b->activePetIdx < 0 || b->activePetIdx >= b->teamSize) return;
    int pLv = b->playerTeam[b->activePetIdx].level;
    int eLv = b->enemyPet.level;
    float runChance = 0.5f + (float)(pLv - eLv) * 0.05f;
    if (runChance < 0.1f) runChance = 0.1f;
    if (runChance > 0.95f) runChance = 0.95f;

    if ((float)rand() / (float)RAND_MAX < runChance) {
        b->playerRan = true;
        b->battleOver = true;
        snprintf(b->message, sizeof(b->message), "成功逃脱了！");
        if (g_audio) PlaySFX(g_audio, SFX_RUN);
    } else {
        snprintf(b->message, sizeof(b->message), "逃脱失败！");
        b->waitingForInput = false;
    }
}

void BattleUseItem(BattleState *b, Inventory *inv, BattleItem item) {
    if (!b->waitingForInput || b->battleOver) return;
    if (b->activePetIdx < 0 || b->activePetIdx >= b->teamSize) return;
    Pet *pp = &b->playerTeam[b->activePetIdx];
    if (IsFainted(pp)) {
        snprintf(b->message, sizeof(b->message), "%s 已阵亡，无法使用道具！", pp->nickname);
        return;
    }

    switch (item) {
        case BITEM_POTION: {
            if (inv->potions <= 0) {
                snprintf(b->message, sizeof(b->message), "没有治疗药水了！");
                return;
            }
            UsePotion(inv);
            int healAmount = pp->maxHP * 20 / 100;
            if (healAmount < 60) healAmount = 60;
            pp->currentHP += healAmount;
            if (pp->currentHP > pp->maxHP) pp->currentHP = pp->maxHP;
            snprintf(b->message, sizeof(b->message), "%s 恢复了 %d 点 HP！", pp->nickname, healAmount);
            if (g_audio) PlaySFX(g_audio, SFX_HEAL);
            break;
        }
        case BITEM_SUPERPOTION: {
            if (inv->superPotions <= 0) {
                snprintf(b->message, sizeof(b->message), "没有超级药水了！");
                return;
            }
            UseSuperPotion(inv);
            int healAmount = pp->maxHP * 40 / 100;
            if (healAmount < 120) healAmount = 120;
            pp->currentHP += healAmount;
            if (pp->currentHP > pp->maxHP) pp->currentHP = pp->maxHP;
            snprintf(b->message, sizeof(b->message), "%s 恢复了 %d 点 HP！", pp->nickname, healAmount);
            if (g_audio) PlaySFX(g_audio, SFX_HEAL);
            break;
        }
        case BITEM_ETHER: {
            if (inv->ethers <= 0) {
                snprintf(b->message, sizeof(b->message), "没有PP回复剂了！");
                return;
            }
            UseEther(inv);
            int ppRestore = pp->maxSharedPP * 30 / 100;
            if (ppRestore < 30) ppRestore = 30;
            pp->sharedPP += ppRestore;
            if (pp->sharedPP > pp->maxSharedPP) pp->sharedPP = pp->maxSharedPP;
            snprintf(b->message, sizeof(b->message), "%s 恢复了 %d 点 PP！", pp->nickname, ppRestore);
            if (g_audio) PlaySFX(g_audio, SFX_HEAL);
            break;
        }
    }
    b->waitingForInput = false;
}

// ============ Turn check ============

bool BattleNeedsUpdate(BattleState *b) {
    return !b->waitingForInput && !b->battleOver;
}

// ============ Execute single attack (player or enemy) ============

static void ExecuteAttack(Pet *attacker, Pet *defender, int moveSlot,
                          bool isPlayer, BattleState *b) {
    int moveId = attacker->moveIds[moveSlot];
    if (moveId < 0 || moveId >= (int)MOVE_COUNT) return;
    const MoveData *move = &MOVE_DATABASE[moveId];

    // Consume PP: shared pool first, then skill packs
    if (attacker->sharedPP >= move->ppCost)
        attacker->sharedPP -= move->ppCost;
    else if (attacker->skillPacks[moveSlot] > 0)
        attacker->skillPacks[moveSlot]--;

    // Heal move — no accuracy check
    if (move->flags & MOVE_FLAG_HEAL) {
        int heal = attacker->maxHP / 2;
        attacker->currentHP += heal;
        if (attacker->currentHP > attacker->maxHP) attacker->currentHP = attacker->maxHP;
        snprintf(b->message, sizeof(b->message), "%s 使用了 %s！恢复了 %d HP！",
                 attacker->nickname, move->name, heal);
        if (g_audio) PlaySFX(g_audio, SFX_HEAL);
        return;
    }

    // Accuracy check
    if ((rand() % 100) >= move->accuracy) {
        snprintf(b->message, sizeof(b->message), "%s 使用了 %s…但没有命中！",
                 attacker->nickname, move->name);
        return;
    }

    int defSpId = defender->speciesId % PET_COUNT;
    PetType defType = PET_DATABASE[defSpId].type;

    // Critical hit
    bool isCrit = false;
    int critRate = (move->flags & MOVE_FLAG_HIGH_CRIT) ? 4 : 1; // 4/16=25% vs 1/16=6.25%
    if ((rand() % 16) < critRate) isCrit = true;

    bool attackerBurned = (attacker->status == STATUS_BURN);

    int dmg = CalcDamage(attacker->level, move->basePower,
                         attacker->attack, defender->defense,
                         move->type, defType, isCrit, attackerBurned);

    defender->currentHP -= dmg;
    if (g_audio) PlaySFX(g_audio, SFX_DAMAGE);

    // Build message
    float tMul = TypeMultiplier(move->type, defType);
    char extra[160] = "";
    if (isCrit) SafeMsgAppend(extra, sizeof(extra), " 暴击！");
    if (tMul > 1.5f) SafeMsgAppend(extra, sizeof(extra), " 效果拔群！");
    else if (tMul < 0.7f) SafeMsgAppend(extra, sizeof(extra), " 效果不好…");

    snprintf(b->message, sizeof(b->message), "%s 使用了 %s！造成 %d 伤害！%s",
             attacker->nickname, move->name, dmg, extra);

    // Status effect on hit
    if (move->statusChance > 0 && defender->status == STATUS_NONE) {
        if ((rand() % 100) < move->statusChance) {
            defender->status = move->statusType;
            if (move->statusType == STATUS_FREEZE) defender->statusTimer = 3;
            char stmp[48];
            snprintf(stmp, sizeof(stmp), " %s 陷入了%s状态！",
                     defender->nickname, StatusName(move->statusType));
            SafeMsgAppend(b->message, sizeof(b->message), stmp);
        }
    }

    // Stat effect on hit
    if (move->statChance > 0 && move->statEffect == 1 && !defender->speedDown) {
        if ((rand() % 100) < move->statChance) {
            defender->speedDown = true;
            char stmp[48];
            snprintf(stmp, sizeof(stmp), " %s 的速度降低了！", defender->nickname);
            SafeMsgAppend(b->message, sizeof(b->message), stmp);
        }
    }
}

// ============ Enemy AI ============

static bool CanUseMove(const Pet *p, int slot) {
    int mid = p->moveIds[slot];
    if (mid < 0) return false;
    if (p->sharedPP >= MOVE_DATABASE[mid].ppCost) return true;
    return p->skillPacks[slot] > 0;
}

static int EnemyPickMove(BattleState *b) {
    Pet *pp = &b->playerTeam[b->activePetIdx];
    PetType pType = PET_DATABASE[pp->speciesId % PET_COUNT].type;
    Pet *ep = &b->enemyPet;

    for (int i = 0; i < 4; i++) {
        int mid = ep->moveIds[i];
        if (!CanUseMove(ep, i)) continue;
        if (TypeMultiplier(MOVE_DATABASE[mid].type, pType) > 1.5f) return i;
    }
    for (int i = 0; i < 4; i++) {
        int mid = ep->moveIds[i];
        if (!CanUseMove(ep, i)) continue;
        if (MOVE_DATABASE[mid].basePower > 0) return i;
    }
    for (int i = 0; i < 4; i++)
        if (CanUseMove(ep, i)) return i;
    return -1;
}

// ============ Main turn execution (two-phase animation) ============

void BattleExecuteTurn(BattleState *b) {
    if (b->battleOver) return;
    if (b->activePetIdx < 0 || b->activePetIdx >= b->teamSize) return;

    Pet *playerPet = &b->playerTeam[b->activePetIdx];
    Pet *enemyPet = &b->enemyPet;

    // --- Animation guard: block re-entry while animation plays ---
    if (b->animStartTime > 0.0) {
        double elapsed = GetTime() - b->animStartTime;
        double needed = (double)b->animInfo.duration;
        if (b->counterPending) needed += (double)b->counterDelay;
        if (elapsed < needed) return;
    }

    // --- After animation finished: handle counter or end turn ---
    if (b->animStartTime > 0.0 && !b->counterPending) {
        b->animStartTime = 0.0;
        // Check for faints caused by the attack that just animated
        if (playerPet->currentHP <= 0) {
            playerPet->currentHP = 0;
            bool allDead = true;
            for (int i = 0; i < b->teamSize; i++)
                if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
            if (allDead) { b->needsRevive = true; b->waitingForInput = true; return; }
            b->waitingForInput = true;
            b->isPlayerTurn = true;
            b->selectedMove = -1;
            b->enemyActedFirst = false;
            return;
        }
        if (enemyPet->currentHP <= 0) {
            enemyPet->currentHP = 0;
            b->battleOver = true;
            b->playerWon = true;
            if (g_audio) PlaySFX(g_audio, SFX_WIN);
            return;
        }
        // If enemy attacked first, player must respond now
        if (b->enemyActedFirst) {
            b->waitingForInput = true;
            b->isPlayerTurn = true;
            b->selectedMove = -1;
            return;
        }
        // Turn fully complete — determine who goes first next turn
        int nextEnemySlot = EnemyPickMove(b);
        bool enemyCanAct = (enemyPet->status != STATUS_FREEZE);
        if (enemyCanAct && EffectiveSpeed(enemyPet) > EffectiveSpeed(playerPet)) {
            // Enemy goes first: auto-attack, then player responds
            b->enemyActedFirst = true;
            b->animStartTime = GetTime();
            if (nextEnemySlot >= 0) {
                int emId = enemyPet->moveIds[nextEnemySlot];
                if (emId < 0 || emId >= (int)MOVE_COUNT) emId = 0;
                b->animInfo.moveId = emId;
                b->animInfo.moveType = MOVE_DATABASE[emId].type;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = MOVE_DATABASE[emId].basePower < 55 ? 1.0f :
                                       MOVE_DATABASE[emId].basePower < 80 ? 1.2f : 1.5f;
                ExecuteAttack(enemyPet, playerPet, nextEnemySlot, false, b);
            } else {
                int sd = enemyPet->level; if (sd < 1) sd = 1;
                playerPet->currentHP -= sd;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = 0.6f;
                snprintf(b->message, sizeof(b->message), "对方的 %s PP耗尽，挣扎造成 %d 伤害！",
                         enemyPet->nickname, sd);
            }
            return;
        }
        // Player goes first next turn
        b->waitingForInput = true;
        b->isPlayerTurn = true;
        b->selectedMove = -1;
        return;
    }

    // --- Counter phase: second attacker's turn ---
    if (b->counterPending) {
        b->counterPending = false;
        b->animStartTime = GetTime();

        if (b->playerFirst && b->playerActs) {
            // Player went first → enemy counters
            if (b->enemySlot >= 0 && enemyPet->status != STATUS_FREEZE) {
                int emId = enemyPet->moveIds[b->enemySlot];
                if (emId < 0 || emId >= (int)MOVE_COUNT) emId = 0;
                b->animInfo.moveId = emId;
                b->animInfo.moveType = MOVE_DATABASE[emId].type;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = MOVE_DATABASE[emId].basePower < 55 ? 1.0f :
                                       MOVE_DATABASE[emId].basePower < 80 ? 1.2f : 1.5f;
                ExecuteAttack(enemyPet, playerPet, b->enemySlot, false, b);
            } else if (b->enemySlot < 0 && enemyPet->status != STATUS_FREEZE) {
                int sd = enemyPet->level; if (sd < 1) sd = 1;
                playerPet->currentHP -= sd;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = 0.6f;
                snprintf(b->message, sizeof(b->message), "对方的 %s PP耗尽，挣扎造成 %d 伤害！",
                         enemyPet->nickname, sd);
            } else {
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = 0.4f;
                snprintf(b->message, sizeof(b->message), "对方的 %s 被冻住了，无法行动！",
                         enemyPet->nickname);
            }
        } else if (!b->playerFirst && b->playerActs) {
            // Enemy went first → player counters
            if (playerPet->status != STATUS_FREEZE && CanUseMove(playerPet, b->selectedMove)) {
                int pmId = playerPet->moveIds[b->selectedMove];
                if (pmId < 0 || pmId >= (int)MOVE_COUNT) pmId = 0;
                b->animInfo.moveId = pmId;
                b->animInfo.moveType = MOVE_DATABASE[pmId].type;
                b->animInfo.isPlayerAttack = true;
                b->animInfo.duration = MOVE_DATABASE[pmId].basePower < 55 ? 1.0f :
                                       MOVE_DATABASE[pmId].basePower < 80 ? 1.2f : 1.5f;
                ExecuteAttack(playerPet, enemyPet, b->selectedMove, true, b);
            } else if (playerPet->status == STATUS_FREEZE) {
                b->animInfo.isPlayerAttack = true;
                b->animInfo.duration = 0.4f;
                snprintf(b->message, sizeof(b->message), "%s 被冻住了，无法行动！", playerPet->nickname);
            }
        } else {
            // Only one side acted this turn, no counter — mark done
            b->animStartTime = 0.0;
            if (playerPet->currentHP <= 0) {
                playerPet->currentHP = 0;
                bool allDead = true;
                for (int i = 0; i < b->teamSize; i++)
                    if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
                if (allDead) { b->needsRevive = true; b->waitingForInput = true; return; }
                b->waitingForInput = true;
                b->isPlayerTurn = true;
                b->selectedMove = -1;
                return;
            }
            if (enemyPet->currentHP <= 0) {
                enemyPet->currentHP = 0;
                b->battleOver = true;
                b->playerWon = true;
                if (g_audio) PlaySFX(g_audio, SFX_WIN);
                return;
            }
            b->waitingForInput = true;
            b->isPlayerTurn = true;
            b->selectedMove = -1;
            return;
        }
        return; // Let counter animation play
    }

    // ============================================================
    // === ENEMY ACTED FIRST — PLAYER RESPONDS ===================
    // ============================================================

    if (b->enemyActedFirst) {
        if (b->selectedMove < 0) {
            // Enemy needs to attack first (player hasn't chosen yet)
            int enemySlot = EnemyPickMove(b);
            b->animStartTime = GetTime();
            if (enemySlot >= 0 && enemyPet->status != STATUS_FREEZE) {
                int emId = enemyPet->moveIds[enemySlot];
                if (emId < 0 || emId >= (int)MOVE_COUNT) emId = 0;
                b->animInfo.moveId = emId;
                b->animInfo.moveType = MOVE_DATABASE[emId].type;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = MOVE_DATABASE[emId].basePower < 55 ? 1.0f :
                                       MOVE_DATABASE[emId].basePower < 80 ? 1.2f : 1.5f;
                ExecuteAttack(enemyPet, playerPet, enemySlot, false, b);
            } else if (enemySlot < 0 && enemyPet->status != STATUS_FREEZE) {
                int sd = enemyPet->level; if (sd < 1) sd = 1;
                playerPet->currentHP -= sd;
                b->animInfo.isPlayerAttack = false;
                b->animInfo.duration = 0.6f;
                snprintf(b->message, sizeof(b->message), "对方的 %s PP耗尽，挣扎造成 %d 伤害！",
                         enemyPet->nickname, sd);
            } else {
                // Enemy frozen — skip its turn, let player go first
                b->enemyActedFirst = false;
                b->animStartTime = 0.0;
                b->animInfo.duration = 0.01f;
                snprintf(b->message, sizeof(b->message), "对方的 %s 被冻住了，无法行动！",
                         enemyPet->nickname);
            }
            // Check if enemy attack killed player
            if (playerPet->currentHP <= 0) {
                playerPet->currentHP = 0;
                bool allDead = true;
                for (int i = 0; i < b->teamSize; i++)
                    if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
                if (allDead) { b->needsRevive = true; b->waitingForInput = true; b->enemyActedFirst = false; return; }
                b->waitingForInput = true;
                b->isPlayerTurn = true;
                b->selectedMove = -1;
                b->enemyActedFirst = false;
                return;
            }
            return;
        }
        // Player responds to enemy's attack
        b->enemyActedFirst = false;
        b->animStartTime = GetTime();
        if (b->selectedMove >= 0 && playerPet->status != STATUS_FREEZE
            && CanUseMove(playerPet, b->selectedMove)) {
            int pmId = playerPet->moveIds[b->selectedMove];
            if (pmId < 0 || pmId >= (int)MOVE_COUNT) pmId = 0;
            b->animInfo.moveId = pmId;
            b->animInfo.moveType = MOVE_DATABASE[pmId].type;
            b->animInfo.isPlayerAttack = true;
            b->animInfo.duration = MOVE_DATABASE[pmId].basePower < 55 ? 1.0f :
                                   MOVE_DATABASE[pmId].basePower < 80 ? 1.2f : 1.5f;
            ExecuteAttack(playerPet, enemyPet, b->selectedMove, true, b);
        } else if (b->selectedMove >= 0 && playerPet->status == STATUS_FREEZE) {
            b->animInfo.isPlayerAttack = true;
            b->animInfo.duration = 0.4f;
            snprintf(b->message, sizeof(b->message), "%s 被冻住了，无法行动！", playerPet->nickname);
        }
        if (enemyPet->currentHP <= 0) {
            enemyPet->currentHP = 0;
            b->battleOver = true;
            b->playerWon = true;
            if (g_audio) PlaySFX(g_audio, SFX_WIN);
            return;
        }
        return;
    }

    // ============================================================
    // === FIRST ATTACK PHASE (fresh turn, player goes first) =====
    // ============================================================

    // Safety: if active pet fainted, handle immediately
    if (IsFainted(playerPet)) {
        bool allDead = true;
        for (int i = 0; i < b->teamSize; i++)
            if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
        if (allDead) b->needsRevive = true;
        b->waitingForInput = true;
        b->isPlayerTurn = true;
        b->selectedMove = -1;
        return;
    }

    b->animStartTime = GetTime();
    b->playerActs = (b->selectedMove >= 0);

    // --- Start-of-turn status ticks ---
    if (b->message[0]) b->message[0] = '\0';
    ApplyStatusTick(playerPet, b->message, sizeof(b->message));
    ApplyStatusTick(enemyPet, b->message, sizeof(b->message));

    // Check if status ticks caused a faint
    if (playerPet->currentHP <= 0) {
        playerPet->currentHP = 0;
        bool allDead = true;
        for (int i = 0; i < b->teamSize; i++)
            if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
        if (allDead) { b->needsRevive = true; b->waitingForInput = true; return; }
        b->waitingForInput = true;
        b->isPlayerTurn = true;
        return;
    }
    if (enemyPet->currentHP <= 0) {
        enemyPet->currentHP = 0;
        b->battleOver = true;
        b->playerWon = true;
        if (g_audio) PlaySFX(g_audio, SFX_WIN);
        return;
    }

    // --- Determine turn order ---
    int playerPrio = 0, enemyPrio = 0;
    b->enemySlot = EnemyPickMove(b);
    if (b->playerActs && b->selectedMove >= 0 && b->selectedMove < 4) {
        int pmId = playerPet->moveIds[b->selectedMove];
        if (pmId >= 0 && pmId < (int)MOVE_COUNT)
            playerPrio = (MOVE_DATABASE[pmId].flags & MOVE_FLAG_PRIORITY) ? 1 : 0;
    }
    if (b->enemySlot >= 0) {
        int emId = enemyPet->moveIds[b->enemySlot];
        if (emId >= 0 && emId < (int)MOVE_COUNT)
            enemyPrio = (MOVE_DATABASE[emId].flags & MOVE_FLAG_PRIORITY) ? 1 : 0;
    }

    if (playerPrio > enemyPrio) b->playerFirst = true;
    else if (enemyPrio > playerPrio) b->playerFirst = false;
    else b->playerFirst = (EffectiveSpeed(playerPet) >= EffectiveSpeed(enemyPet));

    // --- Execute first attacker ---
    if (b->playerFirst && b->playerActs) {
        // Player attacks first
        if (playerPet->status != STATUS_FREEZE) {
            int pmId = playerPet->moveIds[b->selectedMove];
            if (pmId < 0 || pmId >= (int)MOVE_COUNT) pmId = 0;
            b->animInfo.moveId = pmId;
            b->animInfo.moveType = MOVE_DATABASE[pmId].type;
            b->animInfo.isPlayerAttack = true;
            b->animInfo.duration = MOVE_DATABASE[pmId].basePower < 55 ? 1.0f :
                                   MOVE_DATABASE[pmId].basePower < 80 ? 1.2f : 1.5f;
            ExecuteAttack(playerPet, enemyPet, b->selectedMove, true, b);
        } else {
            b->animInfo.isPlayerAttack = true;
            b->animInfo.duration = 0.4f;
            snprintf(b->message, sizeof(b->message), "%s 被冻住了，无法行动！", playerPet->nickname);
        }

        // Check if player's attack killed enemy
        if (enemyPet->currentHP <= 0) {
            enemyPet->currentHP = 0;
            b->battleOver = true;
            b->playerWon = true;
            if (g_audio) PlaySFX(g_audio, SFX_WIN);
            return;
        }

        // Queue enemy counter (unless enemy frozen or no PP)
        if (enemyPet->status != STATUS_FREEZE) {
            b->counterPending = true;
            b->counterDelay = 0.35f;
        }
    } else {
        // Enemy attacks first
        if (b->enemySlot >= 0 && enemyPet->status != STATUS_FREEZE) {
            int emId = enemyPet->moveIds[b->enemySlot];
            if (emId < 0 || emId >= (int)MOVE_COUNT) emId = 0;
            b->animInfo.moveId = emId;
            b->animInfo.moveType = MOVE_DATABASE[emId].type;
            b->animInfo.isPlayerAttack = false;
            b->animInfo.duration = MOVE_DATABASE[emId].basePower < 55 ? 1.0f :
                                   MOVE_DATABASE[emId].basePower < 80 ? 1.2f : 1.5f;
            ExecuteAttack(enemyPet, playerPet, b->enemySlot, false, b);
        } else if (b->enemySlot < 0 && enemyPet->status != STATUS_FREEZE) {
            int sd = enemyPet->level; if (sd < 1) sd = 1;
            playerPet->currentHP -= sd;
            b->animInfo.isPlayerAttack = false;
            b->animInfo.duration = 0.6f;
            snprintf(b->message, sizeof(b->message), "对方的 %s PP耗尽，挣扎造成 %d 伤害！",
                     enemyPet->nickname, sd);
        } else {
            b->animInfo.isPlayerAttack = false;
            b->animInfo.duration = 0.4f;
            snprintf(b->message, sizeof(b->message), "对方的 %s 被冻住了，无法行动！",
                     enemyPet->nickname);
        }

        // Check if enemy's attack killed player pet
        if (playerPet->currentHP <= 0) {
            playerPet->currentHP = 0;
            bool allDead = true;
            for (int i = 0; i < b->teamSize; i++)
                if (!IsFainted(&b->playerTeam[i])) { allDead = false; break; }
            if (allDead) { b->needsRevive = true; b->waitingForInput = true; return; }
            b->waitingForInput = true;
            b->isPlayerTurn = true;
            b->selectedMove = -1;
            return;
        }

        // Queue player counter (if player selected a move and isn't frozen)
        if (b->playerActs && playerPet->status != STATUS_FREEZE
            && CanUseMove(playerPet, b->selectedMove)) {
            b->counterPending = true;
            b->counterDelay = 0.35f;
        }
    }
}

// ============ Queries ============

bool BattleIsPlayerTurn(const BattleState *b) {
    return b->waitingForInput && !b->battleOver;
}

bool BattleIsOver(const BattleState *b) {
    return b->battleOver;
}

void BattleSwitchPet(BattleState *b, int newIdx) {
    if (!b->waitingForInput || b->battleOver) return;
    if (newIdx < 0 || newIdx >= b->teamSize) return;
    if (newIdx == b->activePetIdx) return;
    if (IsFainted(&b->playerTeam[newIdx])) return;

    Pet *oldPet = &b->playerTeam[b->activePetIdx];
    b->activePetIdx = newIdx;
    Pet *newPet = &b->playerTeam[b->activePetIdx];

    snprintf(b->message, sizeof(b->message), "回来吧 %s！上吧 %s！",
             oldPet->nickname, newPet->nickname);
    b->selectedMove = -1;
    b->isPlayerTurn = true;
    b->waitingForInput = true;
}
