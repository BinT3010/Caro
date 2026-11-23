#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "storage.h"

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

void sanitizeName(char *dst, const char *src) {
    char tmp[NAME_LEN];
    int n = 0;
    for (; *src && n < NAME_LEN - 1; src++) {
        unsigned char c = (unsigned char)*src;
        if (c >= 32 && c < 127 && c != '|') tmp[n++] = (char)c;
    }
    tmp[n] = '\0';
    int a = 0;                              /* cat khoang trang hai dau */
    while (tmp[a] == ' ') a++;
    int b = n;
    while (b > a && tmp[b - 1] == ' ') b--;
    tmp[b] = '\0';
    if (b - a <= 0) snprintf(dst, NAME_LEN, "Nguoi choi");
    else snprintf(dst, NAME_LEN, "%s", tmp + a);
}

void normalizeFileName(char *name, size_t cap) {
    if (strchr(name, '.') == NULL && strlen(name) + strlen(SAVE_EXT) < cap)
        strcat(name, SAVE_EXT);
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
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
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
        if (len > ext && len < FILE_NAME_LEN && strcmp(e->d_name + len - ext, SAVE_EXT) == 0 && n < max)
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
    int size = 0, elapsed = 0, cr = 0, cc = 0;
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
    ok = ok && readLine(f, line, sizeof line) && strcmp(line, "board") == 0;
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
        if (sscanf(line, "%16[^|]|%d|%20[^|]|%20[^|]|%d|%d", e.when, &e.result,
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
    }
    fclose(f);
}

int settingsSave(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return SAVE_ERR_IO;
    fprintf(f, "sound=%d\nvolume=%d\nname1=%s\nname2=%s\n", settings.soundOn, settings.volume,
            settings.name[0], settings.name[1]);
    return fclose(f) == 0 ? SAVE_OK : SAVE_ERR_IO;
}
