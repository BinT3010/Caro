/* Kiem thu phan logic (model + storage + sound), KHONG can man hinh console.
 * Bien dich va chay tu thu muc du an:
 *   gcc -std=c99 -Wall -Wextra -I. tests/test_logic.c model.c storage.c sound.c -o test_logic.exe -lwinmm
 *   test_logic.exe
 */
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include "model.h"
#include "storage.h"
#include "sound.h"

static int failed = 0, passed = 0;
#define CHECK(cond) do { if (cond) passed++; else { failed++; \
    printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* Danh quan t tai (r, c) giong nhu nguoi choi bam Enter. */
static void play(int r, int c, int t) {
    currX = r; currY = c; turn = t;
    checkLegalEnter(t);
}

static void testWinLines(void) {
    printf("checkWin / testBoard\n");
    /* ngang sat mep phai */
    resetData();
    for (int c = 7; c < 12; c++) play(11, c, 1);
    CHECK(checkWin(1) == 1 && winCount == 5);
    CHECK(testBoard() == RES_X_WIN);

    /* doc sat mep tren, quan O */
    resetData();
    for (int r = 0; r < 5; r++) play(r, 0, -1);
    CHECK(checkWin(-1) == -1);
    CHECK(testBoard() == RES_O_WIN);

    /* cheo \ ket thuc o goc duoi phai */
    resetData();
    for (int k = 7; k < 12; k++) play(k, k, 1);
    CHECK(checkWin(1) == 1);

    /* cheo / tu goc tren phai */
    resetData();
    for (int k = 0; k < 5; k++) play(k, 11 - k, 1);
    CHECK(checkWin(1) == 1);
    CHECK(winX[0] == 0 && winY[0] == 11 && winX[4] == 4 && winY[4] == 7);

    /* nuoc cuoi danh vao giua day */
    resetData();
    play(5, 1, 1); play(5, 2, 1); play(5, 4, 1); play(5, 5, 1);
    CHECK(checkWin(1) == 0);
    play(5, 3, 1);
    CHECK(checkWin(1) == 1 && winCount == 5);

    /* 4 quan khong thang; 6 quan van thang */
    resetData();
    for (int c = 0; c < 4; c++) play(0, c, 1);
    CHECK(checkWin(1) == 0 && winCount == 0);
    resetData();
    for (int c = 0; c < 6; c++) play(3, c, 1);
    CHECK(checkWin(1) == 1 && winCount == 6);

    /* bi xen quan khac */
    resetData();
    play(2, 0, 1); play(2, 1, 1); play(2, 2, -1); play(2, 3, 1); play(2, 4, 1); play(2, 5, 1);
    CHECK(checkWin(1) == 0);

    /* o da danh thi khong danh lai duoc */
    resetData();
    currX = 4; currY = 4;
    CHECK(checkLegalEnter(1) == 1);
    CHECK(checkLegalEnter(-1) == 0);
    CHECK(chessBoard[4][4].mark == 1 && totalMoves == 1);

    /* hoa: ban co day khong co 5 lien tiep */
    resetData();
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            chessBoard[i][j].mark = ((j / 2 + i) % 2) ? 1 : -1;
    totalMoves = BOARD_SIZE * BOARD_SIZE;
    lastX = lastY = BOARD_SIZE - 1;
    turn = chessBoard[lastX][lastY].mark;
    CHECK(testBoard() == RES_DRAW);
}

static void testLoadState(void) {
    printf("loadState\n");
    int m[BOARD_SIZE][BOARD_SIZE];
    Session s;
    memset(&s, 0, sizeof s);
    strcpy(s.name[0], "A");
    strcpy(s.name[1], "B");
    s.wins[0] = 2; s.wins[1] = 1; s.draws = 1;

    memset(m, 0, sizeof m);
    m[0][0] = 1; m[0][1] = -1; m[1][1] = 1;
    CHECK(loadState(m, 99, -5, &s, 42) == 0);
    CHECK(turn == -1 && totalMoves == 3 && moveCount[0] == 2 && moveCount[1] == 1);
    CHECK(currX == BOARD_SIZE - 1 && currY == 0);        /* con tro bi kep vao ban co */
    CHECK(session.games == 4 && clockElapsed() == 42);

    memset(m, 0, sizeof m);
    m[0][0] = 2;
    CHECK(loadState(m, 0, 0, &s, 0) == 1);               /* gia tri la */

    memset(m, 0, sizeof m);
    m[0][0] = -1;
    CHECK(loadState(m, 0, 0, &s, 0) == 2);               /* O nhieu hon X */

    memset(m, 0, sizeof m);
    for (int c = 0; c < 5; c++) { m[0][c] = 1; m[5][c] = -1; }
    CHECK(loadState(m, 0, 0, &s, 0) == 4);               /* van da co nguoi thang */
}

