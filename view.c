#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include "view.h"
#include "ai.h"
#include "utf8.h"
#include "lang.h"

/* Mau: 0 den, 1 xanh duong, 2 xanh la, 3 xanh ngoc, 4 do, 5 tim, 6 vang, 7 trang,
 *      8 xam, 9..15 la ban sang cua 1..7 */
#define COLOR_X 4
#define COLOR_O 1
#define COLOR_TITLE 14
#define COLOR_HILITE_BG 11
#define COLOR_CURSOR_BG 7   /* nen cua o dang chon */

static int markColor(int mark) { return mark == 1 ? COLOR_X : COLOR_O; }
static char markChar(int mark) { return mark == 1 ? 'X' : 'O'; }

/* Trang thai to sang o dang chon. boardShown = 0 khi man hinh khong phai ban co
 * (menu, lich su...) de placeCursor khong ve len cac man hinh do. */
static int boardShown = 0;
static int hlRow = -1, hlCol = -1;

/* ================= co ban ================= */

/* In chuoi UTF-8 tai vi tri con tro console. Moi chu tren man hinh deu di qua day:
 * doi sang UTF-16 roi WriteConsoleW nen tieng Viet co dau hien dung, khong phu
 * thuoc bang ma (code page) cua console. */
void printText(const char *s) {
    WCHAR wbuf[512];
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, wbuf, (int)(sizeof wbuf / sizeof wbuf[0]));
    if (n <= 1) return;                         /* chuoi rong (n tinh ca '\0') */
    DWORD written;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wbuf, (DWORD)(n - 1), &written, NULL);
}

void printAt(int x, int y, const char *s) {
    goToXY(x, y);
    printText(s);
}

static void printChar(char c) {
    char s[2] = { c, '\0' };
    printText(s);
}

static void printSpaces(int n) {
    char s[SCREEN_W + 1];
    if (n > SCREEN_W) n = SCREEN_W;
    if (n <= 0) return;
    memset(s, ' ', (size_t)n);
    s[n] = '\0';
    printText(s);
}

void goToXY(int x, int y) {
    COORD coord = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setColor(int textColor, int bgColor) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)(bgColor << 4 | textColor));
}

void clearScreen(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    COORD origin = { 0, 0 };
    boardShown = 0;
    hlRow = hlCol = -1;
    setColor(7, 0);
    if (!GetConsoleScreenBufferInfo(h, &csbi)) return;
    DWORD size = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    FillConsoleOutputCharacterA(h, ' ', size, origin, &written);
    FillConsoleOutputAttribute(h, 7, size, origin, &written);
    SetConsoleCursorPosition(h, origin);
}

