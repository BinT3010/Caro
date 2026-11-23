#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdlib.h>
#include "ai.h"

int aiLevel = AI_NONE;

const char *aiName(int level) {
    return level == AI_HARD ? "May (Kho)" : "May (De)";
}

const char *aiModeText(int level) {
    switch (level) {
        case AI_EASY: return "Che do: dau voi may - De";
        case AI_HARD: return "Che do: dau voi may - Kho";
        default:      return "Che do: 2 nguoi choi";
    }
}

#define N BOARD_SIZE
#define CAND_MAX (N * N)

static const int DR[4] = { 0, 1, 1, 1 };   /* ngang, doc, cheo \, cheo / */
static const int DC[4] = { 1, 0, 1, -1 };

static int b[N][N];                         /* ban sao ban co de thu nuoc */

static int inside(int r, int c) { return r >= 0 && r < N && c >= 0 && c < N; }

static void copyBoard(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            b[i][j] = chessBoard[i][j].mark;
}

/* Dat quan p vao o trong (r,c) co tao >= WIN_LEN quan lien tiep khong? */
static int makesFive(int r, int c, int p) {
    for (int d = 0; d < 4; d++) {
        int cnt = 1;
        for (int rr = r + DR[d], cc = c + DC[d]; inside(rr, cc) && b[rr][cc] == p; rr += DR[d], cc += DC[d]) cnt++;
        for (int rr = r - DR[d], cc = c - DC[d]; inside(rr, cc) && b[rr][cc] == p; rr -= DR[d], cc -= DC[d]) cnt++;
        if (cnt >= WIN_LEN) return 1;
    }
    return 0;
}

/* So o trong ma p danh vao la thang ngay ("so cho thang"). >= 2 la doi thu khong chan kip. */
static int countWinCells(int p, int stopAt) {
    int k = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (b[i][j] == 0 && makesFive(i, j, p) && ++k >= stopAt) return k;
    return k;
}

/* Tim 1 o ma p danh vao la thang ngay. */
static int findWinCell(int p, int *r, int *c) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (b[i][j] == 0 && makesFive(i, j, p)) { *r = i; *c = j; return 1; }
    return 0;
}

/* O trong nam gan quan da danh (cach toi da 2 o) -> chi xet nhung o nay cho nhanh. */
static int nearStone(int r, int c) {
    for (int i = r - 2; i <= r + 2; i++)
        for (int j = c - 2; j <= c + 2; j++)
            if (inside(i, j) && b[i][j] != 0) return 1;
    return 0;
}

static int listCandidates(int cr[], int cc[]) {
    int n = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (b[i][j] == 0 && nearStone(i, j)) { cr[n] = i; cc[n] = j; n++; }
    if (n == 0) {                                       /* ban trong: danh giua */
        int m = N / 2;
        for (int i = 0; i < N && n == 0; i++)
            for (int j = 0; j < N && n == 0; j++) {
                int r = (m + i) % N, c = (m + j) % N;
                if (b[r][c] == 0) { cr[n] = r; cc[n] = c; n++; }
            }
    }
    return n;
}

/* Diem cua 1 o cho quan p: xet moi "cua so" 5 o di qua (r,c) theo 4 huong.
 * Cua so khong co quan doi phuong -> cong diem theo so quan p (tinh ca o (r,c)).
 * Cach nay tu nhien uu tien day dai, day thoang 2 dau va ca day co lo hong (X X _ X X). */
static long lineScore(int r, int c, int p) {
    static const long T[WIN_LEN + 1] = { 0, 1, 6, 40, 400, 100000 };
    long s = 0;
    for (int d = 0; d < 4; d++) {
        for (int off = 0; off < WIN_LEN; off++) {
            int sr = r - off * DR[d], sc = c - off * DC[d];
            int er = sr + (WIN_LEN - 1) * DR[d], ec = sc + (WIN_LEN - 1) * DC[d];
            if (!inside(sr, sc) || !inside(er, ec)) continue;
            int cnt = 1, blocked = 0;
            for (int k = 0; k < WIN_LEN; k++) {
                int v = b[sr + k * DR[d]][sc + k * DC[d]];
                if (v == -p) { blocked = 1; break; }
                if (v == p) cnt++;
            }
            if (!blocked) s += T[cnt > WIN_LEN ? WIN_LEN : cnt];
        }
    }
    return s;
}

/* Sau khi `me` vua danh, doi thu `opp` co the tao nuoc doi (>= 2 cho thang) khong? */
static int opponentCanFork(int me, int opp) {
    int myWins = countWinCells(me, 2);
    if (myWins >= 2) return 0;                          /* minh thang truoc */
    if (myWins == 1) {                                  /* doi thu BUOC phai chan cho thang cua minh */
        int r, c;
        findWinCell(me, &r, &c);
        b[r][c] = opp;
        int fork = countWinCells(opp, 2) >= 2;
        b[r][c] = 0;
        return fork;
    }
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            if (b[i][j] != 0 || !nearStone(i, j)) continue;
            b[i][j] = opp;
            int fork = countWinCells(opp, 2) >= 2;
            b[i][j] = 0;
            if (fork) return 1;
        }
    return 0;
}

int aiChooseMove(int level, int me, int *row, int *col) {
    static int cr[CAND_MAX], cc[CAND_MAX];
    static long sc[CAND_MAX];
    int opp = -me;

    copyBoard();
    int n = listCandidates(cr, cc);
    if (n == 0) return 0;                               /* ban co day */

    /* 1. Thang ngay.  2. Chan nuoc thang ngay cua doi thu. */
    if (findWinCell(me, row, col)) return 1;
    if (findWinCell(opp, row, col)) return 1;

    if (level == AI_EASY) {
        /* Uu tien tan cong, phong thu yeu; chon ngau nhien trong 3 o tot nhat */
        for (int k = 0; k < n; k++) sc[k] = 10 * lineScore(cr[k], cc[k], me) + 5 * lineScore(cr[k], cc[k], opp);
        int top = n < 3 ? n : 3;
        for (int a = 0; a < top; a++) {                 /* dua 3 o diem cao nhat len dau */
            int best = a;
            for (int k = a + 1; k < n; k++) if (sc[k] > sc[best]) best = k;
            int tr = cr[a], tc = cc[a]; long ts = sc[a];
            cr[a] = cr[best]; cc[a] = cc[best]; sc[a] = sc[best];
            cr[best] = tr; cc[best] = tc; sc[best] = ts;
        }
        int pick = rand() % top;
        *row = cr[pick];
        *col = cc[pick];
        return 1;
    }

    /* Muc KHO: diem = tan cong + phong thu, cong/tru them theo phan tich "nuoc doi" */
    long bestScore = 0;
    int ties = 0;
    for (int k = 0; k < n; k++) {
        int r = cr[k], c = cc[k];
        long s = 10 * lineScore(r, c, me) + 9 * lineScore(r, c, opp);
        b[r][c] = me;
        if (countWinCells(me, 2) >= 2) s += 50000000L;      /* tao nuoc doi -> chac thang */
        else if (opponentCanFork(me, opp)) s -= 20000000L;  /* de doi thu tao nuoc doi -> nguy hiem */
        b[r][c] = 0;
        if (ties == 0 || s > bestScore) { bestScore = s; ties = 1; *row = r; *col = c; }
        else if (s == bestScore && rand() % ++ties == 0) { *row = r; *col = c; }
    }
    return 1;
}
