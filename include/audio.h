#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    BGM_NONE = 0,
    BGM_MENU,
    BGM_WORLD,
    BGM_BATTLE
} BGMTrack;

typedef enum {
    SFX_SELECT = 0,
    SFX_DAMAGE,
    SFX_HEAL,
    SFX_CATCH,
    SFX_WIN,
    SFX_RUN,
    SFX_COUNT
} SFXType;

typedef struct {
    Music bgmMenu;
    Music bgmWorld;
    Music bgmBattle;
    bool bgmLoaded[3];
    Sound sfx[SFX_COUNT];
    bool sfxLoaded[SFX_COUNT];
    bool audioEnabled;
    BGMTrack currentBGM;
} AudioManager;

extern AudioManager *g_audio;  // global pointer for SFX from any module

void InitAudioManager(AudioManager *am);
void PlayBGM(AudioManager *am, BGMTrack track);
void PauseBGM(AudioManager *am);
void ResumeBGM(AudioManager *am, BGMTrack track);
void StopBGM(AudioManager *am);
void PlaySFX(AudioManager *am, SFXType sfx);
void UpdateAudioManager(AudioManager *am);
void UnloadAudioManager(AudioManager *am);

#endif