void showCursor(int visible) {
    CONSOLE_CURSOR_INFO info = { 25, visible ? TRUE : FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

/* Font raster cu cua console khong co chu tieng Viet -> doi sang Consolas (font TrueType
 * co san tren moi ban Windows). Console dang dung font TrueType thi giu nguyen. */
/* Tu khai bao (giong CONSOLE_FONT_INFOEX cua Windows) va nap ham dong tu kernel32,
 * vi MinGW.org cu khong co san 2 ham nay trong header. */
typedef struct {
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT FontFamily;
    UINT FontWeight;
    WCHAR FaceName[32];
} FontInfoEx;
typedef BOOL (WINAPI *FontFn)(HANDLE, BOOL, FontInfoEx *);

static void useUnicodeFont(void) {
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (!k32) return;
    FontFn getFont = (FontFn)(void (*)(void))GetProcAddress(k32, "GetCurrentConsoleFontEx");
    FontFn setFont = (FontFn)(void (*)(void))GetProcAddress(k32, "SetCurrentConsoleFontEx");
    if (!getFont || !setFont) return;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    FontInfoEx fi;
    memset(&fi, 0, sizeof fi);
    fi.cbSize = sizeof fi;
    if (!getFont(h, FALSE, &fi)) return;
    if (fi.FontFamily & TMPF_TRUETYPE) return;  /* da la font TrueType (Consolas, Cascadia...) */
    fi.nFont = 0;
    fi.dwFontSize.X = 0;
    if (fi.dwFontSize.Y < 14) fi.dwFontSize.Y = 16;
    fi.FontFamily = FF_DONTCARE;
    fi.FontWeight = FW_NORMAL;
    wcscpy(fi.FaceName, L"Consolas");
    setFont(h, FALSE, &fi);
}

void fixConsole(void) {
    useUnicodeFont();
    SetConsoleOutputCP(CP_UTF8);                /* cho chac: cac ham ...A cung hieu UTF-8 */
    HWND consoleWindow = GetConsoleWindow();
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
    style = style & ~(WS_MAXIMIZEBOX) & ~(WS_THICKFRAME);
    SetWindowLong(consoleWindow, GWL_STYLE, style);
}

void resizeConsole(int width, int height) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    /* Thu nho cua so truoc, doi bo dem, roi moi phong cua so: tranh loi khi bo dem < cua so */
    SMALL_RECT tiny = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &tiny);
    COORD coord = { (short)width, (short)height };
    SetConsoleScreenBufferSize(hConsole, coord);
    SMALL_RECT rect = { 0, 0, (short)(width - 1), (short)(height - 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &rect);
}

/* ================= man hinh choi ================= */
void drawBoard(void) {
    setColor(7, 0);
    for (int i = 0; i <= BOARD_SIZE; ++i) {
        for (int j = 0; j <= BOARD_SIZE; ++j) {
            goToXY(LEFT + 4 * j, TOP + 2 * i);
            printText(j < BOARD_SIZE ? "+---" : "+");
            if (i < BOARD_SIZE) {
                goToXY(LEFT + 4 * j, TOP + 2 * i + 1);
                printText("|");
            }
        }
    }
}

static void paintCell(int i, int j, int textColor, int bgColor) {
    goToXY(chessBoard[i][j].X, chessBoard[i][j].Y);
    setColor(textColor, bgColor);
    printChar(markChar(chessBoard[i][j].mark));
}

void drawMarks(void) {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (chessBoard[i][j].mark != 0)
                paintCell(i, j, markColor(chessBoard[i][j].mark), 0);
    setColor(7, 0);
}

/* Ve lai phan trong cua 1 o (3 ky tu giua 2 vach '|'), to nen neu dang chon. */
static void drawCell(int i, int j, int highlighted) {
    goToXY(chessBoard[i][j].X - 1, chessBoard[i][j].Y);
    if (highlighted) {
        setColor(0, COLOR_CURSOR_BG);
        printText(" ");
        currPosHighlight(chessBoard[i][j].mark);
        setColor(0, COLOR_CURSOR_BG);
        printText(" ");
    } else {
        int mark = chessBoard[i][j].mark;
        setColor(mark != 0 ? markColor(mark) : 7, 0);
        char cell[4] = { ' ', mark != 0 ? markChar(mark) : ' ', ' ', '\0' };
        printText(cell);
    }
    setColor(7, 0);
}

/* Dua con tro ve o hien hanh va to sang o do (bo to sang o cu). */
void placeCursor(void) {
    if (boardShown) {
        if (hlRow >= 0 && (hlRow != currX || hlCol != currY))
            drawCell(hlRow, hlCol, 0);
        drawCell(currX, currY, 1);
        hlRow = currX;
        hlCol = currY;
    }
    goToXY(chessBoard[currX][currY].X, chessBoard[currX][currY].Y);
}

/* Bo to sang o dang chon (dung truoc hieu ung thang/menu cuoi van). */
static void hideHighlight(void) {
    if (boardShown && hlRow >= 0) drawCell(hlRow, hlCol, 0);
    hlRow = hlCol = -1;
}

/* In s tai (x, y) trong dung `width` o: dai thi cat, ngan thi them khoang trang.
 * Dem theo KY TU (chu co dau 2-3 byte van chi chiem 1 o). */
static void printPad(int x, int y, const char *s, int width) {
    char buf[4 * SCREEN_W + 1];
    utf8Fit(buf, sizeof buf, s, width, 1);
    printAt(x, y, buf);
}

/* Noi s vao cuoi dst thanh 1 cot rong `width` o (dung cho bang lich su). */
static void appendCol(char *dst, size_t cap, const char *s, int width) {
    size_t len = strlen(dst);
    if (len < cap) utf8Fit(dst + len, cap - len, s, width, 1);
}

#define LOW_TIME_SEC 30     /* con duoi 30 giay -> dong ho chuyen mau do */

/* So mili giay tu luc chuong trinh chay. Tru theo kieu unsigned nen khong bi loi
 * khi may da bat lien tuc > 24,8 ngay (GetTickCount vuot qua gioi han cua long). */
long nowMs(void) {
    static DWORD base = 0;
    static int started = 0;
    if (!started) { base = GetTickCount(); started = 1; }
    return (long)(GetTickCount() - base);
}

/* Cap nhat cac dong thoi gian: dong ho con lai cua tung ben + tong thoi gian van. */
void drawTime(void) {
    char buf[48];
    long now = nowMs();
    for (int k = 0; k < 2; k++) {
        int mark = k == 0 ? 1 : -1;
        int s = turnClockShownSeconds(k, now);
        int color = s <= LOW_TIME_SEC ? 12 : (turn == mark ? 15 : 7);   /* ben dang toi luot: sang hon */
        snprintf(buf, sizeof buf, tr(S_HUD_TIME_LEFT), s / 60, s % 60, turn == mark ? "  <" : "");
        setColor(color, 0);
        printPad(PANEL_X, TOP + 7 + k * 4, buf, HUD_W);
    }
    int s = clockElapsed();
    snprintf(buf, sizeof buf, tr(S_HUD_TOTAL_TIME), s / 60, s % 60);
    setColor(7, 0);
    printPad(PANEL_X, TOP + 14, buf, HUD_W);
    placeCursor();
}

/* Van ket thuc vi het gio -> dong thong bao ghi them "HET GIO!". */
static int finishByTimeout = 0;
void setTimeoutFinish(int on) { finishByTimeout = on; }

void drawHud(void) {
    char buf[160];
    int x = PANEL_X;

    setColor(COLOR_TITLE, 0);
    printPad(x, TOP, "======== CARO ========", HUD_W);
    setColor(8, 0);
    snprintf(buf, sizeof buf, tr(S_HUD_LIMIT), timeLimitSec / 60);
    printPad(x, TOP + 1, buf, HUD_W);

    int p = playerIndex(turn);
    setColor(markColor(turn), 0);
    snprintf(buf, sizeof buf, tr(S_HUD_TURN), markChar(turn), session.name[p]);
    printPad(x, TOP + 2, buf, HUD_W);

    for (int k = 0; k < 2; k++) {
        int mark = k == 0 ? 1 : -1;
        int y = TOP + 4 + k * 4;
        setColor(markColor(mark), 0);
        snprintf(buf, sizeof buf, "%c [%c] %s", turn == mark ? '>' : ' ', markChar(mark), session.name[k]);
        printPad(x, y, buf, HUD_W);
        setColor(7, 0);
        snprintf(buf, sizeof buf, tr(S_HUD_MOVES), moveCount[k]);
        printPad(x, y + 1, buf, HUD_W);
        snprintf(buf, sizeof buf, tr(S_HUD_WINLOSS), session.wins[k],
                 session.games - session.wins[k] - session.draws);
        printPad(x, y + 2, buf, HUD_W);
    }

    setColor(7, 0);
    snprintf(buf, sizeof buf, tr(S_HUD_DRAWS), session.draws, session.games);
    printPad(x, TOP + 12, buf, HUD_W);
    setColor(8, 0);
    printPad(x, TOP + 13, aiModeText(aiLevel), HUD_W);
    drawTime();

    setColor(8, 0);
    printPad(x, HELP_Y, tr(S_HUD_KEYS_MOVE), HUD_W);
    printPad(x, HELP_Y + 1, tr(S_HUD_KEYS_ENTER), HUD_W);
    printPad(x, HELP_Y + 2, tr(S_HUD_KEYS_SAVE), HUD_W);
    printPad(x, HELP_Y + 3, tr(S_HUD_KEYS_ESC), HUD_W);
    if (aiLevel != AI_NONE) {                   /* di lai chi co khi choi voi may */
        snprintf(buf, sizeof buf, tr(S_HUD_UNDO), undoCount);
        printPad(x, HELP_Y + 4, buf, HUD_W);
    } else {
        printPad(x, HELP_Y + 4, "", HUD_W);
    }
    setColor(7, 0);
    placeCursor();
}

/* Xoa quan tai o (r, c) tren man hinh (sau khi di lai). */
void eraseCell(int r, int c) {
    if (boardShown) drawCell(r, c, 0);
}

void drawGameScreen(void) {
    showCursor(1);
    clearScreen();
    drawBoard();
    drawMarks();
    boardShown = 1;
    finishByTimeout = 0;
    drawHud();          /* drawHud -> placeCursor: to sang o dang chon */
}

void showMessage(const char *msg, int color) {
    setColor(color, 0);
    printPad(0, MSG_Y, msg, SCREEN_W - 1);
    setColor(7, 0);
    placeCursor();
}

void clearMessage(void) {
    showMessage("", 7);
}

void printChessChar(int t) {
    /* Di toi dung o hien hanh truoc khi in, khong phu thuoc vi tri con tro console */
    goToXY(chessBoard[currX][currY].X, chessBoard[currX][currY].Y);
    setColor(markColor(t), 0);
    printChar(markChar(t));
    setColor(7, 0);
    placeCursor();
}

static void drawBanner(int res, int color) {
    char buf[200], out[256];
    if (aiLevel != AI_NONE && res != RES_DRAW) {       /* dau voi may */
        int aiWon = res == (AI_MARK == 1 ? RES_X_WIN : RES_O_WIN);
        if (aiWon)
            snprintf(buf, sizeof buf, tr(S_AI_WON), aiName(aiLevel));
        else
            snprintf(buf, sizeof buf, tr(S_YOU_BEAT_AI),
                     session.name[playerIndex(-AI_MARK)], aiName(aiLevel));
    } else {
        switch (res) {
            case RES_X_WIN:
                snprintf(buf, sizeof buf, tr(S_X_WON), session.name[0]);
                break;
            case RES_O_WIN:
                snprintf(buf, sizeof buf, tr(S_O_WON), session.name[1]);
                break;
            default:
                snprintf(buf, sizeof buf, "%s", tr(S_DRAW));
        }
    }
    snprintf(out, sizeof out, "*** %s%s ***", finishByTimeout && res != RES_DRAW ? tr(S_TIMEOUT_PREFIX) : "", buf);
    showMessage(out, color);
}

void winPlayer(int res) {
    drawBanner(res, 14);
}

/* Hieu ung: chop sang day quan thang + doi mau dong chuc mung + (am thanh do control phat) */
void winEffect(int res) {
    hideHighlight();    /* khong de o dang chon de len day quan thang */
    if (res == RES_DRAW) {
        for (int k = 0; k < 6; k++) {
            int on = k % 2 == 0;
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                    if (chessBoard[i][j].mark != 0)
                        paintCell(i, j, on ? 0 : markColor(chessBoard[i][j].mark), on ? 14 : 0);
            drawBanner(res, on ? 14 : 11);
            Sleep(180);
        }
        drawMarks();
    } else {
        for (int k = 0; k < 10; k++) {
            int on = k % 2 == 0;
            for (int w = 0; w < winCount; w++)
                paintCell(winX[w], winY[w], on ? 0 : markColor(chessBoard[winX[w]][winY[w]].mark), on ? 14 : 0);
            drawBanner(res, on ? 14 : 12);
            Sleep(130);
        }
        for (int w = 0; w < winCount; w++)             /* giu day thang duoc to sang */
            paintCell(winX[w], winY[w], 0, 14);
        drawBanner(res, 14);
    }
    setColor(7, 0);
    /* Chi dua con tro ve o, khong to sang lai (van da ket thuc) */
    boardShown = 0;
    goToXY(chessBoard[currX][currY].X, chessBoard[currX][currY].Y);
}

/* Menu ket thuc van (giu nguyen thiet ke cu): dong 5 = "Choi lai", dong 6 = "Ve menu" */
void endMenu(int retryText, int retryBg, int exitText, int exitBg) {
    setColor(0, 15);
    for (int i = TOP + 2 * 5; i <= TOP + 2 * 7; ++i) {
        goToXY(LEFT + 4 * 4, i);
        printSpaces(17);
    }
    setColor(retryText, retryBg);
    goToXY(chessBoard[5][4].X + 2, chessBoard[5][4].Y);
    printText(tr(S_END_REPLAY));
    setColor(exitText, exitBg);
    goToXY(chessBoard[6][4].X + 2, chessBoard[6][4].Y);
    printText(tr(S_END_MENU));
}

void endMenuBorder(int row) {
    for (int i = row; i <= row + 1; ++i) {
        goToXY(LEFT + 4 * 4, TOP + 2 * i);
        printSpaces(17);
    }
    printAt(LEFT + 4 * 4, TOP + 2 * row + 1, " ");
    printAt(LEFT + 4 * 8, TOP + 2 * row + 1, " ");
}

/* In ky tu cua o dang chon (tai vi tri con tro hien tai) tren nen sang.
 * Mau quan giong mau tren ban co (truoc day O bi in mau xanh la, khong khop). */
void currPosHighlight(int mark) {
    if (mark == 0) {
        setColor(0, COLOR_CURSOR_BG);
        printText(" ");
    } else {
        setColor(markColor(mark), COLOR_CURSOR_BG);
        printChar(markChar(mark));
    }
}

/* ================= menu & man hinh phu (muc toi gian) ================= */
void clearRegion(int x, int y, int w, int h) {
    setColor(7, 0);
    for (int i = 0; i < h; i++) printPad(x, y + i, "", w);
}

void drawHelpLine(int y, const char *text) {
    setColor(8, 0);
    printPad(0, y, "", SCREEN_W - 1);
    int w = utf8Width(text);
    printPad((SCREEN_W - w) / 2, y, text, w);
    setColor(7, 0);
}

void drawMenu(const char *title, const char *const items[], int n, int sel) {
    int w = 32, x = (SCREEN_W - w) / 2;     /* du rong cho "Chơi với máy - Trung bình" */
    int tw = utf8Width(title);
    setColor(COLOR_TITLE, 0);
    printPad((SCREEN_W - tw) / 2, 5, title, tw);
    for (int i = 0; i < n; i++) {
        char buf[160];
        snprintf(buf, sizeof buf, "  %s", items[i]);     /* printPad them khoang trang cho du w o */
        setColor(i == sel ? 0 : 7, i == sel ? COLOR_HILITE_BG : 0);
        printPad(x, 9 + 2 * i, buf, w);
    }
    drawHelpLine(SCREEN_H - 2, tr(S_HELP_MENU));
}

void drawSettings(int sel) {
    char line[SETTINGS_ITEMS][160];
    char bar[11];
    int filled = settings.volume / 10;
    for (int k = 0; k < 10; k++) bar[k] = k < filled ? '#' : '-';
    bar[10] = '\0';
    snprintf(line[0], sizeof line[0], tr(S_SET_SOUND), tr(settings.soundOn ? S_ON : S_OFF));
    snprintf(line[1], sizeof line[1], tr(S_SET_VOLUME), bar, settings.volume);
    snprintf(line[2], sizeof line[2], tr(S_SET_NAME1), settings.name[0]);
    snprintf(line[3], sizeof line[3], tr(S_SET_NAME2), settings.name[1]);
    snprintf(line[4], sizeof line[4], tr(S_SET_LANGUAGE), tr(S_LANG_NAME));
    snprintf(line[5], sizeof line[5], "%s", tr(S_SET_TEST_SOUND));
    snprintf(line[6], sizeof line[6], "%s", tr(S_BACK));

    int w = 48, x = (SCREEN_W - w) / 2;
    setColor(COLOR_TITLE, 0);
    int tw = utf8Width(tr(S_TITLE_SETTINGS));
    printPad((SCREEN_W - tw) / 2, 5, tr(S_TITLE_SETTINGS), tw);
    for (int i = 0; i < SETTINGS_ITEMS; i++) {
        char buf[200];
        snprintf(buf, sizeof buf, "  %s", line[i]);
        setColor(i == sel ? 0 : 7, i == sel ? COLOR_HILITE_BG : 0);
        printPad(x, 9 + 2 * i, buf, w);
    }
    drawHelpLine(SCREEN_H - 3, tr(S_HELP_SETTINGS1));
    drawHelpLine(SCREEN_H - 2, tr(S_HELP_SETTINGS2));
}

void drawHistory(int offset) {
    char buf[4 * SCREEN_W + 1];
    int xw = 0, ow = 0, dw = 0;
    for (int i = 0; i < historyCount; i++) {
        if (history[i].result == RES_X_WIN) xw++;
        else if (history[i].result == RES_O_WIN) ow++;
        else dw++;
    }
    setColor(COLOR_TITLE, 0);
    printPad(2, 1, tr(S_TITLE_HISTORY), 40);
    setColor(7, 0);
    snprintf(buf, sizeof buf, tr(S_HIST_SUMMARY), historyCount, xw, ow, dw);
    printPad(2, 2, buf, SCREEN_W - 4);
    setColor(11, 0);
    /* cac cot (dem theo ky tu): #(3)  thoi diem(16)  nguoi X(12) vs nguoi O(12)  ket qua(8) nuoc(4)  gio */
    buf[0] = '\0';
    appendCol(buf, sizeof buf, "  #  ", 5);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_WHEN), 18);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_X), 16);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_O), 14);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_RESULT), 8);
    appendCol(buf, sizeof buf, " ", 1);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_MOVES), 6);
    appendCol(buf, sizeof buf, tr(S_HIST_COL_TIME), 5);
    printPad(2, 4, buf, SCREEN_W - 4);
    setColor(7, 0);
    for (int k = 0; k < HISTORY_ROWS; k++) {
        int idx = historyCount - 1 - offset - k;           /* moi nhat truoc */
        if (idx < 0) { printPad(2, 5 + k, "", SCREEN_W - 4); continue; }
        const HistoryEntry *e = &history[idx];
        const char *res = tr(e->result == RES_X_WIN ? S_HIST_X_WIN : (e->result == RES_O_WIN ? S_HIST_O_WIN : S_HIST_DRAW));
        char num[32];
        buf[0] = '\0';
        snprintf(num, sizeof num, "%3d  ", idx + 1);
        appendCol(buf, sizeof buf, num, 5);
        appendCol(buf, sizeof buf, e->when, 16);
        appendCol(buf, sizeof buf, "  ", 2);
        appendCol(buf, sizeof buf, e->name[0], 12);         /* ten dai bi cat con 12 ky tu */
        appendCol(buf, sizeof buf, " vs ", 4);
        appendCol(buf, sizeof buf, e->name[1], 12);
        appendCol(buf, sizeof buf, "  ", 2);
        appendCol(buf, sizeof buf, res, 8);
        snprintf(num, sizeof num, " %4d  %02d:%02d", e->moves, e->seconds / 60, e->seconds % 60);
        appendCol(buf, sizeof buf, num, 12);
        printPad(2, 5 + k, buf, SCREEN_W - 4);
    }
    if (historyCount == 0) {
        setColor(8, 0);
        printPad(2, 6, tr(S_HIST_EMPTY), 40);
    }
    drawHelpLine(SCREEN_H - 3, tr(S_HELP_HISTORY1));
    drawHelpLine(SCREEN_H - 2, tr(S_HELP_HISTORY2));
    setColor(7, 0);
}

