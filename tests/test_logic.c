/* Kiem thu phan logic (model + storage + sound), KHONG can man hinh console.
 * Bien dich va chay tu thu muc du an:
 *   gcc -std=c99 -Wall -Wextra -I. tests/test_logic.c model.c storage.c sound.c ai.c utf8.c lang.c -o test_logic.exe -lwinmm
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
#include "ai.h"
#include "utf8.h"
#include "lang.h"
#include "view.h"   /* chi lay hang so man hinh (SCREEN_W, HUD_W) */
#include <stdlib.h>

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
    sanitizeName(d, "   ");          CHECK(strcmp(d, "Người chơi") == 0);
    sanitizeName(d, "123456789012345678901234567890");
    CHECK(strlen(d) == NAME_CHARS && utf8Width(d) == NAME_CHARS);
    /* ten tieng Viet co dau: giu nguyen, cat theo KY TU (khong cat doi 1 chu) */
    sanitizeName(d, "  Nguyễn Thị Ánh  ");  CHECK(strcmp(d, "Nguyễn Thị Ánh") == 0);
    sanitizeName(d, "Đặng Thị Ngọc Ánh Dương Hồng");          /* 28 ky tu -> 20 */
    CHECK(utf8Width(d) == NAME_CHARS && strcmp(d, "Đặng Thị Ngọc Ánh Dư") == 0);
    sanitizeName(d, "ệệệệệệệệệệệệệệệệệệệệệệệệệ");            /* 25 chu 3 byte -> vua NAME_LEN */
    CHECK(utf8Width(d) == NAME_CHARS && strlen(d) == NAME_LEN - 1);
    sanitizeName(d, "A\xff\xc3" "B");                         /* byte sai UTF-8 bi bo */
    CHECK(strcmp(d, "AB") == 0);
    sanitizeName(d, "Le\xcc\x81");                            /* dau thanh roi (to hop) bi bo */
    CHECK(strcmp(d, "Le") == 0);
}

static void testUtf8(void) {
    printf("utf8 (tieng Viet co dau)\n");
    char b[64];
    CHECK(utf8Width("") == 0 && utf8Width("abc") == 3 && utf8Width("Việt Nam") == 8);
    CHECK(utf8Width("ĐẶT TÊN NGƯỜI CHƠI") == 18);
    utf8Fit(b, sizeof b, "Người chơi", 5, 0);  CHECK(strcmp(b, "Người") == 0);
    utf8Fit(b, sizeof b, "Hòa", 6, 1);         CHECK(strcmp(b, "Hòa   ") == 0 && utf8Width(b) == 6);
    utf8Fit(b, 4, "ệệ", 5, 0);                 CHECK(strcmp(b, "ệ") == 0);       /* het cho: khong cat doi chu */
    strcpy(b, "Lân");  utf8PopLast(b);         CHECK(strcmp(b, "Lâ") == 0);
    utf8PopLast(b); utf8PopLast(b); utf8PopLast(b); CHECK(b[0] == '\0');
    CHECK(utf8EqualNoCase("ÂN", "ân") && utf8EqualNoCase("ĐỨC", "đức") && utf8EqualNoCase("Ơ Ư", "ơ ư"));
    CHECK(!utf8EqualNoCase("An", "Ân") && !utf8EqualNoCase("An", "Anh"));
    char e[4]; unsigned cp;
    for (unsigned c = 1; c < 0x10000; c++) {                   /* ma hoa roi giai ma lai dung */
        if (c >= 0xD800 && c <= 0xDFFF) continue;
        int n = utf8Encode(c, e);
        char z[5]; memcpy(z, e, (size_t)n); z[n] = '\0';
        if (utf8Decode(z, &cp) != n || cp != c) { CHECK(0); break; }
    }
    CHECK(utf8Decode("\xc0\x80", &cp) == 1 && cp == UTF8_BAD);  /* ma hoa thua */
}

