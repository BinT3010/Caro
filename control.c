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
#include "ai.h"
#include "utf8.h"

#define AI_DELAY_MS 350          /* may "suy nghi" mot chut cho nguoi choi kip nhin */

/* ---------------- phim ---------------- */
enum {
    KEY_BACKSPACE = 8, KEY_ENTER = 13, KEY_ESC = 27,
    KEY_UP = 1001, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_PGUP, KEY_PGDN,
    KEY_TIMEOUT                                 /* khong phai phim: ben dang toi luot het gio */
};
enum { PLAY_MENU = 0, PLAY_REPLAY = 1 };
enum { TEXT_NAME = 0, TEXT_FILE = 1 };



/* Dung / chay lai ca dong ho tong cua van va dong ho cua ben dang toi luot. */
static void pauseClocks(void) { clockStop(); turnClockStop(nowMs()); }
static void resumeClocks(void) { clockStart(); turnClockStart(nowMs()); }

/* Doc 1 phim. W/A/S/D duoc doi thanh phim mui ten, chu thuong -> chu hoa.
 * Khi inGame != 0: trong luc cho phim van cap nhat dong ho tren man hinh, va tra ve
 * KEY_TIMEOUT ngay khi ben dang toi luot het gio. */
static int readKey(int inGame) {
    long shown = -1;
    for (;;) {
        while (!_kbhit()) {
            if (inGame) {
                long now = nowMs();
                if (turnClockExpired(now)) return KEY_TIMEOUT;
                long s = clockElapsed() * 10000L + turnClockShownSeconds(playerIndex(turn), now);
                if (s != shown) { shown = s; drawTime(); }
            }
            Sleep(30);
        }
        if (inGame && turnClockExpired(nowMs())) return KEY_TIMEOUT;   /* bam phim khi vua het gio */
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

/* Doc 1 ky tu Unicode tu ban phim (ReadConsoleInputW). Bo qua cac phim khong ra chu
 * (mui ten, Shift, F1...). Dung thay _getwch vi MinGW.org cu khong khai bao ham nay. */
static unsigned readWideChar(void) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    for (;;) {
        INPUT_RECORD r;
        DWORD n = 0;
        if (!ReadConsoleInputW(h, &r, 1, &n)) return KEY_ESC;
        if (n == 0 || r.EventType != KEY_EVENT || !r.Event.KeyEvent.bKeyDown) continue;
        unsigned ch = r.Event.KeyEvent.uChar.UnicodeChar;
        if (ch != 0) return ch;
    }
}

/* Nhap mot chuoi (UTF-8) toi da maxChars ky tu, buf co cap byte.
 * Doc ky tu Unicode (readWideChar) nen go duoc tieng Viet bang Unikey/EVKey (kieu
 * "Unicode dung san"). Ten file chi nhan chu cai/so khong dau, '_', '-', '.'.
 * Tra ve 1 neu nhan Enter, 0 neu nhan Esc. */
static int readText(int x, int y, const char *label, char *buf, int cap, int maxChars, int mode) {
    size_t len = 0;
    int chars = 0;
    buf[0] = '\0';
    showCursor(1);
    drawInputField(x, y, label, buf);
    for (;;) {
        unsigned c = readWideChar();
        if (c == KEY_ENTER) return 1;
        if (c == KEY_ESC) { buf[0] = '\0'; return 0; }
        if (c == KEY_BACKSPACE) {
            if (chars > 0) { utf8PopLast(buf); len = strlen(buf); chars--; }
        } else {
            int ok;
            if (mode == TEXT_FILE)
                ok = c < 128 && (isalnum((int)c) || c == '_' || c == '-' || c == '.');
            else                                    /* bo ky tu dieu khien, '|', dau thanh roi, surrogate */
                ok = c >= 32 && c != 127 && c != '|' && !(c >= 0x80 && c < 0xA0)
                     && !(c >= 0x300 && c <= 0x36F) && !(c >= 0xD800 && c <= 0xDFFF);
            char enc[4];
            int n = ok ? utf8Encode(c, enc) : 0;
            if (ok && chars < maxChars && len + (size_t)n < (size_t)cap) {
                memcpy(buf + len, enc, (size_t)n);
                len += (size_t)n;
                buf[len] = '\0';
                chars++;
            }
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
        if (!readText(3, promptY, "Nhập tên file để lưu (Esc: hủy): ", name, FILE_BASE_MAX + 1, FILE_BASE_MAX, TEXT_FILE)
            || name[0] == '\0')
            break;
        if (!normalizeFileName(name, sizeof name)) {
            showMessage("Tên file không hợp lệ! (không dùng CON, NUL, COM1...; không bắt đầu bằng '.')", 12);
            soundPlay(SND_ERROR);
            continue;
        }
        if (fileExists(name)) {
            showCursor(0);
            if (!confirm("File đã tồn tại. Ghi đè? (Y: có / N: không)")) { showCursor(1); continue; }
            showCursor(1);
        }
        int r = saveGame(name);
        if (r == SAVE_OK) {
            char msg[100];
            snprintf(msg, sizeof msg, "Đã lưu ván cờ vào file: %s", name);
            showMessage(msg, 10);
            soundPlay(SND_SAVE);
            saved = 1;
            break;
        }
        showMessage("Không ghi được file! Thử tên khác.", 12);
        soundPlay(SND_ERROR);
    }
    clearInputField(3, promptY);
    clearSaveList(listX, listY);
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
        if (!readText(3, promptY, "Nhập tên file cần tải (Esc: hủy): ", name, FILE_BASE_MAX + 1, FILE_BASE_MAX, TEXT_FILE)
            || name[0] == '\0')
            break;
        if (!normalizeFileName(name, sizeof name)) {
            showMessage("Tên file không hợp lệ!", 12);
            soundPlay(SND_ERROR);
            continue;
        }
        int r = loadGame(name);
        if (r == SAVE_OK) {
            char msg[100];
            snprintf(msg, sizeof msg, "Đã tải ván cờ từ file: %s", name);
            showMessage(msg, 10);
            soundPlay(SND_LOAD);
            loaded = 1;
            break;
        }
        showMessage(r == SAVE_ERR_IO ? "Không mở được file (kiểm tra lại tên)!"
                                     : "File không hợp lệ hoặc đã bị sửa hỏng!", 12);
        soundPlay(SND_ERROR);
    }
    clearInputField(3, promptY);
    clearSaveList(listX, listY);
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
    pauseClocks();
    recordResult(res);
    historyAppend(res);
    winPlayer(res);
    drawHud();
    int lostToAi = aiLevel != AI_NONE && res == (AI_MARK == 1 ? RES_X_WIN : RES_O_WIN);
    soundPlay(res == RES_DRAW || lostToAi ? SND_DRAW : SND_WIN);
    winEffect(res);
    return end() ? PLAY_REPLAY : PLAY_MENU;
}

/* ---------------- vong lap mot van ---------------- */

/* ESC: hoi co luu khong. Tra ve 1 neu thoat ve menu. */
static int askLeave(void) {
    showMessage("Thoát về menu?  Y: lưu rồi thoát   N: thoát không lưu   phím khác: ở lại", 14);
    int c = toupper(_getch());
    if (c == 0 || c == 224) _getch();
    clearMessage();
    if (c == 'Y') return promptSave(PANEL_X, GAME_LIST_Y, PROMPT_Y);
    return c == 'N';
}

/* Xu ly sau khi quan `turn` da duoc dat tai (currX, currY): ve quan, kiem tra
 * thang/hoa, doi luot. Dung chung cho nguoi choi va may. */
static int finishMove(void) {
    printChessChar(turn);
    int res = testBoard();
    if (res != RES_NONE) return res;
    soundPlay(turn == 1 ? SND_PLACE_X : SND_PLACE_O);
    long now = nowMs();
    turnClockStop(now);                            /* dung dong ho ben vua danh ... */
    switchTurn();
    turnClockStart(now);                           /* ... va chay dong ho ben kia */
    drawHud();
    return RES_NONE;
}

/* Ben dang toi luot (turn) het gio -> ben kia thang. */
static int timeoutResult(void) {
    setTimeoutFinish(1);
    return turn == 1 ? RES_O_WIN : RES_X_WIN;
}

/* Luot cua may. Tra ve ket qua van co (RES_NONE neu van tiep tuc).
 * Dong ho cua may cung chay trong luc may "suy nghi". */
static int aiMove(void) {
    showMessage("Máy đang suy nghĩ...", 8);
    DWORD start = GetTickCount();
    int r = 0, c = 0;
    if (!aiChooseMove(aiLevel, turn, &r, &c)) return RES_DRAW;   /* khong con o trong */
    DWORD used = GetTickCount() - start;           /* muc Kho co the nghi lau hon */
    if (used < AI_DELAY_MS) Sleep(AI_DELAY_MS - used);
    while (_kbhit()) _getch();                     /* bo cac phim bam trong luc may nghi */
    clearMessage();
    if (turnClockExpired(nowMs())) return timeoutResult();   /* may het gio truoc khi kip danh */
    currX = r;
    currY = c;                                     /* con tro nhay toi nuoc may vua danh */
    if (!checkLegalEnter(turn)) return RES_DRAW;   /* khong xay ra: AI luon chon o trong */
    return finishMove();
}

/* Di lai (phim U) - CHI khi choi voi may: xoa nuoc vua roi cua may va nuoc cua
 * nguoi choi ngay truoc do, tra lai luot cho nguoi choi tai dung o do. */
static void undoVsAi(void) {
    if (aiLevel == AI_NONE) {
        soundPlay(SND_ERROR);
        showMessage("Chỉ được đi lại khi chơi với máy!", 12);
        return;
    }
    int human = -AI_MARK;
    int need = lastMoveMark() == AI_MARK ? 2 : 1;   /* nuoc cua may + nuoc cua nguoi */
    if (moveHistoryLength() < need) {
        soundPlay(SND_ERROR);
        showMessage("Không còn nước nào để đi lại!", 12);
        return;
    }
    int r, c, hr = currX, hc = currY;
    if (lastMoveMark() == AI_MARK && undoMove(&r, &c)) eraseCell(r, c);
    if (lastMoveMark() == human && undoMove(&r, &c)) { eraseCell(r, c); hr = r; hc = c; }
    undoCount++;
    currX = hr;
    currY = hc;                                     /* con tro ve o nguoi choi vua danh */
    soundPlay(SND_LOAD);
    drawHud();
    showMessage("Đã đi lại. Đến lượt bạn đánh lại.", 10);
}

static int gameLoop(void) {
    for (;;) {
        if (aiLevel != AI_NONE && turn == AI_MARK) {
            int res = aiMove();
            if (res != RES_NONE) return processFinish(res);
            continue;
        }
        int key = readKey(1);
        switch (key) {
            case KEY_UP:    moveUp(); break;
            case KEY_DOWN:  moveDown(); break;
            case KEY_LEFT:  moveLeft(); break;
            case KEY_RIGHT: moveRight(); break;

            case KEY_ENTER: {
                if (!checkLegalEnter(turn)) {          /* o da duoc danh */
                    soundPlay(SND_ERROR);
                    showMessage("Ô này đã được đánh rồi, hãy chọn ô khác!", 12);
                    break;
                }
                clearMessage();
                int res = finishMove();
                if (res != RES_NONE) return processFinish(res);
                break;
            }

            case KEY_TIMEOUT:                           /* nguoi dang toi luot het gio */
                return processFinish(timeoutResult());

            case 'U':                                   /* di lai (chi khi choi voi may) */
                undoVsAi();
                break;

            case 'L':                                   /* luu van (dong ho tam dung) */
                pauseClocks();
                promptSave(PANEL_X, GAME_LIST_Y, PROMPT_Y);
                drawHud();                              /* ve lai phan huong dan bi danh sach file de len */
                showCursor(1);
                placeCursor();
                resumeClocks();
                break;

            case 'T':                                   /* tai van */
                pauseClocks();
                if (promptLoad(PANEL_X, GAME_LIST_Y, PROMPT_Y)) {
                    drawGameScreen();
                    showMessage("Đã tải ván cờ. Tiếp tục chơi!", 10);
                } else {
                    drawHud();
                }
                showCursor(1);
                placeCursor();
                resumeClocks();
                break;

            case KEY_ESC:
                pauseClocks();
                if (askLeave()) return PLAY_MENU;
                drawHud();                              /* co the da hien danh sach file khi chon Y roi huy */
                showCursor(1);
                placeCursor();
                resumeClocks();
                break;
        }
    }
}

static void startGame(void) {
    resetData();                                /* ca 2 ben day gio (timeLimitSec) */
    drawGameScreen();
    resumeClocks();
}

/* Choi lien tiep nhieu van cho den khi nguoi dung ve menu. */
static void playMatch(void) {
    while (gameLoop() == PLAY_REPLAY) {
        soundPlay(SND_START);
        startGame();
    }
}

/* Man hinh dat ten truoc khi thi dau. 2 nguoi: nhap ten X va O; choi voi may: chi nhap ten nguoi.
 * Enter de trong hoac Esc = giu ten mac dinh (lay tu Settings). Ten chi ap dung cho phien nay. */
static void askPlayerNames(int level) {
    static const char marks[2] = { 'X', 'O' };
    int count = (level == AI_NONE) ? 2 : 1;
    char label[200], buf[NAME_LEN];
    clearScreen();
    setColor(14, 0);
    printAt(3, 2, "ĐẶT TÊN NGƯỜI CHƠI");
    setColor(8, 0);
    snprintf(label, sizeof label, "Enter để trống hoặc Esc: giữ tên mặc định. Tối đa %d ký tự, gõ được tiếng Việt.",
             NAME_CHARS);
    printAt(3, 3, label);
    setColor(7, 0);
    for (int who = 0; who < count; who++) {
        int y = 6 + 2 * who;
        snprintf(label, sizeof label, "Người chơi %c (mặc định \"%s\"): ", marks[who], session.name[who]);
        for (;;) {
            if (!readText(3, y, label, buf, NAME_LEN, NAME_CHARS, TEXT_NAME)) buf[0] = '\0';   /* Esc = giu ten */
            if (setPlayerName(who, buf) != NAME_DUPLICATE) break;
            showMessage("Tên bị trùng với người chơi còn lại, hãy nhập tên khác!", 12);
        }
        clearMessage();
        clearInputField(3, y);
        snprintf(label, sizeof label, "Người chơi %c: ", marks[who]);
        drawInputField(3, y, label, session.name[who]);
    }
    if (level != AI_NONE) {
        snprintf(label, sizeof label, "Đối thủ       O: ");
        drawInputField(3, 8, label, session.name[playerIndex(AI_MARK)]);
    }
    showCursor(0);
    Sleep(900);                                 /* cho nguoi choi nhin lai ten vua dat */
}

static void newGame(int level, int limitSeconds) {
    resetSession();
    aiLevel = level;
    timeLimitSec = limitSeconds;                /* giu nguyen cho cac van choi lai */
    if (level != AI_NONE)                       /* may cam quan O */
        snprintf(session.name[playerIndex(AI_MARK)], NAME_LEN, "%s", aiName(level));
    askPlayerNames(level);
    soundPlay(SND_START);
    startGame();
    playMatch();
}

static void loadFromMenu(void) {
    clearScreen();
    setColor(14, 0);
    printAt(3, 2, "TẢI VÁN - tải ván cờ đã lưu");
    setColor(7, 0);
    resetSession();
    resetData();
    if (promptLoad(3, 5, 24)) {
        drawGameScreen();
        showMessage("Đã tải ván cờ. Tiếp tục chơi!", 10);
        resumeClocks();
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
            else if (sel == 1) adjustVolume(settings.volume >= 100 ? -100 : 10);  /* 100% -> quay ve 0% */
            else if (sel == 2 || sel == 3) {
                char buf[NAME_LEN];
                int who = sel - 2;
                if (readText(20, 9 + 2 * SETTINGS_ITEMS + 1, "Nhập tên mới (Enter: giữ nguyên): ", buf, NAME_LEN, NAME_CHARS, TEXT_NAME)
                    && buf[0] != '\0')
                    sanitizeName(settings.name[who], buf);
                clearInputField(20, 9 + 2 * SETTINGS_ITEMS + 1);
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
            if (confirm("Xóa TOÀN BỘ lịch sử đấu? (Y: có / N: không)")) {
                historyClear();
                off = 0;
                soundPlay(SND_SAVE);
            }
        }
        else if (key == KEY_ESC) break;
    }
}

/* Chon thoi gian cho moi ben. Tra ve so giay, 0 neu quay lai. */
static int timeMenu(void) {
    static const char *const items[] = { "5 phút mỗi bên", "10 phút mỗi bên", "15 phút mỗi bên", "Quay lại" };
    switch (runMenu("THỜI GIAN", items, 4)) {
        case 0: return TIME_5_MIN;
        case 1: return TIME_10_MIN;
        case 2: return TIME_15_MIN;
        default: return 0;                      /* Quay lai / Esc */
    }
}

/* New Game: chon che do choi, roi chon thoi gian (Quay lai o buoc thoi gian -> chon lai che do). */
static void newGameMenu(void) {
    static const char *const items[] = { "2 người chơi", "Chơi với máy - Dễ", "Chơi với máy - Trung bình",
                                         "Chơi với máy - Khó", "Quay lại" };
    static const int levels[] = { AI_NONE, AI_EASY, AI_MEDIUM, AI_HARD };
    for (;;) {
        int sel = runMenu("VÁN MỚI", items, 5);
        if (sel < 0 || sel > 3) return;         /* Quay lai / Esc */
        int limit = timeMenu();
        if (limit == 0) continue;
        newGame(levels[sel], limit);
        return;
    }
}

void mainMenu(void) {
    static const char *const items[] = { "Ván mới", "Tải ván", "Lịch sử đấu", "Cài đặt", "Thoát" };
    for (;;) {
        switch (runMenu("C A R O", items, 5)) {
            case 0: newGameMenu(); break;
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
