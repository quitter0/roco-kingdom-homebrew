#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    char     chunkID[4];     // "RIFF"
    uint32_t chunkSize;
    char     format[4];      // "WAVE"
    char     subchunk1ID[4]; // "fmt "
    uint32_t subchunk1Size;  // 16 for PCM
    uint16_t audioFormat;    // 1 = PCM
    uint16_t numChannels;    // 1 = mono
    uint32_t sampleRate;     // 44100
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;  // 16
    char     subchunk2ID[4]; // "data"
    uint32_t subchunk2Size;
} WavHeader;

static void WriteWav(FILE *f, int16_t *samples, int count, int sampleRate) {
    WavHeader h;
    memcpy(h.chunkID, "RIFF", 4);
    memcpy(h.format, "WAVE", 4);
    memcpy(h.subchunk1ID, "fmt ", 4);
    memcpy(h.subchunk2ID, "data", 4);

    h.subchunk1Size = 16;
    h.audioFormat = 1;
    h.numChannels = 1;
    h.sampleRate = sampleRate;
    h.bitsPerSample = 16;
    h.byteRate = sampleRate * 1 * 16 / 8;
    h.blockAlign = 1 * 16 / 8;
    h.subchunk2Size = count * sizeof(int16_t);
    h.chunkSize = 36 + h.subchunk2Size;

    fwrite(&h, sizeof(h), 1, f);
    fwrite(samples, sizeof(int16_t), count, f);
}

// Short click/beep sound (select, run)
static void GenBeep(const char *path, float freq, float duration, float volume) {
    int sampleRate = 22050;
    int count = (int)(sampleRate * duration);
    int16_t *samples = malloc(count * sizeof(int16_t));
    for (int i = 0; i < count; i++) {
        float t = (float)i / sampleRate;
        float envelope = 1.0f - (float)i / count;
        samples[i] = (int16_t)(sin(2.0f * M_PI * freq * t) * envelope * volume * 32767);
    }
    FILE *f = fopen(path, "wb");
    if (f) { WriteWav(f, samples, count, sampleRate); fclose(f); printf("Created %s\n", path); }
    free(samples);
}

// Noise burst (damage)
static void GenNoise(const char *path, float duration, float volume) {
    int sampleRate = 22050;
    int count = (int)(sampleRate * duration);
    int16_t *samples = malloc(count * sizeof(int16_t));
    for (int i = 0; i < count; i++) {
        float envelope = 1.0f - (float)i / count;
        samples[i] = (int16_t)(((rand() % 20000 - 10000) / 10000.0f) * envelope * volume * 32767);
    }
    FILE *f = fopen(path, "wb");
    if (f) { WriteWav(f, samples, count, sampleRate); fclose(f); printf("Created %s\n", path); }
    free(samples);
}

// Rising tone (heal, catch, win)
static void GenRising(const char *path, float startFreq, float endFreq, float duration, float volume) {
    int sampleRate = 22050;
    int count = (int)(sampleRate * duration);
    int16_t *samples = malloc(count * sizeof(int16_t));
    for (int i = 0; i < count; i++) {
        float t = (float)i / sampleRate;
        float freq = startFreq + (endFreq - startFreq) * t / duration;
        float envelope = 1.0f - (float)i / count;
        // FM-like richer tone
        float val = sin(2.0f * M_PI * freq * t) + 0.3f * sin(2.0f * M_PI * freq * 1.5f * t);
        samples[i] = (int16_t)(val * envelope * volume * 32767 * 0.7f);
    }
    FILE *f = fopen(path, "wb");
    if (f) { WriteWav(f, samples, count, sampleRate); fclose(f); printf("Created %s\n", path); }
    free(samples);
}

// Simple looping menu BG (short chord-based loop)
static void GenMenuBGM(const char *path) {
    int sampleRate = 22050;
    int duration = 8;  // 8-second loop
    int count = sampleRate * duration;
    int16_t *samples = malloc(count * sizeof(int16_t));

    // Simple melody notes (frequencies of a gentle chord progression)
    float notes[] = {261.63f, 329.63f, 392.00f, 349.23f, 293.66f, 329.63f, 261.63f, 246.94f};
    int noteLen = count / 8;

    for (int i = 0; i < count; i++) {
        int noteIdx = i / noteLen;
        if (noteIdx >= 8) noteIdx = 7;
        float t = (float)(i % noteLen) / sampleRate;
        float envelope = 0.3f;
        float bass = sin(2.0f * M_PI * notes[noteIdx] * 0.5f * t) * 0.5f;
        float melody = sin(2.0f * M_PI * notes[noteIdx] * t) * 0.4f;
        float harmony = sin(2.0f * M_PI * notes[noteIdx] * 1.5f * t) * 0.2f;
        float val = (bass + melody + harmony) * envelope;
        // Gentle fade in/out at loop boundaries
        float fadeIn = (float)(i % noteLen) / (noteLen * 0.1f);
        if (fadeIn > 1.0f) fadeIn = 1.0f;
        float fadeOut = (float)(noteLen - (i % noteLen)) / (noteLen * 0.1f);
        if (fadeOut > 1.0f) fadeOut = 1.0f;
        float fade = fadeIn < fadeOut ? fadeIn : fadeOut;
        if (fade > 1.0f) fade = 1.0f;
        samples[i] = (int16_t)(val * fade * 16000);
    }

    FILE *f = fopen(path, "wb");
    if (f) { WriteWav(f, samples, count, sampleRate); fclose(f); printf("Created %s\n", path); }
    free(samples);
}

int main(void) {
    printf("Generating audio files...\n");
    GenBeep("resources/audio/select.wav", 800, 0.15f, 0.6f);
    GenNoise("resources/audio/damage.wav", 0.3f, 0.7f);
    GenRising("resources/audio/heal.wav", 300, 600, 0.5f, 0.6f);
    GenRising("resources/audio/catch.wav", 400, 1200, 0.8f, 0.7f);
    GenRising("resources/audio/win.wav", 500, 1000, 1.0f, 0.7f);
    GenBeep("resources/audio/run.wav", 400, 0.2f, 0.5f);
    GenMenuBGM("resources/audio/menu.wav");
    printf("Done.\n");
    return 0;
}