static void testPlayerNames(void) {
    printf("setPlayerName\n");
    resetSession();
    snprintf(session.name[0], NAME_LEN, "Người chơi 1");
    snprintf(session.name[1], NAME_LEN, "Người chơi 2");
    CHECK(setPlayerName(0, "  Lan  ") == NAME_OK && strcmp(session.name[0], "Lan") == 0);
    CHECK(setPlayerName(1, "Minh") == NAME_OK && strcmp(session.name[1], "Minh") == 0);
    CHECK(setPlayerName(0, "") == NAME_KEPT && strcmp(session.name[0], "Lan") == 0);     /* bo trong */
    CHECK(setPlayerName(0, "    ") == NAME_KEPT && strcmp(session.name[0], "Lan") == 0); /* chi khoang trang */
    CHECK(setPlayerName(0, NULL) == NAME_KEPT && strcmp(session.name[0], "Lan") == 0);
    CHECK(setPlayerName(1, "LAN") == NAME_DUPLICATE && strcmp(session.name[1], "Minh") == 0); /* trung, khac hoa thuong */
    CHECK(setPlayerName(1, " lan ") == NAME_DUPLICATE && strcmp(session.name[1], "Minh") == 0);
    CHECK(setPlayerName(0, "Lan") == NAME_OK);                                           /* tu doi lai ten minh thi duoc */
    CHECK(setPlayerName(0, "A|B") == NAME_OK && strcmp(session.name[0], "AB") == 0);     /* ky tu '|' bi bo */
    CHECK(setPlayerName(0, "123456789012345678901234567890") == NAME_OK
          && utf8Width(session.name[0]) == NAME_CHARS);                                  /* cat 20 ky tu */
    CHECK(setPlayerName(0, "Ân") == NAME_OK && setPlayerName(1, "ân") == NAME_DUPLICATE); /* trung, chu co dau */
    CHECK(setPlayerName(1, "ÂN") == NAME_DUPLICATE && setPlayerName(1, "An") == NAME_OK); /* "An" khac "Ân" */
    resetSession();                                                                      /* ten khong ro ri sang phien sau */
    CHECK(strcmp(session.name[0], settings.name[0]) == 0 && strcmp(session.name[1], settings.name[1]) == 0);
}

/* Lich su + cai dat giu nguyen ten tieng Viet (ca ten 20 chu, moi chu 3 byte). */
static void testHistorySettings(void) {
    printf("lich su / cai dat voi ten co dau\n");
    const char *longName = "ệệệệệệệệệệệệệệệệệệệệ";           /* 20 ky tu = 60 byte */
    resetSession();
    resetData();
    snprintf(session.name[0], NAME_LEN, "%s", longName);
    snprintf(session.name[1], NAME_LEN, "%s", aiName(AI_HARD));
    rename(HISTORY_FILE, "__hist_backup.txt");
    CHECK(historyAppend(RES_X_WIN) == SAVE_OK);
    CHECK(historyLoad() == 1 && strcmp(history[0].name[0], longName) == 0
          && strcmp(history[0].name[1], "Máy (Khó)") == 0);
    remove(HISTORY_FILE);
    rename("__hist_backup.txt", HISTORY_FILE);

    Settings keep = settings;
    rename(SETTINGS_FILE, "__cfg_backup.cfg");
    snprintf(settings.name[0], NAME_LEN, "Trần Thị Lan");
    snprintf(settings.name[1], NAME_LEN, "%s", longName);
    CHECK(settingsSave() == SAVE_OK);
    memset(settings.name, 0, sizeof settings.name);
    settingsLoad();
    CHECK(strcmp(settings.name[0], "Trần Thị Lan") == 0 && strcmp(settings.name[1], longName) == 0);
    remove(SETTINGS_FILE);
    rename("__cfg_backup.cfg", SETTINGS_FILE);
    settings = keep;
}

/* Lay day cac ma dinh dang (%d, %02d, %s, %%...) cua 1 cau, de so 2 ban dich. */
static void formatSpecs(const char *s, char *out, size_t cap) {
    size_t n = 0;
    for (; *s && n + 1 < cap; s++) {
        if (*s != '%') continue;
        s++;
        while (*s && strchr("0123456789.-+ #", *s)) out[n < cap - 1 ? n++ : n] = *s++;
        if (*s) out[n < cap - 1 ? n++ : n] = *s;
        out[n < cap - 1 ? n++ : n] = '|';
        if (!*s) break;
    }
    out[n] = '\0';
}

