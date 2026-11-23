#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <math.h>
#include <string.h>
#include "sound.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#endif

#define SAMPLE_RATE 22050
#define MAX_SAMPLES (SAMPLE_RATE * 2)           /* toi da 2 giay */
#define WAV_HEADER 44

typedef struct { int freq; int ms; } Note;      /* freq = 0: nghi; ms = 0: het day */

static const Note S_MENU[]   = { {880, 30}, {0, 0} };
static const Note S_PLACE_X[] = { {784, 70}, {0, 0} };
static const Note S_PLACE_O[] = { {587, 70}, {0, 0} };
static const Note S_ERROR[]  = { {196, 90}, {0, 30}, {196, 130}, {0, 0} };
static const Note S_WIN[]    = { {523, 120}, {659, 120}, {784, 120}, {1047, 320},
                                 {0, 60}, {784, 100}, {1047, 380}, {0, 0} };
static const Note S_DRAW[]   = { {440, 180}, {392, 180}, {349, 320}, {0, 0} };
static const Note S_SAVE[]   = { {659, 80}, {988, 150}, {0, 0} };
static const Note S_LOAD[]   = { {988, 80}, {659, 150}, {0, 0} };
static const Note S_START[]  = { {523, 90}, {784, 150}, {0, 0} };

static const Note *const SEQ[SND_COUNT] = {
    S_MENU, S_PLACE_X, S_PLACE_O, S_ERROR, S_WIN, S_DRAW, S_SAVE, S_LOAD, S_START
};

static int soundEnabled = 1;
static int soundVolume = 60;

static void put16(unsigned char *p, int v) { p[0] = (unsigned char)(v & 255); p[1] = (unsigned char)((v >> 8) & 255); }
static void put32(unsigned char *p, long v) {
    p[0] = (unsigned char)(v & 255);
    p[1] = (unsigned char)((v >> 8) & 255);
    p[2] = (unsigned char)((v >> 16) & 255);
    p[3] = (unsigned char)((v >> 24) & 255);
}

int soundRender(SoundId id, int volume, unsigned char *wav, int cap) {
    if (id < 0 || id >= SND_COUNT || cap < SOUND_WAV_MAX) return 0;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    /* Do to cam nhan theo luy thua, nen dung binh phuong de thanh truot "deu" hon */
    double v = volume / 100.0;
    double amp = 32767.0 * 0.8 * v * v;
    const double PI = 3.14159265358979323846;

    unsigned char *data = wav + WAV_HEADER;
    int n = 0;
    for (const Note *nt = SEQ[id]; nt->ms > 0; nt++) {
        int len = SAMPLE_RATE * nt->ms / 1000;
        if (n + len > MAX_SAMPLES) break;
        int attack = SAMPLE_RATE / 200;                         /* 5 ms: tranh tieng "tch" */
        int release = len / 2 < SAMPLE_RATE / 50 ? len / 2 : SAMPLE_RATE / 50;
        for (int i = 0; i < len; i++) {
            double s = 0.0;
            if (nt->freq > 0) {
                double env = 1.0;
                if (i < attack) env = (double)i / attack;
                if (i > len - release && release > 0) env *= (double)(len - i) / release;
                double ph = 2.0 * PI * nt->freq * i / SAMPLE_RATE;
                s = amp * env * (sin(ph) + 0.3 * sin(2.0 * ph)) / 1.3;
            }
            put16(data + 2 * (n + i), (int)(short)s);
        }
        n += len;
    }
    if (n == 0) return 0;

    long bytes = 2L * n;
    memcpy(wav, "RIFF", 4);          put32(wav + 4, 36 + bytes);
    memcpy(wav + 8, "WAVEfmt ", 8);  put32(wav + 16, 16);
    put16(wav + 20, 1);              /* PCM */
    put16(wav + 22, 1);              /* mono */
    put32(wav + 24, SAMPLE_RATE);
    put32(wav + 28, SAMPLE_RATE * 2);
    put16(wav + 32, 2);
    put16(wav + 34, 16);
    memcpy(wav + 36, "data", 4);    put32(wav + 40, bytes);
    return WAV_HEADER + (int)bytes;
}

void soundConfigure(int enabled, int volume) {
    soundEnabled = enabled != 0;
    soundVolume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
    if (!soundEnabled || soundVolume == 0) soundStop();
}

void soundStop(void) {
#ifdef _WIN32
    PlaySoundA(NULL, NULL, 0);
#endif
}

void soundPlay(SoundId id) {
    if (!soundEnabled || soundVolume <= 0) return;
#ifdef _WIN32
    static unsigned char wavBuf[SOUND_WAV_MAX];  /* phai ton tai cho den khi phat xong */
    PlaySoundA(NULL, NULL, 0);                   /* dung am cu truoc khi ghi de bo dem */
    if (soundRender(id, soundVolume, wavBuf, (int)sizeof wavBuf) > 0)
        PlaySoundA((LPCSTR)wavBuf, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)id;
#endif
}
