#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h>
#include "control.h"
#include "view.h"
#include "model.h"
#include "storage.h"
#include "sound.h"

/* ---------------- phim ---------------- */
enum {
    KEY_BACKSPACE = 8, KEY_ENTER = 13, KEY_ESC = 27,
    KEY_UP = 1001, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_PGUP, KEY_PGDN
};
enum { PLAY_MENU = 0, PLAY_REPLAY = 1 };
enum { TEXT_NAME = 0, TEXT_FILE = 1 };

/* Doc 1 phim. W/A/S/D duoc doi thanh phim mui ten, chu thuong -> chu hoa.
 * Khi inGame != 0, trong luc cho phim van cap nhat dong ho tren man hinh. */
static int readKey(int inGame) {
    int shown = -1;
    for (;;) {
        while (!_kbhit()) {
            if (inGame) {
                int s = clockElapsed();
                if (s != shown) { shown = s; drawTime(); }
            }
            Sleep(30);
        }
        int c = _getch();
        if (c == 0 || c == 224) {              /* phim mo rong: mui ten, PgUp... */
            c = _getch();
            switch (c) {
                case 72: return KEY_UP;
                case 80: return KEY_DOWN;
                case 75: return KEY_LEFT;
                case 77: return KEY_RIGHT;
                case 73: return KEY_PGUP;
                case 81: return KEY_PGDN;
                default: continue;
            }
        }
        c = toupper(c);
        switch (c) {
            case 'W': return KEY_UP;
            case 'S': return KEY_DOWN;
            case 'A': return KEY_LEFT;
            case 'D': return KEY_RIGHT;
            default: return c;
        }
    }
}

/* Nhap mot chuoi. Tra ve 1 neu nhan Enter, 0 neu nhan Esc. */
static int readText(int x, int y, const char *label, char *buf, int cap, int mode) {
    int len = 0;
    buf[0] = '\0';
    showCursor(1);
    drawInputField(x, y, label, buf);
    for (;;) {
        int c = _getch();
        if (c == 0 || c == 224) { _getch(); continue; }
        if (c == KEY_ENTER) return 1;
        if (c == KEY_ESC) { buf[0] = '\0'; return 0; }
        if (c == KEY_BACKSPACE) {
            if (len > 0) buf[--len] = '\0';
        } else if (c >= 32 && c < 127 && len < cap - 1) {
            int ok = mode == TEXT_FILE ? (isalnum(c) || c == '_' || c == '-' || c == '.')
                                       : (c != '|');
            if (ok) { buf[len++] = (char)c; buf[len] = '\0'; }
        }
        drawInputField(x, y, label, buf);
    }
}

/* Hoi Y/N tren dong thong bao. */
static int confirm(const char *msg) {
    showMessage(msg, 14);
    int c = toupper(_getch());
    if (c == 0 || c == 224) _getch();
    clearMessage();
    return c == 'Y';
}

/* ---------------- di chuyen tren ban co ---------------- */
void checkLegalMove(void) {
    clampCursor();
    placeCursor();
}
void moveUp(void)    { currX--; checkLegalMove(); }
void moveDown(void)  { currX++; checkLegalMove(); }
void moveLeft(void)  { currY--; checkLegalMove(); }
void moveRight(void) { currY++; checkLegalMove(); }

/* ---------------- luu / tai (muc 4.1) ---------------- */

/* Hoi ten file va luu van hien tai. Tra ve 1 neu luu thanh cong. */
static int promptSave(int listX, int listY, int promptY) {
    FileName names[32];
    char name[FILE_NAME_LEN];
    int n = listSaves(names, 32);
    drawSaveList(listX, listY, names, n);
    int saved = 0;
    for (;;) {
        if (!readText(3, promptY, "Nhap ten file de luu (Esc: huy): ", name, FILE_BASE_MAX + 1, TEXT_FILE)
            || name[0] == '\0')
            break;
        normalizeFileName(name, sizeof name);
        if (fileExists(name)) {
            showCursor(0);
            if (!confirm("File da ton tai. Ghi de? (Y/N)")) { showCursor(1); continue; }
            showCursor(1);
        }
        int r = saveGame(name);
        if (r == SAVE_OK) {
            char msg[100];
            snprintf(msg, sizeof msg, "Da luu van co vao file: %s", name);
            showMessage(msg, 10);
            soundPlay(SND_SAVE);
            saved = 1;
            break;
        }
        showMessage("Khong ghi duoc file! Thu ten khac.", 12);
        soundPlay(SND_ERROR);
    }
    clearRegion(3, promptY, 70, 1);
    clearRegion(listX, listY, 30, 8);
    return saved;
}