static void testLang(void) {
    printf("ngon ngu (Tieng Viet / English)\n");
    int missing = 0, badFormat = 0, tooWide = 0;
    for (int id = 0; id < S_COUNT; id++) {
        const char *vi = trIn(LANG_VI, (StrId)id), *en = trIn(LANG_EN, (StrId)id);
        if (vi[0] == '\0' || en[0] == '\0') { missing++; printf("    thieu ban dich: ma %d\n", id); }
        char fv[64], fe[64];
        formatSpecs(vi, fv, sizeof fv);
        formatSpecs(en, fe, sizeof fe);
        if (strcmp(fv, fe) != 0) { badFormat++; printf("    %%d/%%s khac nhau: ma %d (%s / %s)\n", id, fv, fe); }
        /* moi cau phai vua dong thong bao (SCREEN_W - 1 o) */
        if (utf8Width(vi) > SCREEN_W - 1 || utf8Width(en) > SCREEN_W - 1) { tooWide++; printf("    qua dai: ma %d\n", id); }
    }
    CHECK(missing == 0);
    CHECK(badFormat == 0);
    CHECK(tooWide == 0);
    /* cac dong trong bang thong so phai vua HUD_W o (khi da dien so / ten dai nhat) */
    static const StrId hud[] = { S_HUD_KEYS_MOVE, S_HUD_KEYS_ENTER, S_HUD_KEYS_SAVE, S_HUD_KEYS_ESC,
                                 S_AIMODE_EASY, S_AIMODE_MEDIUM, S_AIMODE_HARD, S_AIMODE_2P };
    int hudOk = 1;
    for (int l = 0; l < LANG_COUNT; l++) {
        for (size_t k = 0; k < sizeof hud / sizeof hud[0]; k++)
            if (utf8Width(trIn(l, hud[k])) > HUD_W) { hudOk = 0; printf("    HUD qua rong: ngon ngu %d ma %d\n", l, (int)hud[k]); }
        char b[160];
        snprintf(b, sizeof b, trIn(l, S_HUD_TOTAL_TIME), 99, 59);           if (utf8Width(b) > HUD_W) hudOk = 0;
        snprintf(b, sizeof b, trIn(l, S_HUD_TIME_LEFT), 15, 0, "  <");      if (utf8Width(b) > HUD_W) hudOk = 0;
        snprintf(b, sizeof b, trIn(l, S_HUD_LIMIT), 15);                    if (utf8Width(b) > HUD_W) hudOk = 0;
        snprintf(b, sizeof b, trIn(l, S_HUD_WINLOSS), 99, 99);              if (utf8Width(b) > HUD_W) hudOk = 0;
        snprintf(b, sizeof b, trIn(l, S_HUD_DRAWS), 99, 999);               if (utf8Width(b) > HUD_W) hudOk = 0;
        snprintf(b, sizeof b, trIn(l, S_HUD_UNDO), 999);                    if (utf8Width(b) > HUD_W) hudOk = 0;
        /* menu: muc dai nhat phai vua khung 32 o (2 o le trai) */
        if (utf8Width(trIn(l, S_MODE_AI_MEDIUM)) > 30 || utf8Width(trIn(l, S_TIME_15)) > 30) hudOk = 0;
        /* menu cuoi van: o rong 17, chu bat dau o cot thu 3 */
        if (utf8Width(trIn(l, S_END_REPLAY)) > 14 || utf8Width(trIn(l, S_END_MENU)) > 14) hudOk = 0;
    }
    CHECK(hudOk);

    CHECK(strcmp(langCode(LANG_VI), "vi") == 0 && strcmp(langCode(LANG_EN), "en") == 0);
    CHECK(langFromCode("en") == LANG_EN && langFromCode("vi") == LANG_VI);
    CHECK(langFromCode("fr") == LANG_VI && langFromCode("") == LANG_VI && langFromCode(NULL) == LANG_VI);

    langSet(LANG_EN);
    CHECK(langGet() == LANG_EN && strcmp(tr(S_MENU_NEW), "New Game") == 0);
    CHECK(strcmp(aiName(AI_HARD), "CPU (Hard)") == 0 && strcmp(aiModeText(AI_NONE), "Mode: 2 players") == 0);
    char d[NAME_LEN];
    sanitizeName(d, "   ");  CHECK(strcmp(d, "Player") == 0);                /* ten rong theo ngon ngu */
    langSet(99);             CHECK(langGet() == LANG_VI);                     /* gia tri la -> Tieng Viet */
    CHECK(strcmp(aiName(AI_HARD), "Máy (Khó)") == 0);

    /* ngon ngu duoc luu vao file cai dat va doc lai */
    Settings keep = settings;
    rename(SETTINGS_FILE, "__cfg_backup.cfg");
    settings.lang = LANG_EN;
    CHECK(settingsSave() == SAVE_OK);
    settings.lang = LANG_VI;
    settingsLoad();
    CHECK(settings.lang == LANG_EN);
    FILE *f = fopen(SETTINGS_FILE, "w");                 /* file cu (chua co dong lang) -> Tieng Viet */
    fputs("sound=1\nvolume=60\nname1=A\nname2=B\n", f);
    fclose(f);
    settings.lang = LANG_VI;
    settingsLoad();
    CHECK(settings.lang == LANG_VI);
    remove(SETTINGS_FILE);
    rename("__cfg_backup.cfg", SETTINGS_FILE);

    /* doi ngon ngu: ten mac dinh doi theo, ten tu dat giu nguyen */
    settings.lang = LANG_VI;
    snprintf(settings.name[0], NAME_LEN, "Người chơi 1");
    snprintf(settings.name[1], NAME_LEN, "Bình");
    settingsSetLanguage(LANG_EN);
    CHECK(langGet() == LANG_EN && settings.lang == LANG_EN);
    CHECK(strcmp(settings.name[0], "Player 1") == 0 && strcmp(settings.name[1], "Bình") == 0);
    snprintf(settings.name[1], NAME_LEN, "Player 2");
    settingsSetLanguage(LANG_VI);
    CHECK(strcmp(settings.name[0], "Người chơi 1") == 0 && strcmp(settings.name[1], "Người chơi 2") == 0);
    settingsSetLanguage(LANG_VI);                                         /* dat lai cung ngon ngu: khong doi gi */
    CHECK(strcmp(settings.name[0], "Người chơi 1") == 0 && langGet() == LANG_VI);
    settings = keep;
    langSet(LANG_VI);
}