/* O nhap lieu: dung het chieu rong con lai cua man hinh, de nhan + ten dai
 * khong bi cat va con tro khong chay ra ngoai phan da in. */
void drawInputField(int x, int y, const char *label, const char *text) {
    char buf[400];
    int width = SCREEN_W - x - 1;
    snprintf(buf, sizeof buf, "%s%s", label, text);
    setColor(7, 0);
    printPad(x, y, buf, width);
    int len = utf8Width(buf);                   /* con tro dat ngay sau ky tu cuoi (dem theo ky tu) */
    goToXY(x + (len < width ? len : width - 1), y);
}

void clearInputField(int x, int y) {
    clearRegion(x, y, SCREEN_W - x - 1, 1);
}

/* Danh sach file da luu: 1 dong tieu de + SAVE_LIST_ROWS dong ten + 1 dong "con N file". */
void drawSaveList(int x, int y, FileName *names, int n) {
    char buf[64];
    setColor(11, 0);
    printPad(x, y, tr(n > 0 ? S_SAVE_LIST_TITLE : S_SAVE_LIST_EMPTY), SAVE_LIST_W);
    setColor(7, 0);
    for (int k = 0; k < SAVE_LIST_ROWS; k++) {
        snprintf(buf, sizeof buf, "  %s", k < n ? names[k] : "");
        printPad(x, y + 1 + k, buf, SAVE_LIST_W);
    }
    setColor(8, 0);
    if (n > SAVE_LIST_ROWS) snprintf(buf, sizeof buf, tr(S_SAVE_LIST_MORE), n - SAVE_LIST_ROWS);
    else buf[0] = '\0';
    printPad(x, y + 1 + SAVE_LIST_ROWS, buf, SAVE_LIST_W);
    setColor(7, 0);
}

void clearSaveList(int x, int y) {
    clearRegion(x, y, SAVE_LIST_W, SAVE_LIST_ROWS + 2);
}