/* Hoi ten file va tai van. Tra ve 1 neu tai thanh cong (du lieu da vao Model). */
static int promptLoad(int listX, int listY, int promptY) {
    FileName names[32];
    char name[FILE_NAME_LEN];
    int n = listSaves(names, 32);
    drawSaveList(listX, listY, names, n);
    int loaded = 0;
    for (;;) {
        if (!readText(3, promptY, "Nhap ten file can tai (Esc: huy): ", name, FILE_BASE_MAX + 1, TEXT_FILE)
            || name[0] == '\0')
            break;
        normalizeFileName(name, sizeof name);
        int r = loadGame(name);
        if (r == SAVE_OK) {
            char msg[100];
            snprintf(msg, sizeof msg, "Da tai van co tu file: %s", name);
            showMessage(msg, 10);
            soundPlay(SND_LOAD);
            loaded = 1;
            break;
        }
        showMessage(r == SAVE_ERR_IO ? "Khong mo duoc file (kiem tra lai ten)!"
                                     : "File khong hop le hoac da bi sua hong!", 12);
        soundPlay(SND_ERROR);
    }
    clearRegion(3, promptY, 70, 1);
    clearRegion(listX, listY, 30, 8);
    return loaded;
}

/* ---------------- ket thuc van (muc 4.2, 4.3) ---------------- */

/* Menu cuoi van (thiet ke cu cua nhom). Tra ve 1 = choi lai, 0 = ve menu. */
int end(void) {
    int curr = 5;                          /* dong cua lua chon hien tai: 5 = Choi lai, 6 = Ve menu */
    showCursor(0);
    endMenu(3, 6, 0, 15);
    for (;;) {
        setColor(0, 8);
        endMenuBorder(curr);
        int key = readKey(0);
        if (key == KEY_DOWN && curr == 5) {
            endMenu(0, 15, 3, 6);
            curr = 6;
            soundPlay(SND_MENU);
        } else if (key == KEY_UP && curr == 6) {
            endMenu(3, 6, 0, 15);
            curr = 5;
            soundPlay(SND_MENU);
        } else if (key == 'Y') {
            curr = 5;
            break;
        } else if (key == 'N' || key == KEY_ESC) {
            curr = 6;
            break;
        } else if (key == KEY_ENTER) {
            break;
        }
    }
    setColor(7, 0);
    showCursor(1);
    return curr == 5;
}

/* Xu ly mot van da co ket qua: ghi diem, ghi lich su, hieu ung, am thanh, menu cuoi van. */
static int processFinish(int res) {
    clockStop();
    recordResult(res);
    historyAppend(res);
    winPlayer(res);
    drawHud();
    soundPlay(res == RES_DRAW ? SND_DRAW : SND_WIN);
    winEffect(res);
    return end() ? PLAY_REPLAY : PLAY_MENU;
}

/* ---------------- vong lap mot van ---------------- */

/* ESC: hoi co luu khong. Tra ve 1 neu thoat ve menu. */
static int askLeave(void) {
    showMessage("Thoat ve menu?  Y: luu roi thoat   N: thoat khong luu   phim khac: o lai", 14);
    int c = toupper(_getch());
    if (c == 0 || c == 224) _getch();
    clearMessage();
    if (c == 'Y') return promptSave(PANEL_X, TOP + 21, PROMPT_Y);
    return c == 'N';
}

static int gameLoop(void) {
    for (;;) {
        int key = readKey(1);
        switch (key) {
            case KEY_UP:    moveUp(); break;
            case KEY_DOWN:  moveDown(); break;
            case KEY_LEFT:  moveLeft(); break;
            case KEY_RIGHT: moveRight(); break;

            case KEY_ENTER: {
                if (!checkLegalEnter(turn)) {          /* o da duoc danh */
                    soundPlay(SND_ERROR);
                    showMessage("O nay da duoc danh roi, hay chon o khac!", 12);
                    break;
                }
                clearMessage();
                printChessChar(turn);
                int res = testBoard();
                if (res != RES_NONE) return processFinish(res);
                soundPlay(turn == 1 ? SND_PLACE_X : SND_PLACE_O);
                switchTurn();
                drawHud();
                break;
            }

            case 'L':                                   /* luu van */
                clockStop();
                promptSave(PANEL_X, TOP + 21, PROMPT_Y);
                showCursor(1);
                placeCursor();
                clockStart();
                break;

            case 'T':                                   /* tai van */
                clockStop();
                if (promptLoad(PANEL_X, TOP + 21, PROMPT_Y)) {
                    drawGameScreen();
                    showMessage("Da tai van co. Tiep tuc choi!", 10);
                }
                showCursor(1);
                placeCursor();
                clockStart();
                break;

            case KEY_ESC:
                clockStop();
                if (askLeave()) return PLAY_MENU;
                placeCursor();
                clockStart();
                break;
        }
    }
}

static void startGame(void) {
    resetData();
    drawGameScreen();
    clockStart();
}