static void testSaveLoad(void) {
    printf("saveGame / loadGame\n");
    const char *file = "__test_tmp.caro";
    resetSession();
    strcpy(session.name[0], "Nguyễn Văn Đức");
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
    CHECK(strcmp(session.name[0], "Nguyễn Văn Đức") == 0 && strcmp(session.name[1], "O") == 0);   /* ten co dau qua file */
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

/* Dat nhanh nhieu quan (khong can dung thu tu luot). */
static void put(int r, int c, int t) { chessBoard[r][c].mark = t; totalMoves++; }

static int aiPick(int level, int me, int *r, int *c) {
    *r = *c = -1;
    return aiChooseMove(level, me, r, c) && *r >= 0 && *r < BOARD_SIZE && *c >= 0 && *c < BOARD_SIZE
           && chessBoard[*r][*c].mark == 0;
}

/* Cho 2 AI dau voi nhau (4 nuoc dau ngau nhien gan giua ban de cac van khac nhau),
 * tra ve ket qua. -99 neu co nuoc di sai luat. */
static int aiVsAi(int levelX, int levelO) {
    resetData();
    for (int k = 0; k < 4; k++) {
        do { currX = 4 + rand() % 4; currY = 4 + rand() % 4; } while (chessBoard[currX][currY].mark != 0);
        checkLegalEnter(turn);
        switchTurn();
    }
    for (;;) {
        int r, c;
        if (!aiPick(turn == 1 ? levelX : levelO, turn, &r, &c)) return -99;
        currX = r; currY = c;
        if (!checkLegalEnter(turn)) return -99;
        int res = testBoard();
        if (res != RES_NONE) return res;
        switchTurn();
    }
}

static void testAI(void) {
    printf("AI (choi voi may)\n");
    int r, c;
    srand(12345);

    for (int level = AI_EASY; level <= AI_HARD; level++) {
        /* ban trong: van chon duoc o hop le */
        resetData();
        CHECK(aiPick(level, -1, &r, &c));

        /* may (O) co 4 quan -> phai danh thang ngay */
        resetData();
        for (int k = 3; k < 7; k++) put(3, k, -1);
        put(8, 1, 1); put(8, 3, 1); put(9, 5, 1); put(10, 8, 1); put(0, 0, 1);
        CHECK(aiPick(level, -1, &r, &c) && r == 3 && (c == 2 || c == 7));

        /* nguoi (X) co 4 quan, may khong co cho thang -> phai chan */
        resetData();
        for (int k = 2; k < 6; k++) put(6, k, 1);
        put(0, 0, -1); put(11, 11, -1); put(0, 11, -1);
        CHECK(aiPick(level, -1, &r, &c) && r == 6 && (c == 1 || c == 6));

        /* X co 4 quan bi chan 1 dau (O X X X X _) -> chan dung o con lai */
        resetData();
        put(2, 1, -1);
        for (int k = 2; k < 6; k++) put(2, k, 1);
        put(11, 0, -1); put(11, 11, -1);
        CHECK(aiPick(level, -1, &r, &c) && r == 2 && c == 6);
    }

    /* Kho: X co hang 3 thoang (_ X X X _) -> phai chan sat 2 dau, khong de thanh 4 thoang */
    for (int t = 0; t < 5; t++) {
        resetData();
        put(5, 4, 1); put(5, 5, 1); put(5, 6, 1);
        put(0, 0, -1); put(11, 11, -1);
        CHECK(aiPick(AI_HARD, -1, &r, &c) && r == 5 && (c == 3 || c == 7));
    }

    /* Kho: may co hang 3 thoang (_ O O O _) -> phai tao "nuoc doi" (4 thoang = 2 cho thang) */
    resetData();
    put(5, 4, -1); put(5, 5, -1); put(5, 6, -1);
    put(0, 0, 1); put(0, 11, 1); put(11, 0, 1); put(11, 11, 1);
    CHECK(aiPick(AI_HARD, -1, &r, &c));
    chessBoard[r][c].mark = -1;
    {
        int wins = 0;
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (chessBoard[i][j].mark != 0) continue;
                chessBoard[i][j].mark = -1;
                lastX = i; lastY = j;
                if (checkWin(-1)) wins++;
                chessBoard[i][j].mark = 0;
            }
        CHECK(wins >= 2);
    }

    /* Kho: thang bang chuoi nuoc 4 (VCF). O danh (5,4) -> 4 quan ngang (X buoc chan (5,5))
     * dong thoi cot 4 thanh 3 thoang -> nuoc sau tao 4 thoang. Dau voi Trung binh phai thang. */
    resetData();
    put(5, 0, 1); put(5, 1, -1); put(5, 2, -1); put(5, 3, -1);
    put(3, 4, -1); put(4, 4, -1);
    put(0, 11, 1); put(11, 11, 1); put(11, 0, 1); put(0, 0, 1); put(9, 9, 1);
    turn = -1;
    {
        int res = RES_NONE, steps = 0;
        while (res == RES_NONE && steps++ < 10) {
            if (!aiPick(turn == -1 ? AI_HARD : AI_MEDIUM, turn, &r, &c)) break;
            currX = r; currY = c;
            checkLegalEnter(turn);
            res = testBoard();
            if (res == RES_NONE) switchTurn();
        }
        CHECK(res == RES_O_WIN && steps <= 5);
    }

    /* Dau thu: moi nuoc deu hop le, van luon ket thuc; muc cao thang muc thap ap dao */
    struct { int strong, weak; const char *label; } match[] = {
        { AI_MEDIUM, AI_EASY, "Trung binh thang De" },
        { AI_HARD, AI_EASY, "Kho thang De" },
        { AI_HARD, AI_MEDIUM, "Kho thang Trung binh" },
    };
    for (int m = 0; m < 3; m++) {
        int wins = 0, losses = 0, games = 0, illegal = 0;
        for (int g = 0; g < 10; g++) {
            int res = aiVsAi(match[m].strong, match[m].weak);    /* muc cao cam X */
            if (res == -99) illegal++;
            if (res == RES_X_WIN) wins++;
            if (res == RES_O_WIN) losses++;
            res = aiVsAi(match[m].weak, match[m].strong);        /* muc cao cam O */
            if (res == -99) illegal++;
            if (res == RES_O_WIN) wins++;
            if (res == RES_X_WIN) losses++;
            games += 2;
        }
        printf("  %s: thang %d, thua %d, hoa %d / %d van\n", match[m].label, wins, losses,
               games - wins - losses, games);
        CHECK(illegal == 0);
        CHECK(wins * 2 >= games);                          /* thang it nhat 1 nua so van */
        CHECK(losses * 10 <= games);                       /* thua khong qua 10% */
    }

    /* luu / tai giu nguyen che do choi */
    const char *file = "__test_ai.caro";
    resetSession();
    resetData();
    play(5, 5, 1);
    aiLevel = AI_HARD;
    CHECK(saveGame(file) == SAVE_OK);
    aiLevel = AI_NONE;
    CHECK(loadGame(file) == SAVE_OK && aiLevel == AI_HARD);
    FILE *f = fopen(file, "w");
    fprintf(f, "CARO-SAVE 1\nsize 12\np1 A\np2 B\nwins 0 0\ndraws 0\nelapsed 0\ncursor 0 0\nmode 5\nboard\n");
    for (int i = 0; i < BOARD_SIZE; i++) fprintf(f, "0 0 0 0 0 0 0 0 0 0 0 0\n");
    fclose(f);
    CHECK(loadGame(file) == SAVE_ERR_FORMAT && aiLevel == AI_HARD);   /* file loi: khong doi che do */
    remove(file);
    aiLevel = AI_NONE;
}