static void testFileNames(void) {
    printf("normalizeFileName / sanitizeName\n");
    char n[FILE_NAME_LEN];
    strcpy(n, "van1");             CHECK(normalizeFileName(n, sizeof n) && strcmp(n, "van1.caro") == 0);
    strcpy(n, "van1.caro");        CHECK(normalizeFileName(n, sizeof n) && strcmp(n, "van1.caro") == 0);
    strcpy(n, "van1.CARO");        CHECK(normalizeFileName(n, sizeof n) && strcmp(n, "van1.CARO") == 0);
    strcpy(n, "caro_settings.cfg");CHECK(normalizeFileName(n, sizeof n) && strcmp(n, "caro_settings.cfg.caro") == 0);
    strcpy(n, "abc...");           CHECK(normalizeFileName(n, sizeof n) && strcmp(n, "abc.caro") == 0);
    strcpy(n, ".caro");            CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, "...");              CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, ".hidden");          CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, "con");              CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, "NUL.caro");         CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, "com1");             CHECK(!normalizeFileName(n, sizeof n));
    strcpy(n, "console");          CHECK(normalizeFileName(n, sizeof n));
    memset(n, 'a', FILE_BASE_MAX); n[FILE_BASE_MAX] = '\0';
    CHECK(normalizeFileName(n, sizeof n) && strlen(n) == FILE_BASE_MAX + 5);

    char d[NAME_LEN];
    sanitizeName(d, "  Lan|Anh  ");  CHECK(strcmp(d, "LanAnh") == 0);
    sanitizeName(d, "   ");          CHECK(strcmp(d, "Nguoi choi") == 0);
    sanitizeName(d, "123456789012345678901234567890");
    CHECK(strlen(d) == NAME_LEN - 1);
}

static void testSaveLoad(void) {
    printf("saveGame / loadGame\n");
    const char *file = "__test_tmp.caro";
    resetSession();
    strcpy(session.name[0], "Nguoi X co dau cach");
    strcpy(session.name[1], "O");
    session.wins[0] = 3; session.draws = 2;
    resetData();
    play(3, 3, 1); play(3, 4, -1); play(4, 4, 1);
    turn = -1;
    currX = 7; currY = 8;
    clockSet(125);
    CHECK(saveGame(file) == SAVE_OK);

    resetSession();
    resetData();
    CHECK(loadGame(file) == SAVE_OK);
    CHECK(strcmp(session.name[0], "Nguoi X co dau cach") == 0 && strcmp(session.name[1], "O") == 0);
    CHECK(session.wins[0] == 3 && session.draws == 2 && session.games == 5);
    CHECK(chessBoard[3][4].mark == -1 && chessBoard[4][4].mark == 1 && totalMoves == 3);
    CHECK(turn == -1 && currX == 7 && currY == 8 && clockElapsed() == 125);

    /* file bi sua hong */
    FILE *f = fopen(file, "w");
    fprintf(f, "CARO-SAVE 1\nsize 12\np1 A\n");
    fclose(f);
    CHECK(loadGame(file) == SAVE_ERR_FORMAT);
    remove(file);
    CHECK(loadGame(file) == SAVE_ERR_IO);
}

static void testSound(void) {
    printf("soundRender\n");
    static unsigned char wav[SOUND_WAV_MAX];
    for (int id = 0; id < SND_COUNT; id++) {
        int n = soundRender((SoundId)id, 60, wav, (int)sizeof wav);
        CHECK(n > 44 && n <= SOUND_WAV_MAX && memcmp(wav, "RIFF", 4) == 0 && memcmp(wav + 8, "WAVE", 4) == 0);
    }
    int n = soundRender(SND_WIN, 0, wav, (int)sizeof wav);
    int silent = 1;
    for (int i = 44; i < n; i++) if (wav[i] != 0) { silent = 0; break; }
    CHECK(n > 44 && silent);                             /* am luong 0 -> im lang */
    CHECK(soundRender(SND_COUNT, 50, wav, (int)sizeof wav) == 0);
}

int main(void) {
    testWinLines();
    testLoadState();
    testFileNames();
    testSaveLoad();
    testSound();
    printf("\n%d dung, %d sai\n", passed, failed);
    return failed ? 1 : 0;
}
