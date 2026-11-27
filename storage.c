#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "storage.h"
#include "ai.h"
#include "utf8.h"
#include "lang.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

HistoryEntry history[MAX_HISTORY];
int historyCount;

/* ---------------- tien ich ---------------- */

/* Doc 1 dong, bo ky tu xuong dong. Tra ve 0 neu het file. */
static int readLine(FILE *f, char *buf, int cap) {
    if (!fgets(buf, cap, f)) return 0;
    size_t n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
    return 1;
}

/* Ten hop le (UTF-8, co the co dau): giu ky tu in duoc; bo ky tu dieu khien, '|' (dau phan
 * cach trong file lich su), dau thanh roi (to hop), byte sai UTF-8; cat khoang trang hai dau;
 * toi da NAME_CHARS ky tu. Ten rong -> "Người chơi". */
void sanitizeName(char *dst, const char *src) {
    char tmp[NAME_LEN];
    size_t n = 0;
    int chars = 0;
    unsigned cp;
    for (int k; chars < NAME_CHARS && (k = utf8Decode(src, &cp)) > 0; src += k) {
        if (cp < 32 || cp == 127 || cp == '|' || cp == UTF8_BAD || (cp >= 0x80 && cp < 0xA0)
            || (cp >= 0x300 && cp <= 0x36F) || cp > 0xFFFF)
            continue;
        if (chars == 0 && cp == ' ') continue;          /* cat khoang trang dau */
        n += (size_t)utf8Encode(cp, tmp + n);
        chars++;
    }
    while (n > 0 && tmp[n - 1] == ' ') n--;             /* cat khoang trang cuoi */
    tmp[n] = '\0';
    if (n == 0) snprintf(dst, NAME_LEN, "%s", tr(S_DEFAULT_PLAYER));
    else snprintf(dst, NAME_LEN, "%s", tmp);
}

/* Dat ten cho nguoi choi idx (0 = X, 1 = O) trong van dau hien tai.
 * - Bo trong / chi co khoang trang  -> giu ten cu (NAME_KEPT)
 * - Trung ten nguoi con lai (khong phan biet hoa thuong) -> tu choi, giu ten cu (NAME_DUPLICATE)
 * - Con lai: lam sach bang sanitizeName roi luu vao session (NAME_OK) */
int setPlayerName(int idx, const char *typed) {
    char clean[NAME_LEN];
    const char *p = typed ? typed : "";
    while (*p == ' ') p++;
    if (*p == '\0') return NAME_KEPT;
    sanitizeName(clean, p);
    if (utf8EqualNoCase(clean, session.name[1 - idx])) return NAME_DUPLICATE;   /* "ÂN" == "ân" */
    snprintf(session.name[idx], NAME_LEN, "%s", clean);
    return NAME_OK;
}

