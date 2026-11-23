#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdlib.h>
#include "ai.h"

int aiLevel = AI_NONE;

const char *aiName(int level) {
    switch (level) {
        case AI_MEDIUM: return "May (Trung binh)";
        case AI_HARD:   return "May (Kho)";
        default:        return "May (De)";
    }
}

const char *aiModeText(int level) {
    switch (level) {
        case AI_EASY:   return "Che do: dau voi may - De";
        case AI_MEDIUM: return "Che do: dau voi may - Trung binh";
        case AI_HARD:   return "Che do: dau voi may - Kho";
        default:        return "Che do: 2 nguoi choi";
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

/* Diem nhanh cua 1 o cho ben p sap danh: tan cong + phong thu (dung de chon va sap xep nuoc). */
static long quickScore(int r, int c, int p) {
    return 10 * lineScore(r, c, p) + 9 * lineScore(r, c, -p);
}

/* Lay toi da k o tot nhat (theo quickScore) cho ben p, sap xep giam dan. */
static int topMoves(int p, int mr[], int mc[], int k) {
    static int cr[CAND_MAX], cc[CAND_MAX];
    long best[CAND_MAX];
    int n = listCandidates(cr, cc), m = 0;
    for (int t = 0; t < n; t++) {
        long s = quickScore(cr[t], cc[t], p);
        int pos = m < k ? m++ : k;                      /* chen vao danh sach top-k (sap xep chen) */
        if (pos == k && s <= best[k - 1]) continue;
        if (pos == k) pos = k - 1;
        while (pos > 0 && best[pos - 1] < s) {
            best[pos] = best[pos - 1]; mr[pos] = mr[pos - 1]; mc[pos] = mc[pos - 1];
            pos--;
        }
        best[pos] = s; mr[pos] = cr[t]; mc[pos] = cc[t];
    }
    return m;
}

/* ----- Muc KHO: tim kiem nhieu nuoc (negamax + cat tia alpha-beta) ----- */
#define SEARCH_DEPTH 4          /* so nuoc nhin truoc (ca 2 ben) */
#define SEARCH_WIDTH 10         /* moi nuoc chi xet 10 o tot nhat */
#define ROOT_WIDTH 14
#define WIN_SCORE 100000000L
#define INF_SCORE 1000000000L

/* Danh gia the co cho ben p (sap toi luot p): cong moi cua so 5 o chi co quan p,
 * tru moi cua so chi co quan doi thu. Cua so 4 quan cua p = p thang o nuoc toi. */
static long evalBoard(int p) {
    static const long MINE[WIN_LEN + 1] = { 0, 1, 12, 150, 2000000, 0 };
    static const long THEIRS[WIN_LEN + 1] = { 0, 1, 10, 120, 3000, 0 };
    long s = 0;
    for (int d = 0; d < 4; d++)
        for (int r = 0; r < N; r++)
            for (int c = 0; c < N; c++) {
                int er = r + (WIN_LEN - 1) * DR[d], ec = c + (WIN_LEN - 1) * DC[d];
                if (!inside(er, ec)) continue;
                int mine = 0, theirs = 0;
                for (int k = 0; k < WIN_LEN; k++) {
                    int v = b[r + k * DR[d]][c + k * DC[d]];
                    if (v == p) mine++;
                    else if (v == -p) theirs++;
                }
                if (mine && !theirs) s += MINE[mine];
                else if (theirs && !mine) s -= THEIRS[theirs];
            }
    return s;
}

/* Gia tri the co voi ben p sap danh (cang lon cang tot cho p). */
static long negamax(int p, int depth, long alpha, long beta) {
    int r, c;
    if (findWinCell(p, &r, &c)) return WIN_SCORE + depth;      /* thang ngay (cang som cang tot) */
    if (depth == 0) return evalBoard(p);

    int mr[SEARCH_WIDTH], mc[SEARCH_WIDTH], n;
    if (findWinCell(-p, &r, &c)) { mr[0] = r; mc[0] = c; n = 1; }   /* bat buoc chan */
    else n = topMoves(p, mr, mc, SEARCH_WIDTH);
    if (n == 0) return 0;                                       /* het cho: hoa */

    long best = -INF_SCORE;
    for (int k = 0; k < n; k++) {
        b[mr[k]][mc[k]] = p;
        long v = -negamax(-p, depth - 1, -beta, -alpha);
        b[mr[k]][mc[k]] = 0;
        if (v > best) best = v;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

static void searchMove(int me, int *row, int *col) {
    int mr[ROOT_WIDTH], mc[ROOT_WIDTH];
    int n = topMoves(me, mr, mc, ROOT_WIDTH);
    long best = -INF_SCORE;
    *row = mr[0];
    *col = mc[0];
    for (int k = 0; k < n; k++) {
        b[mr[k]][mc[k]] = me;
        long v = -negamax(-me, SEARCH_DEPTH - 1, -INF_SCORE, -best);
        b[mr[k]][mc[k]] = 0;
        if (v > best) { best = v; *row = mr[k]; *col = mc[k]; }
    }
}

/* VCF ("thang bang chuoi nuoc 4"): p danh lien tiep cac nuoc tao 4 (doi thu buoc phai chan
 * dung 1 o), cho toi khi tao duoc >= 2 cho thang. Tra ve 1 neu tim thay, nuoc dau ghi vao *r,*c.
 * Neu nuoc chan cua doi thu lai tao cho doi thu 1 cho thang (doi thu phan cong) -> bo nhanh do. */
#define VCF_DEPTH 5                                     /* toi da 5 nuoc 4 cua minh */
static int vcf(int p, int depth, int *r, int *c) {
    if (depth == 0) return 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            if (b[i][j] != 0 || !nearStone(i, j)) continue;
            b[i][j] = p;
            int wins = countWinCells(p, 2);
            int found = 0;
            if (wins >= 2) found = 1;                   /* 4 thoang / nuoc doi -> thang */
            else if (wins == 1) {
                int br, bc;
                findWinCell(p, &br, &bc);               /* doi thu buoc phai chan o nay */
                b[br][bc] = -p;
                if (countWinCells(-p, 1) == 0) {        /* doi thu khong phan cong duoc */
                    int nr, nc;
                    found = vcf(p, depth - 1, &nr, &nc);
                }
                b[br][bc] = 0;
            }
            b[i][j] = 0;
            if (found) { *r = i; *c = j; return 1; }
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

    /* Muc KHO: tim chuoi nuoc 4 dan toi thang; khong co thi tim kiem nhieu nuoc */
    if (level == AI_HARD) {
        if (!vcf(me, VCF_DEPTH, row, col)) searchMove(me, row, col);
        return 1;
    }

    /* TRUNG BINH: o diem cao nhat (tan cong + phong thu), chi nhin 1 nuoc */
    long bestScore = 0;
    int ties = 0;
    for (int k = 0; k < n; k++) {
        int r = cr[k], c = cc[k];
        long s = quickScore(r, c, me);
        if (ties == 0 || s > bestScore) { bestScore = s; ties = 1; *row = r; *col = c; }
        else if (s == bestScore && rand() % ++ties == 0) { *row = r; *col = c; }
    }
    return 1;
}