static void testUndo(void) {
    printf("di lai (undo)\n");
    int r, c;
    resetData();
    CHECK(moveHistoryLength() == 0 && lastMoveMark() == 0);
    CHECK(undoMove(&r, &c) == 0);                         /* chua co nuoc nao */

    play(5, 5, 1); switchTurn();                          /* nguoi (X) */
    play(5, 6, -1); switchTurn();                         /* may (O) */
    play(6, 6, 1); switchTurn();
    play(4, 4, -1); switchTurn();
    CHECK(moveHistoryLength() == 4 && lastMoveMark() == -1 && turn == 1);

    /* di lai 1 cap: nuoc cua may roi nuoc cua nguoi */
    CHECK(undoMove(&r, &c) == 1 && r == 4 && c == 4);
    CHECK(undoMove(&r, &c) == 1 && r == 6 && c == 6);
    CHECK(chessBoard[4][4].mark == 0 && chessBoard[6][6].mark == 0);
    CHECK(turn == 1 && totalMoves == 2 && moveCount[0] == 1 && moveCount[1] == 1);
    CHECK(lastX == 5 && lastY == 6 && lastMoveMark() == -1);

    /* danh lai o do duoc, va van kiem tra thang dung */
    currX = 6; currY = 6;
    CHECK(checkLegalEnter(1) == 1 && moveHistoryLength() == 3);

    /* di lai het */
    while (undoMove(NULL, NULL)) {}
    CHECK(totalMoves == 0 && moveCount[0] == 0 && moveCount[1] == 0 && lastX == -1 && turn == 1);

    /* di lai nuoc thang -> khong con thang */
    resetData();
    for (int k = 0; k < 5; k++) play(0, k, 1);
    CHECK(checkWin(1) == 1);
    undoMove(NULL, NULL);
    CHECK(testBoard() == RES_NONE && winCount == 0);

    /* van moi / tai van: xoa lich su nuoc di */
    resetData();
    CHECK(moveHistoryLength() == 0 && undoCount == 0);
    int m[BOARD_SIZE][BOARD_SIZE];
    Session s;
    memset(m, 0, sizeof m);
    memset(&s, 0, sizeof s);
    strcpy(s.name[0], "A"); strcpy(s.name[1], "B");
    m[3][3] = 1; m[3][4] = -1;
    play(7, 7, 1);
    CHECK(loadState(m, 0, 0, &s, 0) == 0 && moveHistoryLength() == 0);
    CHECK(undoMove(NULL, NULL) == 0 && chessBoard[3][3].mark == 1);   /* khong xoa duoc quan da tai */
}

