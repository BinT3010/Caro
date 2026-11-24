/* Kiem thu GIAO DIEN CARO tu dong: chay caro.exe trong 1 cua so console rieng, "bam phim"
 * nhu nguoi choi that, doc lai man hinh (ky tu + mau) de kiem tra tung chuc nang:
 * menu, danh co, to sang, thang, choi lai, luu/tai, Esc, dau may De/Kho, di lai,
 * dong ho 5/10/15 phut, het gio (ca nguoi va may), History, Settings, Exit.
 * Ket qua ghi vao uitest.log (co ban chup man hinh khi co loi).
 *
 * CHAY TRONG 1 THU MUC RIENG vi chuong trinh se ghi de caro_settings.cfg (tat am thanh),
 * xoa caro_history.txt va tao vai file .caro. Vi du (cmd, o thu muc du an):
 *   gcc -std=c99 -O2 -D_WIN32_WINNT=0x0601 tests/ui_test.c -o ui_test.exe
 *   mkdir uitest & copy caro.exe uitest & cd uitest
 *   ..\ui_test.exe caro.exe
 * Se co 1 cua so game tu bat len va tu choi khoang 1 phut; dung bam phim trong luc do. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define W 90
#define H 30
static HANDLE hIn, hOut;
static FILE *logf;
static int pass, fail;
static char scr[H][W + 1];
static WORD attr[H][W];
static PROCESS_INFORMATION pi;
static int curR, curC;                       /* vi tri con tro do driver tu theo doi */

static void logp(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(logf, fmt, ap); va_end(ap); fflush(logf);
}

static void readScreen(void) {
    for (int y = 0; y < H; y++) {
        COORD c = { 0, (SHORT)y }; DWORD n = 0;
        ReadConsoleOutputCharacterA(hOut, scr[y], W, c, &n);
        scr[y][W] = '\0';
        ReadConsoleOutputAttribute(hOut, attr[y], W, c, &n);
    }
}

static void dump(const char *title) {
    readScreen();
    DWORD ec = 0;
    GetExitCodeProcess(pi.hProcess, &ec);
    logp("----- man hinh: %s  [tien trinh: %s, ma=0x%lx] -----\n", title,
         ec == STILL_ACTIVE ? "DANG CHAY" : "DA THOAT", ec);
    for (int y = 0; y < H; y++) {
        char line[W + 1]; strcpy(line, scr[y]);
        for (int k = W - 1; k >= 0 && line[k] == ' '; k--) line[k] = '\0';
        logp("%2d|%s\n", y, line);
    }
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < 12; c++)
            if ((attr[2 * r + 2][4 * c + 5] >> 4) == 7) logp("   (o dang to sang: hang %d, cot %d)\n", r, c);
}

static int seenNow(const char *s) {
    readScreen();
    for (int y = 0; y < H; y++) if (strstr(scr[y], s)) return 1;
    return 0;
}

/* Cho toi da ms mili giay den khi chuoi xuat hien */
static int waitFor(const char *s, int ms) {
    for (int t = 0; t <= ms; t += 50) { if (seenNow(s)) return 1; Sleep(50); }
    return 0;
}

/* Console ve tung dong mot (cham), nen "co tren man hinh" = cho toi da 1,5 giay. */
static int onScreen(const char *s) { return waitFor(s, 1500); }

static void dump(const char *title);
static void check(const char *name, int ok) {
    if (!ok) dump(name);
    if (ok) pass++; else fail++;
    logp("%s  %s\n", ok ? "[OK]  " : "[LOI] ", name);
}

static void sendKey(WORD vk, char ch, DWORD ctrl) {
    INPUT_RECORD r[2]; DWORD n;
    memset(r, 0, sizeof r);
    for (int k = 0; k < 2; k++) {
        r[k].EventType = KEY_EVENT;
        r[k].Event.KeyEvent.bKeyDown = k == 0;
        r[k].Event.KeyEvent.wRepeatCount = 1;
        r[k].Event.KeyEvent.wVirtualKeyCode = vk;
        r[k].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKeyA(vk, 0);
        r[k].Event.KeyEvent.uChar.AsciiChar = ch;
        r[k].Event.KeyEvent.dwControlKeyState = ctrl;
    }
    WriteConsoleInputA(hIn, r, 2, &n);
    Sleep(120);
}
static void key(char ch) { sendKey((WORD)(VkKeyScanA(ch) & 0xff), ch, 0); }
static void enter(void) { sendKey(VK_RETURN, '\r', 0); }
static void esc(void) { sendKey(VK_ESCAPE, 27, 0); }
static void arrow(WORD vk) { sendKey(vk, 0, ENHANCED_KEY); }
static void down(int n) { while (n--) arrow(VK_DOWN); }
static void text(const char *s) { while (*s) key(*s++); }

