#pragma once
#include <stddef.h>   /* size_t */
#include "model.h"
/* ===================== STORAGE: doc/ghi tap tin =====================
 * - Luu / tai van co dang choi (phim L / T)
 * - Lich su cac van da dau (caro_history.txt)
 * - Cai dat am thanh + ten nguoi choi (caro_settings.cfg)
 * Chi dung stdio nen khong phu thuoc Windows (tru ham liet ke file).
 */

#define SAVE_EXT ".caro"
#define FILE_BASE_MAX 40            /* toi da ky tu nguoi dung go cho ten file */
#define FILE_NAME_LEN 48
#define HISTORY_FILE "caro_history.txt"
#define SETTINGS_FILE "caro_settings.cfg"

#define SAVE_OK 0
#define SAVE_ERR_IO 1               /* khong mo/ghi/doc duoc file */
#define SAVE_ERR_FORMAT 2           /* file sai dinh dang hoac sai luat */

typedef char FileName[FILE_NAME_LEN];

/* ---- ten file ---- */
void sanitizeName(char *dst, const char *src);     /* ten nguoi choi hop le, khong rong */
/* Dat ten nguoi choi idx (0 = X, 1 = O) cho van dau hien tai (chi doi session, khong doi Settings). */
#define NAME_OK 0                   /* da doi ten */
#define NAME_KEPT 1                 /* bo trong -> giu ten cu */
#define NAME_DUPLICATE 2            /* trung ten nguoi con lai -> khong doi */
int  setPlayerName(int idx, const char *typed);
/* Dam bao ten ket thuc bang .caro (them neu thieu). Tra ve 0 neu ten khong hop le
 * (rong, bat dau bang '.', ten thiet bi Windows nhu CON/NUL/COM1, qua dai). */
int  normalizeFileName(char *name, size_t cap);
int  fileExists(const char *name);

/* ---- luu / tai van ---- */
int  saveGame(const char *file);
int  loadGame(const char *file);
int  listSaves(FileName *names, int max);          /* liet ke *.caro trong thu muc hien tai */

/* ---- lich su ---- */
#define MAX_HISTORY 1000
typedef struct {
    char when[17];                  /* "YYYY-MM-DD HH:MM" */
    int result;                     /* RES_X_WIN / RES_O_WIN / RES_DRAW */
    char name[2][NAME_LEN];
    int moves;
    int seconds;
} HistoryEntry;

extern HistoryEntry history[MAX_HISTORY];
extern int historyCount;

int  historyAppend(int result);     /* ghi van vua ket thuc (dung session + totalMoves + dong ho) */
int  historyLoad(void);             /* doc file vao history[]; tra ve so van */
int  historyClear(void);

/* ---- cai dat ---- */
void settingsLoad(void);
int  settingsSave(void);
/* Doi ngon ngu (LANG_VI / LANG_EN) ngay lap tuc. Ten nguoi choi con la ten mac dinh
 * ("Người chơi 1") thi doi theo ngon ngu moi ("Player 1"); ten tu dat giu nguyen. */
void settingsSetLanguage(int lang);