static void testClock(void) {
    printf("dong ho tung ben (5/10/15 phut)\n");
    timeLimitSec = TIME_5_MIN;
    resetData();                                          /* van moi: ca 2 ben day 5 phut */
    CHECK(turnClockLeftMs(0, 0) == 300000L && turnClockLeftMs(1, 0) == 300000L);
    CHECK(turnClockShownSeconds(0, 0) == 300 && !turnClockExpired(0));

    /* X nghi 2,5 giay: chi dong ho X bi tru, O giu nguyen */
    turnClockStart(1000);
    CHECK(turnClockLeftMs(0, 3500) == 297500L && turnClockLeftMs(1, 3500) == 300000L);
    CHECK(turnClockShownSeconds(0, 3500) == 298);         /* 297,5 giay -> hien 298 (lam tron len) */
    turnClockStop(3500);
    switchTurn();
    turnClockStart(3500);                                 /* den luot O */
    CHECK(turnClockLeftMs(1, 13500) == 290000L && turnClockLeftMs(0, 13500) == 297500L);

    /* tam dung (luu/tai/ESC): thoi gian khong bi tru */
    turnClockStop(13500);
    CHECK(turnClockLeftMs(1, 999999) == 290000L);
    turnClockStart(500000);
    CHECK(turnClockLeftMs(1, 501000) == 289000L);
    turnClockStart(400000);                               /* goi start khi dang chay: bo qua */
    CHECK(turnClockLeftMs(1, 501000) == 289000L);

    /* het gio */
    CHECK(!turnClockExpired(500000 + 289999));
    CHECK(turnClockExpired(500000 + 290000));
    CHECK(turnClockLeftMs(1, 500000 + 999999) == 0 && turnClockShownSeconds(1, 500000 + 999999) == 0);
    turnClockStop(500000 + 999999);
    CHECK(turnClockLeftMs(1, 0) == 0);                    /* khong bi am */

    /* 10 / 15 phut */
    turnClockReset(TIME_15_MIN);
    CHECK(timeLimitSec == 900 && turnClockLeftMs(0, 0) == 900000L);
    timeLimitSec = TIME_10_MIN;
    resetData();
    CHECK(turnClockLeftMs(1, 0) == 600000L);

    /* luu / tai giu thoi gian con lai */
    const char *file = "__test_clock.caro";
    resetSession();
    timeLimitSec = TIME_15_MIN;
    resetData();
    play(5, 5, 1);
    turnClockSet(TIME_15_MIN, 812345L, 777000L);
    CHECK(saveGame(file) == SAVE_OK);
    timeLimitSec = TIME_5_MIN;
    resetData();
    CHECK(loadGame(file) == SAVE_OK);
    CHECK(timeLimitSec == TIME_15_MIN && turnClockLeftMs(0, 0) == 812345L && turnClockLeftMs(1, 0) == 777000L);

    /* file sai: gio la, con lai > gioi han, con lai = 0 */
    const char *bad[] = { "clock 120 1000 1000", "clock 300 300001 1000", "clock 600 0 5000", "clock 600 abc" };
    for (int k = 0; k < 4; k++) {
        FILE *f = fopen(file, "w");
        fprintf(f, "CARO-SAVE 1\nsize 12\np1 A\np2 B\nwins 0 0\ndraws 0\nelapsed 0\ncursor 0 0\nmode 0\n%s\nboard\n", bad[k]);
        for (int i = 0; i < BOARD_SIZE; i++) fprintf(f, "0 0 0 0 0 0 0 0 0 0 0 0\n");
        fclose(f);
        CHECK(loadGame(file) == SAVE_ERR_FORMAT);
    }
    /* file cu (khong co dong clock): 10 phut, ca 2 ben day gio */
    FILE *f = fopen(file, "w");
    fprintf(f, "CARO-SAVE 1\nsize 12\np1 A\np2 B\nwins 0 0\ndraws 0\nelapsed 0\ncursor 0 0\nboard\n");
    for (int i = 0; i < BOARD_SIZE; i++) fprintf(f, "0 0 0 0 0 0 0 0 0 0 0 0\n");
    fclose(f);
    timeLimitSec = TIME_5_MIN;
    CHECK(loadGame(file) == SAVE_OK && timeLimitSec == TIME_10_MIN && turnClockLeftMs(1, 0) == 600000L);
    remove(file);
    timeLimitSec = TIME_DEFAULT;
}

int main(void) {
    testWinLines();
    testLoadState();
    testFileNames();
    testPlayerNames();
    testUtf8();
    testHistorySettings();
    testLang();
    testSaveLoad();
    testSound();
    testAI();
    testUndo();
    testClock();
    printf("\n%d dung, %d sai\n", passed, failed);
    return failed ? 1 : 0;
}