static char cell(int r, int c) { readScreen(); return scr[2 * r + 2][4 * c + 5]; }
static int countMarks(char m) {
    readScreen(); int n = 0;
    for (int r = 0; r < 12; r++) for (int c = 0; c < 12; c++) if (scr[2 * r + 2][4 * c + 5] == m) n++;
    return n;
}

/* Di chuyen con tro toi (r,c) bang ca WASD va phim mui ten, roi danh */
static void moveTo(int r, int c) {
    while (curR < r) { key('s'); curR++; }
    while (curR > r) { arrow(VK_UP); curR--; }
    while (curC < c) { arrow(VK_RIGHT); curC++; }
    while (curC > c) { key('a'); curC--; }
}
static void playAt(int r, int c) { moveTo(r, c); enter(); }

/* Sau khi may danh, game cho con tro nhay toi o may vua danh: doc lai vi tri o dang to sang. */
static void syncCursor(void) {
    readScreen();
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < 12; c++)
            if ((attr[2 * r + 2][4 * c + 5] >> 4) == 7) { curR = r; curC = c; return; }
}

static void writeFile(const char *name, const char *head, const char *rows[12]) {
    FILE *f = fopen(name, "w");
    fputs(head, f);
    for (int i = 0; i < 12; i++) fprintf(f, "%s\n", rows[i]);
    fclose(f);
}