/* So sanh khong phan biet hoa/thuong (Windows khong phan biet ten file). */
static int sameNoCase(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int ca = tolower((unsigned char)a[i]), cb = tolower((unsigned char)b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
    return 1;
}

static int hasSaveExt(const char *name) {
    size_t len = strlen(name), ext = strlen(SAVE_EXT);
    return len > ext && sameNoCase(name + len - ext, SAVE_EXT, ext + 1);
}

/* Ten thiet bi cua Windows (CON, NUL, COM1...) khong duoc dung lam ten file. */
static int reservedBase(const char *name) {
    static const char *const R[] = { "CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$" };
    char base[FILE_NAME_LEN];
    size_t n = 0;
    while (name[n] && name[n] != '.' && n < sizeof base - 1) { base[n] = name[n]; n++; }
    base[n] = '\0';
    for (size_t k = 0; k < sizeof R / sizeof R[0]; k++)
        if (strlen(R[k]) == n && sameNoCase(base, R[k], n)) return 1;
    if (n == 4 && (sameNoCase(base, "COM", 3) || sameNoCase(base, "LPT", 3)) && isdigit((unsigned char)base[3]))
        return 1;
    return 0;
}

int normalizeFileName(char *name, size_t cap) {
    /* Bo khoang trang / dau cham o cuoi (Windows tu cat di, de gay nham ten) */
    size_t len = strlen(name);
    while (len > 0 && (name[len - 1] == '.' || name[len - 1] == ' ')) name[--len] = '\0';
    /* Luon ket thuc bang .caro de khong the ghi de file khac (vd: caro_settings.cfg) */
    if (!hasSaveExt(name)) {
        if (len + strlen(SAVE_EXT) + 1 > cap) return 0;
        strcat(name, SAVE_EXT);
    }
    if (strlen(name) <= strlen(SAVE_EXT)) return 0;  /* chi co ".caro" */
    if (name[0] == '.') return 0;                     /* file an / ten rong */
    if (reservedBase(name)) return 0;
    return 1;
}

int fileExists(const char *name) {
    FILE *f = fopen(name, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int cmpNames(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

int listSaves(FileName *names, int max) {
    int n = 0;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("*" SAVE_EXT, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        /* kiem tra lai duoi file: mau "*.caro" cua Windows co the khop ca ten 8.3 */
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && hasSaveExt(fd.cFileName) &&
            strlen(fd.cFileName) < FILE_NAME_LEN && n < max) {
            snprintf(names[n++], FILE_NAME_LEN, "%s", fd.cFileName);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    size_t ext = strlen(SAVE_EXT);
    DIR *d = opendir(".");
    if (!d) return 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t len = strlen(e->d_name);
        if (len > ext && len < FILE_NAME_LEN && hasSaveExt(e->d_name) && n < max)
            snprintf(names[n++], FILE_NAME_LEN, "%s", e->d_name);
    }
    closedir(d);
#endif
    qsort(names, (size_t)n, FILE_NAME_LEN, cmpNames);
    return n;
}

/* ---------------- luu / tai van ----------------
 * Dinh dang text de de kiem tra bang Notepad:
 *   CARO-SAVE 1
 *   size 12
 *   p1 <ten X>
 *   p2 <ten O>
 *   wins <X> <O>
 *   draws <n>
 *   elapsed <giay>
 *   cursor <dong> <cot>
 *   mode <0..3>         (0 = 2 nguoi, 1 = may De, 2 = may Trung binh, 3 = may Kho;
 *                        file cu khong co dong nay -> coi la 2 nguoi)
 *   clock <gioi han giay> <ms con lai cua X> <ms con lai cua O>
 *                        (300/600/900; file cu khong co -> 10 phut, ca 2 ben day gio)
 *   board
 *   <12 dong, moi dong 12 so: 0 trong, 1 = X, -1 = O>
 * Luot di va so nuoc di duoc suy ra tu ban co khi tai (nen khong the sai lech).
 */
int saveGame(const char *file) {
    FILE *f = fopen(file, "w");
    if (!f) return SAVE_ERR_IO;
    fprintf(f, "CARO-SAVE 1\n");
    fprintf(f, "size %d\n", BOARD_SIZE);
    fprintf(f, "p1 %s\n", session.name[0]);
    fprintf(f, "p2 %s\n", session.name[1]);
    fprintf(f, "wins %d %d\n", session.wins[0], session.wins[1]);
    fprintf(f, "draws %d\n", session.draws);
    fprintf(f, "elapsed %d\n", clockElapsed());
    fprintf(f, "cursor %d %d\n", currX, currY);
    fprintf(f, "mode %d\n", aiLevel);
    /* luc luu dong ho dang dung (control da tam dung) nen thoi diem truyen vao khong anh huong */
    fprintf(f, "clock %d %ld %ld\n", timeLimitSec, turnClockLeftMs(0, 0), turnClockLeftMs(1, 0));
    fprintf(f, "board\n");
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            fprintf(f, "%d%c", chessBoard[i][j].mark, j == BOARD_SIZE - 1 ? '\n' : ' ');
    if (ferror(f)) { fclose(f); return SAVE_ERR_IO; }
    return fclose(f) == 0 ? SAVE_OK : SAVE_ERR_IO;
}

int loadGame(const char *file) {
    FILE *f = fopen(file, "r");
    if (!f) return SAVE_ERR_IO;

    char line[256];
    int size = 0, elapsed = 0, cr = 0, cc = 0, mode = AI_NONE;
    int limit = TIME_DEFAULT;
    long leftX = TIME_DEFAULT * 1000L, leftO = TIME_DEFAULT * 1000L;
    int marks[BOARD_SIZE][BOARD_SIZE];
    Session s;
    memset(&s, 0, sizeof s);
    int ok = 1;

    ok = ok && readLine(f, line, sizeof line) && strcmp(line, "CARO-SAVE 1") == 0;
    ok = ok && readLine(f, line, sizeof line) && sscanf(line, "size %d", &size) == 1 && size == BOARD_SIZE;
    ok = ok && readLine(f, line, sizeof line) && strncmp(line, "p1 ", 3) == 0;
    if (ok) sanitizeName(s.name[0], line + 3);
    ok = ok && readLine(f, line, sizeof line) && strncmp(line, "p2 ", 3) == 0;
    if (ok) sanitizeName(s.name[1], line + 3);
    ok = ok && readLine(f, line, sizeof line) && sscanf(line, "wins %d %d", &s.wins[0], &s.wins[1]) == 2;
    ok = ok && readLine(f, line, sizeof line) && sscanf(line, "draws %d", &s.draws) == 1;
    ok = ok && readLine(f, line, sizeof line) && sscanf(line, "elapsed %d", &elapsed) == 1;
    ok = ok && readLine(f, line, sizeof line) && sscanf(line, "cursor %d %d", &cr, &cc) == 2;
    ok = ok && readLine(f, line, sizeof line);
    if (ok && strncmp(line, "mode ", 5) == 0) {                /* dong "mode" la tuy chon */
        ok = sscanf(line, "mode %d", &mode) == 1 && mode >= AI_NONE && mode <= AI_HARD;
        ok = ok && readLine(f, line, sizeof line);
    }
    if (ok && strncmp(line, "clock ", 6) == 0) {               /* dong "clock" la tuy chon */
        ok = sscanf(line, "clock %d %ld %ld", &limit, &leftX, &leftO) == 3
             && (limit == TIME_5_MIN || limit == TIME_10_MIN || limit == TIME_15_MIN)
             && leftX > 0 && leftO > 0 && leftX <= limit * 1000L && leftO <= limit * 1000L;
        ok = ok && readLine(f, line, sizeof line);
    }
    ok = ok && strcmp(line, "board") == 0;
    for (int i = 0; ok && i < BOARD_SIZE; i++) {
        ok = readLine(f, line, sizeof line);
        const char *p = line;
        for (int j = 0; ok && j < BOARD_SIZE; j++) {
            char *end;
            long v = strtol(p, &end, 10);
            if (end == p) ok = 0;
            else { marks[i][j] = (int)v; p = end; }
        }
    }
    fclose(f);
    if (!ok) return SAVE_ERR_FORMAT;
    s.games = s.wins[0] + s.wins[1] + s.draws;
    if (loadState(marks, cr, cc, &s, elapsed) != 0) return SAVE_ERR_FORMAT;
    aiLevel = mode;                 /* chi doi che do va dong ho khi tai thanh cong */
    turnClockSet(limit, leftX, leftO);
    return SAVE_OK;
}

/* ---------------- lich su ----------------
 * Moi dong:  YYYY-MM-DD HH:MM|ketqua|ten X|ten O|so nuoc|so giay
 */
int historyAppend(int result) {
    char when[32];
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    if (tmv) strftime(when, sizeof when, "%Y-%m-%d %H:%M", tmv);
    else snprintf(when, sizeof when, "0000-00-00 00:00");

    FILE *f = fopen(HISTORY_FILE, "a");
    if (!f) return SAVE_ERR_IO;
    fprintf(f, "%s|%d|%s|%s|%d|%d\n", when, result, session.name[0], session.name[1],
            totalMoves, clockElapsed());
    return fclose(f) == 0 ? SAVE_OK : SAVE_ERR_IO;
}

int historyLoad(void) {
    historyCount = 0;
    FILE *f = fopen(HISTORY_FILE, "r");
    if (!f) return 0;
    char line[256];
    while (readLine(f, line, sizeof line)) {
        HistoryEntry e;
        memset(&e, 0, sizeof e);
        if (sscanf(line, "%16[^|]|%d|%60[^|]|%60[^|]|%d|%d", e.when, &e.result,
                   e.name[0], e.name[1], &e.moves, &e.seconds) != 6)
            continue;                                   /* bo qua dong hong */
        if (e.result != RES_X_WIN && e.result != RES_O_WIN && e.result != RES_DRAW) continue;
        if (historyCount == MAX_HISTORY) {              /* day: bo van cu nhat */
            memmove(&history[0], &history[1], sizeof(HistoryEntry) * (MAX_HISTORY - 1));
            historyCount--;
        }
        history[historyCount++] = e;
    }
    fclose(f);
    return historyCount;
}

int historyClear(void) {
    FILE *f = fopen(HISTORY_FILE, "w");
    historyCount = 0;
    if (!f) return SAVE_ERR_IO;
    return fclose(f) == 0 ? SAVE_OK : SAVE_ERR_IO;
}

/* ---------------- cai dat ---------------- */
void settingsLoad(void) {
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;                                     /* dung gia tri mac dinh */
    char line[256];
    while (readLine(f, line, sizeof line)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line, *val = eq + 1;
        if (strcmp(key, "sound") == 0) settings.soundOn = atoi(val) != 0;
        else if (strcmp(key, "volume") == 0) {
            int v = atoi(val);
            settings.volume = v < 0 ? 0 : (v > 100 ? 100 : v);
        }
        else if (strcmp(key, "name1") == 0) sanitizeName(settings.name[0], val);
        else if (strcmp(key, "name2") == 0) sanitizeName(settings.name[1], val);
        else if (strcmp(key, "lang") == 0) settings.lang = langFromCode(val);
    }
    fclose(f);
}

int settingsSave(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return SAVE_ERR_IO;
    fprintf(f, "sound=%d\nvolume=%d\nname1=%s\nname2=%s\nlang=%s\n", settings.soundOn, settings.volume,
            settings.name[0], settings.name[1], langCode(settings.lang));
    return fclose(f) == 0 ? SAVE_OK : SAVE_ERR_IO;
}

void settingsSetLanguage(int lang) {
    int from = settings.lang;
    int to = lang == LANG_EN ? LANG_EN : LANG_VI;
    for (int k = 0; k < 2; k++) {
        StrId def = k == 0 ? S_DEFAULT_PLAYER1 : S_DEFAULT_PLAYER2;
        if (strcmp(settings.name[k], trIn(from, def)) == 0)
            snprintf(settings.name[k], NAME_LEN, "%s", trIn(to, def));
    }
    settings.lang = to;
    langSet(to);
}
