#include "audio.h"
#include <string.h>

AudioManager *g_audio = NULL;

static bool TryLoadMusic(const char *path, Music *music) {
    if (FileExists(path)) {
        *music = LoadMusicStream(path);
        return music->stream.buffer != NULL;
    }
    return false;
}

static bool TryLoadSoundFile(const char *path, Sound *sound) {
    if (FileExists(path)) {
        *sound = LoadSound(path);
        return sound->stream.buffer != NULL;
    }
    return false;
}

void InitAudioManager(AudioManager *am) {
    g_audio = am;
    memset(am, 0, sizeof(AudioManager));
    InitAudioDevice();
    am->audioEnabled = true;

    am->bgmLoaded[0] = TryLoadMusic("resources/audio/menu.wav", &am->bgmMenu);
    am->bgmLoaded[1] = TryLoadMusic("resources/bgms/world.mp3", &am->bgmWorld);
    am->bgmLoaded[2] = TryLoadMusic("resources/bgms/battle.mp3", &am->bgmBattle);

    TryLoadSoundFile("resources/audio/select.wav", &am->sfx[SFX_SELECT]);
    am->sfxLoaded[SFX_SELECT] = am->sfx[SFX_SELECT].stream.buffer != NULL;

    TryLoadSoundFile("resources/audio/damage.wav", &am->sfx[SFX_DAMAGE]);
    am->sfxLoaded[SFX_DAMAGE] = am->sfx[SFX_DAMAGE].stream.buffer != NULL;

    TryLoadSoundFile("resources/audio/heal.wav", &am->sfx[SFX_HEAL]);
    am->sfxLoaded[SFX_HEAL] = am->sfx[SFX_HEAL].stream.buffer != NULL;

    TryLoadSoundFile("resources/audio/catch.wav", &am->sfx[SFX_CATCH]);
    am->sfxLoaded[SFX_CATCH] = am->sfx[SFX_CATCH].stream.buffer != NULL;

    TryLoadSoundFile("resources/audio/win.wav", &am->sfx[SFX_WIN]);
    am->sfxLoaded[SFX_WIN] = am->sfx[SFX_WIN].stream.buffer != NULL;

    TryLoadSoundFile("resources/audio/run.wav", &am->sfx[SFX_RUN]);
    am->sfxLoaded[SFX_RUN] = am->sfx[SFX_RUN].stream.buffer != NULL;

    am->currentBGM = BGM_NONE;
}

static Music *GetMusicForTrack(AudioManager *am, BGMTrack track) {
    switch (track) {
        case BGM_MENU:   return &am->bgmMenu;
        case BGM_WORLD:  return &am->bgmWorld;
        case BGM_BATTLE: return &am->bgmBattle;
        default: return NULL;
    }
}

static int GetBgmIndex(BGMTrack track) { return track - 1; }

void PlayBGM(AudioManager *am, BGMTrack track) {
    if (!am->audioEnabled) return;

    // WORLD ↔ BATTLE: pause world instead of stopping, so it resumes from same spot
    if (track == BGM_BATTLE && am->currentBGM == BGM_WORLD) {
        PauseBGM(am);
    }

    int idx = GetBgmIndex(track);
    if (idx >= 0 && idx < 3 && am->bgmLoaded[idx]) {
        Music *m = GetMusicForTrack(am, track);
        if (!m) return;
        PlayMusicStream(*m);
        am->currentBGM = track;
    }
}

void PauseBGM(AudioManager *am) {
    if (!am->audioEnabled) return;
    int idx = GetBgmIndex(am->currentBGM);
    if (idx >= 0 && idx < 3 && am->bgmLoaded[idx]) {
        Music *m = GetMusicForTrack(am, am->currentBGM);
        if (m) PauseMusicStream(*m);
    }
}

void ResumeBGM(AudioManager *am, BGMTrack track) {
    if (!am->audioEnabled) return;
    // Stop a different track (e.g., battle) but keep the current one playing
    // from its existing position; StopMusicStream rewinds to the start.
    if (am->currentBGM != track) StopBGM(am);
    int idx = GetBgmIndex(track);
    if (idx >= 0 && idx < 3 && am->bgmLoaded[idx]) {
        Music *m = GetMusicForTrack(am, track);
        if (m) ResumeMusicStream(*m);
        am->currentBGM = track;
    }
}

void StopBGM(AudioManager *am) {
    if (!am->audioEnabled) return;
    int idx = GetBgmIndex(am->currentBGM);
    if (idx >= 0 && idx < 3 && am->bgmLoaded[idx]) {
        Music *m = GetMusicForTrack(am, am->currentBGM);
        if (m) StopMusicStream(*m);
    }
    am->currentBGM = BGM_NONE;
}

void PlaySFX(AudioManager *am, SFXType sfx) {
    if (!am->audioEnabled) return;
    if (sfx < 0 || sfx >= SFX_COUNT) return;
    if (am->sfxLoaded[sfx]) {
        PlaySound(am->sfx[sfx]);
    }
}

void UpdateAudioManager(AudioManager *am) {
    if (!am->audioEnabled) return;
    int idx = am->currentBGM - 1;
    if (idx >= 0 && idx < 3 && am->bgmLoaded[idx]) {
        Music *m = NULL;
        switch (am->currentBGM) {
            case BGM_MENU:   m = &am->bgmMenu;   break;
            case BGM_WORLD:  m = &am->bgmWorld;  break;
            case BGM_BATTLE: m = &am->bgmBattle; break;
            default: break;
        }
        if (m) UpdateMusicStream(*m);
    }
}

void UnloadAudioManager(AudioManager *am) {
    for (int i = 0; i < 3; i++) {
        if (am->bgmLoaded[i]) {
            Music *m = (i == 0) ? &am->bgmMenu : (i == 1) ? &am->bgmWorld : &am->bgmBattle;
            UnloadMusicStream(*m);
        }
    }
    for (int i = 0; i < SFX_COUNT; i++) {
        if (am->sfxLoaded[i]) {
            UnloadSound(am->sfx[i]);
        }
    }
    CloseAudioDevice();
}
