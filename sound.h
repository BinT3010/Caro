#pragma once
/* ===================== SOUND: am thanh co chinh duoc =====================
 * Cac am thanh duoc TU TONG HOP (song sin) ra bo nho dang WAV roi phat bang
 * PlaySound cua Windows, nen khong can file am thanh di kem.
 * Am luong (0..100) chinh bien do song => nghe to/nho that su.
 * Khi bien dich can lien ket thu vien winmm (-lwinmm).
 */

typedef enum {
    SND_MENU,       /* chon menu / chinh cai dat */
    SND_PLACE_X,    /* danh quan X */
    SND_PLACE_O,    /* danh quan O */
    SND_ERROR,      /* danh vao o da co, file loi... */
    SND_WIN,        /* co nguoi thang */
    SND_DRAW,       /* hoa */
    SND_SAVE,       /* luu van */
    SND_LOAD,       /* tai van */
    SND_START,      /* bat dau van moi */
    SND_COUNT
} SoundId;

void soundConfigure(int enabled, int volume);   /* ap dung cai dat */
void soundPlay(SoundId id);                     /* phat bat dong bo, bo qua neu dang tat */
void soundStop(void);

/* Tong hop WAV vao bo dem (khong phat). Tra ve so byte, 0 neu loi.
 * Dung de phat va de kiem thu. */
#define SOUND_WAV_MAX (44 + 2 * 22050 * 2)
int soundRender(SoundId id, int volume, unsigned char *wav, int cap);