/* Choi lien tiep nhieu van cho den khi nguoi dung ve menu. */
static void playMatch(void) {
    while (gameLoop() == PLAY_REPLAY) {
        soundPlay(SND_START);
        startGame();
    }
}

static void newGame(void) {
    resetSession();
    soundPlay(SND_START);
    startGame();
    playMatch();
}

static void loadFromMenu(void) {
    clearScreen();
    setColor(14, 0);
    goToXY(3, 2);
    printf("LOAD GAME - tai van co da luu");
    setColor(7, 0);
    resetSession();
    resetData();
    if (promptLoad(3, 5, 24)) {
        drawGameScreen();
        showMessage("Da tai van co. Tiep tuc choi!", 10);
        clockStart();
        playMatch();
    }
}

/* ---------------- menu chinh, cai dat, lich su (muc 4.5) ---------------- */
static int runMenu(const char *title, const char *const items[], int n) {
    int sel = 0;
    clearScreen();
    showCursor(0);
    for (;;) {
        drawMenu(title, items, n, sel);
        int key = readKey(0);
        if (key == KEY_UP) { sel = (sel + n - 1) % n; soundPlay(SND_MENU); }
        else if (key == KEY_DOWN) { sel = (sel + 1) % n; soundPlay(SND_MENU); }
        else if (key == KEY_ENTER) return sel;
        else if (key == KEY_ESC) return -1;
    }
}

static void adjustVolume(int delta) {
    int v = settings.volume + delta;
    settings.volume = v < 0 ? 0 : (v > 100 ? 100 : v);
    soundConfigure(settings.soundOn, settings.volume);
    soundPlay(SND_PLACE_X);                    /* nghe thu muc am luong moi */
}

static void toggleSound(void) {
    settings.soundOn = !settings.soundOn;
    soundConfigure(settings.soundOn, settings.volume);
    soundPlay(SND_PLACE_X);
}

static void settingsScreen(void) {
    int sel = 0;
    clearScreen();
    showCursor(0);
    for (;;) {
        drawSettings(sel);
        int key = readKey(0);
        if (key == KEY_UP) { sel = (sel + SETTINGS_ITEMS - 1) % SETTINGS_ITEMS; soundPlay(SND_MENU); }
        else if (key == KEY_DOWN) { sel = (sel + 1) % SETTINGS_ITEMS; soundPlay(SND_MENU); }
        else if (key == KEY_LEFT || key == KEY_RIGHT) {
            if (sel == 0) toggleSound();
            else if (sel == 1) adjustVolume(key == KEY_RIGHT ? 10 : -10);
        }
        else if (key == KEY_ENTER) {
            if (sel == 0) toggleSound();
            else if (sel == 1) adjustVolume(10);
            else if (sel == 2 || sel == 3) {
                char buf[NAME_LEN];
                int who = sel - 2;
                if (readText(20, 9 + 2 * SETTINGS_ITEMS + 1, "Nhap ten moi (Enter: giu nguyen): ", buf, NAME_LEN, TEXT_NAME)
                    && buf[0] != '\0')
                    sanitizeName(settings.name[who], buf);
                clearRegion(20, 9 + 2 * SETTINGS_ITEMS + 1, 70, 1);
                showCursor(0);
            }
            else if (sel == 4) soundPlay(SND_WIN);
            else break;
        }
        else if (key == KEY_ESC) break;
    }
    settingsSave();
}

static void historyScreen(void) {
    int off = 0;
    historyLoad();
    clearScreen();
    showCursor(0);
    for (;;) {
        int maxOff = historyCount > HISTORY_ROWS ? historyCount - HISTORY_ROWS : 0;
        if (off > maxOff) off = maxOff;
        if (off < 0) off = 0;
        drawHistory(off);
        int key = readKey(0);
        if (key == KEY_UP && off > 0) off--;
        else if (key == KEY_DOWN) off++;
        else if (key == KEY_PGUP) off -= HISTORY_ROWS;
        else if (key == KEY_PGDN) off += HISTORY_ROWS;
        else if (key == 'C' && historyCount > 0) {
            if (confirm("Xoa TOAN BO lich su dau? (Y/N)")) {
                historyClear();
                off = 0;
                soundPlay(SND_SAVE);
            }
        }
        else if (key == KEY_ESC) break;
    }
}

void mainMenu(void) {
    static const char *const items[] = { "New Game", "Load Game", "History", "Settings", "Exit" };
    for (;;) {
        switch (runMenu("C A R O", items, 5)) {
            case 0: newGame(); break;
            case 1: loadFromMenu(); break;
            case 2: historyScreen(); break;
            case 3: settingsScreen(); break;
            default: return;
        }
    }
}

void out(void) {
    soundStop();
    setColor(7, 0);
    clearScreen();
    showCursor(1);
}
