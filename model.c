#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS /* Visual Studio: bo canh bao fopen/sscanf */
#endif
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "model.h"

point chessBoard[BOARD_SIZE][BOARD_SIZE];
int currX, currY, turn;
int moveCount[2];
int totalMoves;
int lastX = -1, lastY = -1;
int winCount;
int winX[BOARD_SIZE], winY[BOARD_SIZE];
Session session;
int undoCount;

/* Lich su nuoc di cua van hien tai (de di lai). Xoa khi bat dau van moi hoac tai van. */
static struct { int r, c, mark; } hist[BOARD_SIZE * BOARD_SIZE];
static int histLen;

Settings settings = { 1, 60, { "Người chơi 1", "Người chơi 2" }, 0 /* LANG_VI */ };

/* ---------------- dong ho ---------------- */
static time_t clockBegan;
static int clockAccum, clockRunning;

void clockReset(void) { clockAccum = 0; clockRunning = 0; }
void clockStart(void) {
    if (!clockRunning) { clockBegan = time(NULL); clockRunning = 1; }
}
void clockStop(void) {
    if (clockRunning) {
        clockAccum += (int)difftime(time(NULL), clockBegan);
        clockRunning = 0;
    }
}
int clockElapsed(void) {
    return clockAccum + (clockRunning ? (int)difftime(time(NULL), clockBegan) : 0);
}
void clockSet(int seconds) {
    clockAccum = seconds < 0 ? 0 : seconds;
    clockRunning = 0;
}

/* ---------------- dong ho tung nguoi (gioi han thoi gian) ---------------- */
int timeLimitSec = TIME_DEFAULT;
static long leftMs[2];              /* thoi gian con lai: [0] = X, [1] = O */
static int tcRunning, tcIdx;        /* dang dem cho nguoi nao */
static long tcStart;

void turnClockReset(int limitSeconds) {
    timeLimitSec = limitSeconds;
    leftMs[0] = leftMs[1] = (long)limitSeconds * 1000L;
    tcRunning = 0;
}

void turnClockSet(int limitSeconds, long leftX, long leftO) {
    turnClockReset(limitSeconds);
    leftMs[0] = leftX;
    leftMs[1] = leftO;
}

void turnClockStart(long nowMs) {
    if (tcRunning) return;
    tcIdx = playerIndex(turn);
    tcStart = nowMs;
    tcRunning = 1;
}

void turnClockStop(long nowMs) {
    if (!tcRunning) return;
    long used = nowMs - tcStart;
    if (used > 0) leftMs[tcIdx] -= used;
    if (leftMs[tcIdx] < 0) leftMs[tcIdx] = 0;
    tcRunning = 0;
}

long turnClockLeftMs(int idx, long nowMs) {
    long left = leftMs[idx];
    if (tcRunning && idx == tcIdx && nowMs > tcStart) left -= nowMs - tcStart;
    return left < 0 ? 0 : left;
}

int turnClockShownSeconds(int idx, long nowMs) {
    return (int)((turnClockLeftMs(idx, nowMs) + 999) / 1000);    /* lam tron len: 0 = het gio */
}

int turnClockExpired(long nowMs) {
    return turnClockLeftMs(playerIndex(turn), nowMs) <= 0;
}

/* ---------------- ban co ---------------- */
int playerIndex(int mark) { return mark == 1 ? 0 : 1; }

void resetData(void) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            chessBoard[i][j].X = 4 * j + LEFT + 2; /* trung hoanh do man hinh */
            chessBoard[i][j].Y = 2 * i + TOP + 1;  /* trung tung do man hinh */
            chessBoard[i][j].mark = 0;             /* 0 = chua ai danh */
        }
    }
    turn = 1;                      /* X di truoc */
    currX = currY = 0;
    moveCount[0] = moveCount[1] = 0;
    totalMoves = 0;
    lastX = lastY = -1;
    winCount = 0;
    histLen = 0;
    undoCount = 0;
    clockReset();
    turnClockReset(timeLimitSec);  /* van moi: ca 2 ben day gio */
}

void resetSession(void) {
    memset(&session, 0, sizeof session);
    snprintf(session.name[0], NAME_LEN, "%s", settings.name[0]);
    snprintf(session.name[1], NAME_LEN, "%s", settings.name[1]);
}

void clampCursor(void) {
    if (currX < 0) currX = 0;
    if (currX > BOARD_SIZE - 1) currX = BOARD_SIZE - 1;
    if (currY < 0) currY = 0;
    if (currY > BOARD_SIZE - 1) currY = BOARD_SIZE - 1;
}

int checkLegalEnter(int t) {
    if (chessBoard[currX][currY].mark != 0) return 0;
    chessBoard[currX][currY].mark = t;
    moveCount[playerIndex(t)]++;
    totalMoves++;
    lastX = currX;
    lastY = currY;
    if (histLen < BOARD_SIZE * BOARD_SIZE) {
        hist[histLen].r = currX;
        hist[histLen].c = currY;
        hist[histLen].mark = t;
        histLen++;
    }
    return 1;
}

