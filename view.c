#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "view.h"

/* Mau: 0 den, 1 xanh duong, 2 xanh la, 3 xanh ngoc, 4 do, 5 tim, 6 vang, 7 trang,
 *      8 xam, 9..15 la ban sang cua 1..7 */
#define COLOR_X 4
#define COLOR_O 1
#define COLOR_TITLE 14
#define COLOR_HILITE_BG 11

static int markColor(int mark) { return mark == 1 ? COLOR_X : COLOR_O; }
static char markChar(int mark) { return mark == 1 ? 'X' : 'O'; }

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

void placeCursor(void) {
    goToXY(chessBoard[currX][currY].X, chessBoard[currX][currY].Y);
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
    printPad(PANEL_X, TOP + 14, buf, 30);
    placeCursor();
}

void drawHud(void) {
    char buf[64];
    int x = PANEL_X;

    setColor(COLOR_TITLE, 0);
    printPad(x, TOP, "======== CARO ========", 30);

    int p = playerIndex(turn);
    setColor(markColor(turn), 0);
    snprintf(buf, sizeof buf, "Luot choi: %c (%s)", markChar(turn), session.name[p]);
    printPad(x, TOP + 2, buf, 30);

    for (int k = 0; k < 2; k++) {
        int mark = k == 0 ? 1 : -1;
        int y = TOP + 4 + k * 4;
        setColor(markColor(mark), 0);
        snprintf(buf, sizeof buf, "%c [%c] %s", turn == mark ? '>' : ' ', markChar(mark), session.name[k]);
        printPad(x, y, buf, 30);
        setColor(7, 0);
        snprintf(buf, sizeof buf, "    Nuoc di: %d", moveCount[k]);
        printPad(x, y + 1, buf, 30);
        snprintf(buf, sizeof buf, "    Thang: %d   Thua: %d", session.wins[k],
                 session.games - session.wins[k] - session.draws);
        printPad(x, y + 2, buf, 30);
    }

    setColor(7, 0);
    snprintf(buf, sizeof buf, "Hoa: %d    Tong so van: %d", session.draws, session.games);
    printPad(x, TOP + 12, buf, 30);
    drawTime();

    setColor(8, 0);
    printPad(x, TOP + 16, "W/A/S/D hoac phim mui ten: di chuyen", 34);
    printPad(x, TOP + 17, "Enter: danh co", 34);
    printPad(x, TOP + 18, "L: luu van    T: tai van", 34);
    printPad(x, TOP + 19, "Esc: thoat ve menu", 34);
    setColor(7, 0);
    placeCursor();
}

void drawGameScreen(void) {
    showCursor(1);
    clearScreen();
    drawBoard();
    drawMarks();
    drawHud();
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
    setColor(markColor(t), 0);
    printf("%c", markChar(t));
    setColor(7, 0);
    placeCursor();
}

static void drawBanner(int res, int color) {
    char buf[100];
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
    placeCursor();
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

void currPosHighlight(int mark) {
    if (mark == 0) {
        setColor(0, 7);
        printf(" ");
    } else {
        switch (mark) {
            case 1:
                setColor(4, 7);
                printf("X");
                break;
            default:
                setColor(2, 7);
                printf("O");
        }
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
    int w = 26, x = (SCREEN_W - w) / 2;
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

void drawInputField(int x, int y, const char *label, const char *text) {
    char buf[100];
    snprintf(buf, sizeof buf, "%s%s", label, text);
    setColor(7, 0);
    printPad(x, y, buf, 60);
    goToXY(x + (int)strlen(buf), y);
}

void drawSaveList(int x, int y, FileName *names, int n) {
    setColor(11, 0);
    printPad(x, y, n > 0 ? "File da luu:" : "(Chua co file luu nao)", 30);
    setColor(7, 0);
    for (int k = 0; k < 6; k++) {
        char buf[64];
        snprintf(buf, sizeof buf, "  %s", k < n ? names[k] : "");
        printPad(x, y + 1 + k, buf, 30);
    }
    if (n > 6) { setColor(8, 0); printPad(x, y + 7, "  ...", 30); setColor(7, 0); }
}
