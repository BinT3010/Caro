#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "view.h"
#include "ai.h"

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

void fixConsole(void) {
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
            printf("+");
            if (j < BOARD_SIZE)
                printf("---");
            if (i < BOARD_SIZE) {
                goToXY(LEFT + 4 * j, TOP + 2 * i + 1);
                printf("|");
            }
        }
    }
}

static void paintCell(int i, int j, int textColor, int bgColor) {
    goToXY(chessBoard[i][j].X, chessBoard[i][j].Y);
    setColor(textColor, bgColor);
    printf("%c", markChar(chessBoard[i][j].mark));
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
        printf(" ");
        currPosHighlight(chessBoard[i][j].mark);
        setColor(0, COLOR_CURSOR_BG);
        printf(" ");
    } else {
        int mark = chessBoard[i][j].mark;
        setColor(mark != 0 ? markColor(mark) : 7, 0);
        printf(" %c ", mark != 0 ? markChar(mark) : ' ');
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

static void printPad(int x, int y, const char *s, int width) {
    goToXY(x, y);
    printf("%-*.*s", width, width, s);
}

void drawTime(void) {
    char buf[40];
    int s = clockElapsed();
    snprintf(buf, sizeof buf, "Thoi gian : %02d:%02d", s / 60, s % 60);
    setColor(7, 0);
    printPad(PANEL_X, TOP + 14, buf, HUD_W);
    placeCursor();
}

void drawHud(void) {
    char buf[64];
    int x = PANEL_X;

    setColor(COLOR_TITLE, 0);
    printPad(x, TOP, "======== CARO ========", HUD_W);

    int p = playerIndex(turn);
    setColor(markColor(turn), 0);
    snprintf(buf, sizeof buf, "Luot choi: %c (%s)", markChar(turn), session.name[p]);
    printPad(x, TOP + 2, buf, HUD_W);

    for (int k = 0; k < 2; k++) {
        int mark = k == 0 ? 1 : -1;
        int y = TOP + 4 + k * 4;
        setColor(markColor(mark), 0);
        snprintf(buf, sizeof buf, "%c [%c] %s", turn == mark ? '>' : ' ', markChar(mark), session.name[k]);
        printPad(x, y, buf, HUD_W);
        setColor(7, 0);
        snprintf(buf, sizeof buf, "    Nuoc di: %d", moveCount[k]);
        printPad(x, y + 1, buf, HUD_W);
        snprintf(buf, sizeof buf, "    Thang: %d   Thua: %d", session.wins[k],
                 session.games - session.wins[k] - session.draws);
        printPad(x, y + 2, buf, HUD_W);
    }

    setColor(7, 0);
    snprintf(buf, sizeof buf, "Hoa: %d    Tong so van: %d", session.draws, session.games);
    printPad(x, TOP + 12, buf, HUD_W);
    setColor(8, 0);
    printPad(x, TOP + 13, aiModeText(aiLevel), HUD_W);
    drawTime();

    setColor(8, 0);
    printPad(x, HELP_Y, "W/A/S/D hoac phim mui ten: di chuyen", HUD_W);
    printPad(x, HELP_Y + 1, "Enter: danh co", HUD_W);
    printPad(x, HELP_Y + 2, "L: luu van    T: tai van", HUD_W);
    printPad(x, HELP_Y + 3, "Esc: thoat ve menu", HUD_W);
    if (aiLevel != AI_NONE) {                   /* di lai chi co khi choi voi may */
        snprintf(buf, sizeof buf, "U: di lai (da dung %d lan)", undoCount);
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
    printf("%c", markChar(t));
    setColor(7, 0);
    placeCursor();
}

static void drawBanner(int res, int color) {
    char buf[100];
    if (aiLevel != AI_NONE && res != RES_DRAW) {       /* dau voi may */
        int aiWon = res == (AI_MARK == 1 ? RES_X_WIN : RES_O_WIN);
        if (aiWon)
            snprintf(buf, sizeof buf, "*** %s DA THANG! Thu lai nhe! ***", aiName(aiLevel));
        else
            snprintf(buf, sizeof buf, "*** CHUC MUNG %s DA THANG %s! ***",
                     session.name[playerIndex(-AI_MARK)], aiName(aiLevel));
        showMessage(buf, color);
        return;
    }
    switch (res) {
        case RES_X_WIN:
            snprintf(buf, sizeof buf, "*** CHUC MUNG %s (X) DA CHIEN THANG! ***", session.name[0]);
            break;
        case RES_O_WIN:
            snprintf(buf, sizeof buf, "*** CHUC MUNG %s (O) DA CHIEN THANG! ***", session.name[1]);
            break;
        default:
            snprintf(buf, sizeof buf, "*** Hai ben hoa nhau! ***");
    }
    showMessage(buf, color);
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
        for (int k = 1; k <= 17; ++k)
            printf(" ");
    }
    setColor(retryText, retryBg);
    goToXY(chessBoard[5][4].X + 2, chessBoard[5][4].Y);
    printf("Choi lai!");
    setColor(exitText, exitBg);
    goToXY(chessBoard[6][4].X + 2, chessBoard[6][4].Y);
    printf("Ve menu!");
}

void endMenuBorder(int row) {
    for (int i = row; i <= row + 1; ++i) {
        goToXY(LEFT + 4 * 4, TOP + 2 * i);
        for (int j = 1; j <= 17; ++j) {
            printf(" ");
        }
    }
    goToXY(LEFT + 4 * 4, TOP + 2 * row + 1);
    printf(" ");
    goToXY(LEFT + 4 * 8, TOP + 2 * row + 1);
    printf(" ");
}

/* In ky tu cua o dang chon (tai vi tri con tro hien tai) tren nen sang.
 * Mau quan giong mau tren ban co (truoc day O bi in mau xanh la, khong khop). */
void currPosHighlight(int mark) {
    if (mark == 0) {
        setColor(0, COLOR_CURSOR_BG);
        printf(" ");
    } else {
        setColor(markColor(mark), COLOR_CURSOR_BG);
        printf("%c", markChar(mark));
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
    printPad((SCREEN_W - (int)strlen(text)) / 2, y, text, (int)strlen(text));
    setColor(7, 0);
}

void drawMenu(const char *title, const char *const items[], int n, int sel) {
    int w = 32, x = (SCREEN_W - w) / 2;     /* du rong cho "Choi voi may - Trung binh" */
    setColor(COLOR_TITLE, 0);
    printPad((SCREEN_W - (int)strlen(title)) / 2, 5, title, (int)strlen(title));
    for (int i = 0; i < n; i++) {
        char buf[64];
        snprintf(buf, sizeof buf, "  %-*s", w - 2, items[i]);
        setColor(i == sel ? 0 : 7, i == sel ? COLOR_HILITE_BG : 0);
        printPad(x, 9 + 2 * i, buf, w);
    }
    drawHelpLine(SCREEN_H - 2, "W/S hoac mui ten: chon    Enter: dong y    Esc: quay lai");
}

void drawSettings(int sel) {
    char line[SETTINGS_ITEMS][72];
    char bar[11];
    int filled = settings.volume / 10;
    for (int k = 0; k < 10; k++) bar[k] = k < filled ? '#' : '-';
    bar[10] = '\0';
    snprintf(line[0], sizeof line[0], "Am thanh         : %s", settings.soundOn ? "BAT" : "TAT");
    snprintf(line[1], sizeof line[1], "Am luong         : [%s] %3d%%", bar, settings.volume);
    snprintf(line[2], sizeof line[2], "Ten nguoi choi 1 : %s", settings.name[0]);
    snprintf(line[3], sizeof line[3], "Ten nguoi choi 2 : %s", settings.name[1]);
    snprintf(line[4], sizeof line[4], "Nghe thu am thanh");
    snprintf(line[5], sizeof line[5], "Quay lai");

    int w = 48, x = (SCREEN_W - w) / 2;
    setColor(COLOR_TITLE, 0);
    printPad((SCREEN_W - 8) / 2, 5, "SETTINGS", 8);
    for (int i = 0; i < SETTINGS_ITEMS; i++) {
        char buf[80];
        snprintf(buf, sizeof buf, "  %s", line[i]);
        setColor(i == sel ? 0 : 7, i == sel ? COLOR_HILITE_BG : 0);
        printPad(x, 9 + 2 * i, buf, w);
    }
    drawHelpLine(SCREEN_H - 3, "W/S: chon    A/D hoac mui ten trai/phai: chinh    Enter: dong y/sua ten");
    drawHelpLine(SCREEN_H - 2, "Esc: quay lai (ten moi ap dung tu van moi)");
}

void drawHistory(int offset) {
    char buf[120];
    int xw = 0, ow = 0, dw = 0;
    for (int i = 0; i < historyCount; i++) {
        if (history[i].result == RES_X_WIN) xw++;
        else if (history[i].result == RES_O_WIN) ow++;
        else dw++;
    }
    setColor(COLOR_TITLE, 0);
    printPad(2, 1, "LICH SU DAU", 40);
    setColor(7, 0);
    snprintf(buf, sizeof buf, "Tong: %d van    X thang: %d    O thang: %d    Hoa: %d", historyCount, xw, ow, dw);
    printPad(2, 2, buf, SCREEN_W - 4);
    setColor(11, 0);
    snprintf(buf, sizeof buf, "%3s  %-16s  %-12s    %-12s  %-8s %4s  %s", "#", "Thoi diem", "Nguoi X", "Nguoi O",
             "Ket qua", "Nuoc", "Gio");
    printPad(2, 4, buf, SCREEN_W - 4);
    setColor(7, 0);
    for (int k = 0; k < HISTORY_ROWS; k++) {
        int idx = historyCount - 1 - offset - k;           /* moi nhat truoc */
        if (idx < 0) { printPad(2, 5 + k, "", SCREEN_W - 4); continue; }
        const HistoryEntry *e = &history[idx];
        const char *res = e->result == RES_X_WIN ? "X thang" : (e->result == RES_O_WIN ? "O thang" : "Hoa");
        snprintf(buf, sizeof buf, "%3d  %-16s  %-12.12s vs %-12.12s  %-8s %4d  %02d:%02d", idx + 1, e->when,
                 e->name[0], e->name[1], res, e->moves, e->seconds / 60, e->seconds % 60);
        printPad(2, 5 + k, buf, SCREEN_W - 4);
    }
    if (historyCount == 0) {
        setColor(8, 0);
        printPad(2, 6, "(Chua co van nao duoc ghi lai)", 40);
    }
    drawHelpLine(SCREEN_H - 3, "W/S hoac mui ten: cuon    PgUp/PgDn: nhay trang    C: xoa lich su");
    drawHelpLine(SCREEN_H - 2, "Esc: quay lai");
    setColor(7, 0);
}

/* O nhap lieu: dung het chieu rong con lai cua man hinh, de nhan + ten dai
 * khong bi cat va con tro khong chay ra ngoai phan da in. */
void drawInputField(int x, int y, const char *label, const char *text) {
    char buf[160];
    int width = SCREEN_W - x - 1;
    snprintf(buf, sizeof buf, "%s%s", label, text);
    setColor(7, 0);
    printPad(x, y, buf, width);
    int len = (int)strlen(buf);
    goToXY(x + (len < width ? len : width - 1), y);
}

void clearInputField(int x, int y) {
    clearRegion(x, y, SCREEN_W - x - 1, 1);
}

/* Danh sach file da luu: 1 dong tieu de + SAVE_LIST_ROWS dong ten + 1 dong "con N file". */
void drawSaveList(int x, int y, FileName *names, int n) {
    char buf[64];
    setColor(11, 0);
    printPad(x, y, n > 0 ? "File da luu:" : "(Chua co file luu nao)", SAVE_LIST_W);
    setColor(7, 0);
    for (int k = 0; k < SAVE_LIST_ROWS; k++) {
        snprintf(buf, sizeof buf, "  %s", k < n ? names[k] : "");
        printPad(x, y + 1 + k, buf, SAVE_LIST_W);
    }
    setColor(8, 0);
    if (n > SAVE_LIST_ROWS) snprintf(buf, sizeof buf, "  ... va %d file khac", n - SAVE_LIST_ROWS);
    else buf[0] = '\0';
    printPad(x, y + 1 + SAVE_LIST_ROWS, buf, SAVE_LIST_W);
    setColor(7, 0);
}

void clearSaveList(int x, int y) {
    clearRegion(x, y, SAVE_LIST_W, SAVE_LIST_ROWS + 2);
}