int main(int argc, char **argv) {
    const char *exe = argc > 1 ? argv[1] : "caro.exe";
    logf = fopen("uitest.log", "w");

    /* tat am thanh, dat ten de kiem tra */
    FILE *cfg = fopen("caro_settings.cfg", "w");
    fputs("sound=0\nvolume=50\nname1=An\nname2=Binh\n", cfg);
    fclose(cfg);
    remove("caro_history.txt");

    STARTUPINFOA si;
    memset(&si, 0, sizeof si); si.cb = sizeof si;
    char cmd[MAX_PATH]; snprintf(cmd, sizeof cmd, "\"%s\"", exe);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        logp("Khong chay duoc %s (loi %lu)\n", exe, GetLastError()); return 2;
    }
    Sleep(800);
    FreeConsole();
    if (!AttachConsole(pi.dwProcessId)) { logp("AttachConsole loi %lu\n", GetLastError()); return 2; }
    hIn = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    hOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    /* ================= A. Menu + 2 nguoi choi, 5 phut ================= */
    logp("\n=== A. MENU CHINH, NEW GAME 2 NGUOI, 5 PHUT ===\n");
    check("Menu chinh hien 'C A R O' + 5 muc", waitFor("C A R O", 3000) && onScreen("New Game") && onScreen("Load Game")
          && onScreen("History") && onScreen("Settings") && onScreen("Exit"));
    enter();
    check("Menu NEW GAME co du 4 che do, chu 'Trung binh' khong bi cat",
          waitFor("NEW GAME", 2000) && onScreen("2 nguoi choi") && onScreen("Choi voi may - De")
          && onScreen("Choi voi may - Trung binh") && onScreen("Choi voi may - Kho"));
    enter();
    check("Menu THOI GIAN co 5 / 10 / 15 phut", waitFor("THOI GIAN", 2000) && onScreen("5 phut moi ben")
          && onScreen("10 phut moi ben") && onScreen("15 phut moi ben"));
    esc();
    check("Esc o menu thoi gian -> quay lai chon che do", waitFor("NEW GAME", 2000));
    enter();
    waitFor("THOI GIAN", 2000);
    enter();
    check("Vao ban co: HUD 'Gio moi ben: 5 phut'", waitFor("Gio moi ben: 5 phut", 3000));
    dump("bat dau van 2 nguoi 5 phut");
    check("HUD: che do 2 nguoi, ten An / Binh, con lai 05:00", onScreen("Che do: 2 nguoi choi") && onScreen("[X] An")
          && onScreen("[O] Binh") && onScreen("Con lai: 05:00"));
    check("Che do 2 nguoi KHONG hien dong 'U: di lai'", !seenNow("U: di lai"));
    readScreen();
    check("O dang chon (0,0) duoc to nen sang", (attr[2][5] >> 4) == 7);
    curR = curC = 0;

    enter();
    check("Danh X tai (0,0)", cell(0, 0) == 'X');
    check("Doi luot sang O (Binh)", waitFor("Luot choi: O (Binh)", 1000));
    enter();
    check("Danh vao o da co -> bao loi", waitFor("O nay da duoc danh roi", 1000));
    key('u');
    check("Bam U khi choi 2 nguoi -> 'Chi duoc di lai khi choi voi may!'", waitFor("Chi duoc di lai khi choi voi may!", 1000));
    check("U khong xoa quan nao", cell(0, 0) == 'X');

    /* dong ho chi chay cho ben dang toi luot (O) */
    readScreen();
    char xBefore[W + 1]; strcpy(xBefore, scr[8]);
    Sleep(2300);
    readScreen();
    check("Dong ho X (khong toi luot) dung yen", strcmp(xBefore, scr[8]) == 0);
    check("Dong ho O (dang toi luot) giam xuong 04:5x", strstr(scr[12], "Con lai: 04:5") != NULL);
    check("Dong ho ben dang toi luot co dau '<'", strstr(scr[12], "<") != NULL && strstr(scr[8], "<") == NULL);

    playAt(0, 1);                                         /* O */
    check("Danh O tai (0,1)", cell(0, 1) == 'O');
    readScreen();
    check("To sang da chuyen sang o moi, o cu het to", (attr[2][9] >> 4) == 7 && (attr[2][5] >> 4) == 0);
    int xr[] = { 2, 2, 2, 2, 2 }, xc[] = { 0, 1, 2, 3, 4 };
    int orow[] = { 4, 4, 4, 4 }, oc[] = { 0, 1, 2, 3 };
    for (int k = 0; k < 4; k++) { playAt(xr[k], xc[k]); playAt(orow[k], oc[k]); }
    playAt(xr[4], xc[4]);
    check("5 quan X lien tiep -> 'CHUC MUNG An (X) DA CHIEN THANG'", waitFor("CHUC MUNG An (X) DA CHIEN THANG", 3000));
    check("Menu cuoi van hien 'Choi lai!' / 'Ve menu!'", waitFor("Choi lai!", 3000) && onScreen("Ve menu!"));
    dump("thang van 1");
    check("Lich su/diem: X Thang 1, Tong so van 1", onScreen("Thang: 1   Thua: 0") && onScreen("Tong so van: 1"));
    key('y');
    check("Choi lai: ban co trong, dong ho day lai 05:00", waitFor("Luot choi: X (An)", 3000) && waitFor("Con lai: 05:00", 2000) && cell(2, 0) == ' '
          && countMarks('X') == 0 && onScreen("Con lai: 05:00"));
    check("Choi lai giu diem (Tong so van: 1) va muc 5 phut", onScreen("Tong so van: 1") && onScreen("Gio moi ben: 5 phut"));
    curR = curC = 0;

    /* ---- luu van ---- */
    playAt(3, 3);                                         /* X */
    key('l');
    check("L: hien o nhap ten + danh sach file", waitFor("Nhap ten file de luu", 1500) && onScreen("(Chua co file luu nao)"));
    dump("dang nhap ten file luu");
    text("con"); enter();
    check("Ten 'con' (thiet bi Windows) bi tu choi", waitFor("Ten file khong hop le", 1500));
    text("van1"); enter();
    check("Luu thanh cong van1.caro", waitFor("Da luu van co vao file: van1.caro", 1500));
    check("Sau khi luu: phan huong dan phim duoc ve lai, khong bi cat chu", waitFor("W/A/S/D, mui ten: di chuyen", 1500) && onScreen("Esc: thoat ve menu") && onScreen("L: luu van"));
    {
        FILE *f = fopen("van1.caro", "r"); char buf[2000] = ""; if (f) { fread(buf, 1, sizeof buf - 1, f); fclose(f); }
        check("File van1.caro co 'mode 0' va 'clock 300 ...'", strstr(buf, "mode 0") && strstr(buf, "clock 300 "));
    }
    key('l'); text("van1"); enter();
    check("Luu trung ten -> hoi ghi de", waitFor("File da ton tai. Ghi de? (Y/N)", 1500));
    key('y');
    check("Ghi de thanh cong", waitFor("Da luu van co vao file: van1.caro", 1500));

    /* ---- tai van ---- */
    playAt(3, 4);                                         /* O */
    check("Danh them O (3,4) truoc khi tai", cell(3, 4) == 'O');
    key('t');
    check("T: danh sach file co van1.caro", waitFor("Nhap ten file can tai", 1500) && onScreen("van1.caro"));
    text("khongco"); enter();
    check("Tai file khong ton tai -> bao loi", waitFor("Khong mo duoc file", 1500));
    text("van1"); enter();
    check("Tai lai van1: (3,3)=X, (3,4) trong", waitFor("Da tai van co", 2000) && cell(3, 3) == 'X' && cell(3, 4) == ' ');
    check("Sau khi tai: den luot O, gio 5 phut", onScreen("Luot choi: O (Binh)") && onScreen("Gio moi ben: 5 phut"));

    esc();
    check("Esc: hoi thoat ve menu", waitFor("Thoat ve menu?", 1500));
    key('x');
    check("Phim khac: o lai van", waitFor("Luot choi: O (Binh)", 1500) && cell(3, 3) == 'X');
    esc(); key('n');
    check("Esc + N: ve menu chinh", waitFor("C A R O", 2000));

    /* ================= B. Dau voi may De, 10 phut + di lai ================= */
    logp("\n=== B. DAU VOI MAY DE, 10 PHUT, DI LAI ===\n");
    enter(); waitFor("NEW GAME", 2000); down(1); enter();
    waitFor("THOI GIAN", 2000); down(1); enter();
    check("Vao van: May (De), 10 phut, co dong 'U: di lai'", waitFor("Gio moi ben: 10 phut", 3000) && onScreen("[O] May (De)")
          && onScreen("Che do: dau voi may - De") && onScreen("U: di lai (da dung 0 lan)") && onScreen("Con lai: 10:00"));
    curR = curC = 0;
    playAt(5, 5);
    Sleep(1200); syncCursor();
    check("May tu danh 1 quan O sau nuoc cua nguoi", countMarks('X') == 1 && countMarks('O') == 1);
    check("Lai den luot nguoi", onScreen("Luot choi: X (An)"));
    dump("sau nuoc dau voi may De");
    key('u');
    check("U: xoa ca nuoc may + nuoc nguoi", waitFor("Da di lai", 1500) && countMarks('X') == 0 && countMarks('O') == 0);
    check("HUD: 'da dung 1 lan', so nuoc di ve 0", onScreen("U: di lai (da dung 1 lan)") && onScreen("Nuoc di: 0"));
    key('u');
    check("U khi chua co nuoc -> 'Khong con nuoc nao de di lai!'", waitFor("Khong con nuoc nao de di lai!", 1500));
    curR = 5; curC = 5;
    playAt(5, 5); Sleep(1200); syncCursor();
    int r2 = 6, c2 = 6;                                   /* chon o con trong cho nuoc thu 2 */
    while (cell(r2, c2) != ' ') c2++;
    playAt(r2, c2); Sleep(1200); syncCursor();
    check("Danh 2 nuoc -> may danh 2 nuoc", countMarks('X') == 2 && countMarks('O') == 2);
    key('u');
    Sleep(300);
    logp("   (driver: nuoc thu 2 cua nguoi o hang %d, cot %d)\n", r2, c2);
    check("U lan 2: con 1 X + 1 O, o vua danh duoc xoa", countMarks('X') == 1 && countMarks('O') == 1 && cell(r2, c2) == ' ');
    readScreen();
    check("U lan 2: con tro (to sang) ve dung o vua xoa", (attr[2 * r2 + 2][4 * c2 + 5] >> 4) == 7);
    esc(); key('n'); waitFor("C A R O", 2000);

    /* ================= C. Dau voi may Kho, 15 phut ================= */
    logp("\n=== C. DAU VOI MAY KHO, 15 PHUT ===\n");
    enter(); waitFor("NEW GAME", 2000); down(3); enter();
    waitFor("THOI GIAN", 2000); down(2); enter();
    check("Vao van: May (Kho), 15 phut", waitFor("Gio moi ben: 15 phut", 4000) && waitFor("[O] May (Kho)", 2000) && waitFor("Con lai: 15:00", 2000));
    curR = curC = 0;
    int hx[][2] = { { 5, 5 }, { 5, 6 }, { 5, 7 } };
    int okAi = 1;
    for (int k = 0; k < 3; k++) {
        if (cell(hx[k][0], hx[k][1]) != ' ') { hx[k][0] = 7; hx[k][1] = 2 + k; }
        playAt(hx[k][0], hx[k][1]);
        Sleep(2000); syncCursor();
        if (countMarks('O') != k + 1) okAi = 0;
    }
    check("May Kho tra loi moi nuoc (3 nuoc)", okAi);
    dump("dau voi may Kho");
    readScreen();
    check("Dong ho cua may (O) da bi tru (chay ca khi may nghi)", strstr(scr[12], "Con lai: 15:00") == NULL);
    esc(); key('n'); waitFor("C A R O", 2000);

    /* ================= D. Het gio (file luu con it giay) ================= */
    logp("\n=== D. HET GIO ===\n");
    const char *empty[12], *one[12];
    for (int i = 0; i < 12; i++) { empty[i] = "0 0 0 0 0 0 0 0 0 0 0 0"; one[i] = empty[i]; }
    one[6] = "0 0 0 0 0 1 0 0 0 0 0 0";
    writeFile("hetgio.caro", "CARO-SAVE 1\nsize 12\np1 An\np2 Binh\nwins 0 0\ndraws 0\nelapsed 0\ncursor 0 0\nmode 0\nclock 300 3000 300000\nboard\n", empty);
    writeFile("mayhetgio.caro", "CARO-SAVE 1\nsize 12\np1 An\np2 May (Kho)\nwins 0 0\ndraws 0\nelapsed 0\ncursor 6 5\nmode 3\nclock 300 300000 100\nboard\n", one);
    down(1); enter();
    check("Load Game: danh sach co hetgio.caro, mayhetgio.caro, van1.caro",
          waitFor("LOAD GAME", 2000) && onScreen("hetgio.caro") && onScreen("mayhetgio.caro") && onScreen("van1.caro"));
    text("hetgio"); enter();
    check("Tai van X con 3 giay: hien 00:03, mau do", waitFor("Con lai: 00:0", 2000));
    readScreen();
    check("Dong ho X duoi 30 giay -> mau do", (attr[8][60] & 0x0f) == 12);
    check("X het gio -> 'HET GIO! CHUC MUNG Binh (O)'", waitFor("HET GIO! CHUC MUNG Binh (O) DA CHIEN THANG", 6000));
    check("Sau khi het gio: dong ho X dung o 00:00", waitFor("Con lai: 00:00", 2000));
    dump("X het gio");
    waitFor("Ve menu!", 3000);
    key('n');
    check("Ve menu sau khi het gio", waitFor("C A R O", 2000));

    down(1); enter(); waitFor("LOAD GAME", 2000);
    text("mayhetgio"); enter();
    check("May (O) con 0,1 giay -> may het gio, nguoi thang",
          waitFor("HET GIO! CHUC MUNG An DA THANG May (Kho)", 5000));
    check("May khong kip danh quan nao", countMarks('O') == 0);
    waitFor("Ve menu!", 3000);
    key('n'); waitFor("C A R O", 2000);

    /* ================= E. History, Settings, Exit ================= */
    logp("\n=== E. HISTORY, SETTINGS, EXIT ===\n");
    down(2); enter();
    check("History: 3 van (1 thang thuong + 2 het gio)", waitFor("LICH SU DAU", 2000) && onScreen("Tong: 3 van"));
    check("History ghi ten may 'May (Kho)'", onScreen("May (Kho)"));
    dump("lich su");
    esc(); waitFor("C A R O", 2000);
    down(3); enter();
    check("Settings hien am thanh TAT + ten An/Binh", waitFor("SETTINGS", 2000) && onScreen("Am thanh         : TAT")
          && onScreen("Ten nguoi choi 1 : An"));
    esc(); waitFor("C A R O", 2000);
    down(4); enter();
    DWORD w = WaitForSingleObject(pi.hProcess, 4000);
    DWORD code = 99; GetExitCodeProcess(pi.hProcess, &code);
    check("Exit: chuong trinh thoat, ma 0", w == WAIT_OBJECT_0 && code == 0);
    if (w != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 1);

    logp("\nTONG KET: %d dung, %d loi\n", pass, fail);
    fclose(logf);
    return fail ? 1 : 0;
}