/* ---------------- di lai ---------------- */
int moveHistoryLength(void) { return histLen; }

int lastMoveMark(void) { return histLen > 0 ? hist[histLen - 1].mark : 0; }

int undoMove(int *r, int *c) {
    if (histLen == 0) return 0;
    histLen--;
    int rr = hist[histLen].r, cc = hist[histLen].c, mark = hist[histLen].mark;
    chessBoard[rr][cc].mark = 0;
    moveCount[playerIndex(mark)]--;
    totalMoves--;
    turn = mark;                            /* nguoi vua bi xoa nuoc se danh lai */
    winCount = 0;
    if (histLen > 0) { lastX = hist[histLen - 1].r; lastY = hist[histLen - 1].c; }
    else lastX = lastY = -1;
    if (r) *r = rr;
    if (c) *c = cc;
    return 1;
}

/* ---------------- luat thang ---------------- */
static const int DR[4] = { 0, 1, 1, 1 };   /* ngang, doc, cheo \, cheo / */
static const int DC[4] = { 1, 0, 1, -1 };

static int inside(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

/* Do dai day quan cung loai di qua (r,c) theo huong d; tra ve diem dau day. */
static int runThrough(int m[BOARD_SIZE][BOARD_SIZE], int r, int c, int d,
                      int *startR, int *startC) {
    int mark = m[r][c];
    if (mark == 0) return 0;
    while (inside(r - DR[d], c - DC[d]) && m[r - DR[d]][c - DC[d]] == mark) {
        r -= DR[d];
        c -= DC[d];
    }
    *startR = r;
    *startC = c;
    int n = 0;
    while (inside(r, c) && m[r][c] == mark) { n++; r += DR[d]; c += DC[d]; }
    return n;
}

static void snapshot(int m[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            m[i][j] = chessBoard[i][j].mark;
}

int checkWin(int t) {
    int m[BOARD_SIZE][BOARD_SIZE];
    winCount = 0;
    if (!inside(lastX, lastY)) return 0;
    snapshot(m);
    if (m[lastX][lastY] != t) return 0;
    for (int d = 0; d < 4; d++) {
        int sr, sc;
        int n = runThrough(m, lastX, lastY, d, &sr, &sc);
        if (n >= WIN_LEN) {
            winCount = n;
            for (int k = 0; k < n; k++) {
                winX[k] = sr + k * DR[d];
                winY[k] = sc + k * DC[d];
            }
            return t;
        }
    }
    return 0;
}

int boardFull(void) { return totalMoves >= BOARD_SIZE * BOARD_SIZE; }

int testBoard(void) {
    if (checkWin(turn)) return turn == 1 ? RES_X_WIN : RES_O_WIN;
    if (boardFull()) return RES_DRAW;
    return RES_NONE;
}

void switchTurn(void) { turn = -turn; }

void recordResult(int res) {
    session.games++;
    if (res == RES_X_WIN) session.wins[0]++;
    else if (res == RES_O_WIN) session.wins[1]++;
    else if (res == RES_DRAW) session.draws++;
}

/* ---------------- nap van da luu ---------------- */
int loadState(int marks[BOARD_SIZE][BOARD_SIZE], int cursorRow, int cursorCol,
              const Session *s, int elapsedSeconds) {
    int cx = 0, co = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            int v = marks[i][j];
            if (v == 1) cx++;
            else if (v == -1) co++;
            else if (v != 0) return 1;
        }
    }
    /* X di truoc nen so X bang so O, hoac nhieu hon dung 1 */
    if (!(cx == co || cx == co + 1)) return 2;
    if (cx + co >= BOARD_SIZE * BOARD_SIZE) return 3;      /* ban co da day */
    for (int i = 0; i < BOARD_SIZE; i++) {                  /* van da co nguoi thang */
        for (int j = 0; j < BOARD_SIZE; j++) {
            for (int d = 0; d < 4; d++) {
                int sr, sc;
                if (runThrough(marks, i, j, d, &sr, &sc) >= WIN_LEN) return 4;
            }
        }
    }

    resetData();
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            chessBoard[i][j].mark = marks[i][j];
    moveCount[0] = cx;
    moveCount[1] = co;
    totalMoves = cx + co;
    turn = (cx == co) ? 1 : -1;
    currX = cursorRow;
    currY = cursorCol;
    clampCursor();

    session = *s;
    session.name[0][NAME_LEN - 1] = '\0';
    session.name[1][NAME_LEN - 1] = '\0';
    if (session.wins[0] < 0) session.wins[0] = 0;
    if (session.wins[1] < 0) session.wins[1] = 0;
    if (session.draws < 0) session.draws = 0;
    if (session.games < session.wins[0] + session.wins[1] + session.draws)
        session.games = session.wins[0] + session.wins[1] + session.draws;
    clockSet(elapsedSeconds);
    return 0;
}
