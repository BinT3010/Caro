/* Kiem thu GIAO DIEN CARO tu dong: chay caro.exe trong 1 cua so console rieng, "bam phim"
 * nhu nguoi choi that, doc lai man hinh (ky tu + mau) de kiem tra tung chuc nang:
 * menu, danh co, to sang, thang, choi lai, luu/tai, Esc, dau may De/Kho, di lai,
 * dat ten khi bat dau van (go tieng Viet co dau), dong ho 5/10/15 phut, het gio
 * (ca nguoi va may), Lich su, Cai dat, Thoat.
 * Man hinh duoc doc dang Unicode (ReadConsoleOutputCharacterW) roi doi sang UTF-8, nen
 * cac chuoi can tim viet thang bang tieng Viet co dau (file nay luu UTF-8).
 * Ket qua ghi vao uitest.log (UTF-8, co ban chup man hinh khi co loi).
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
#include <wchar.h>
#ifndef VK_PACKET
#define VK_PACKET 0xE7          /* MinGW.org cu khong co: phim "ky tu Unicode" (bo go tieng Viet dung) */
#endif

#define W 90
#define H 30
static HANDLE hIn, hOut;
static FILE *logf;
static int pass, fail;
static WCHAR wscr[H][W];                     /* ky tu tung o (de doc X/O tren ban co) */
static char scr[H][3 * W + 1];               /* tung dong doi sang UTF-8 (de tim chuoi) */
static WORD attr[H][W];
static PROCESS_INFORMATION pi;
static int curR, curC;                       /* vi tri con tro do driver tu theo doi */

static void logp(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(logf, fmt, ap); va_end(ap); fflush(logf);
}

static void readScreen(void) {
    for (int y = 0; y < H; y++) {
        COORD c = { 0, (SHORT)y }; DWORD n = 0;
        ReadConsoleOutputCharacterW(hOut, wscr[y], W, c, &n);
        int len = WideCharToMultiByte(CP_UTF8, 0, wscr[y], W, scr[y], 3 * W, NULL, NULL);
        scr[y][len > 0 ? len : 0] = '\0';
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
        char line[3 * W + 1]; strcpy(line, scr[y]);
        for (int k = (int)strlen(line) - 1; k >= 0 && line[k] == ' '; k--) line[k] = '\0';
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

static void check(const char *name, int ok) {
    if (!ok) dump(name);
    if (ok) pass++; else fail++;
    logp("%s  %s\n", ok ? "[OK]  " : "[LOI] ", name);
}

static void sendKey(WORD vk, WCHAR ch, DWORD ctrl) {
    INPUT_RECORD r[2]; DWORD n;
    memset(r, 0, sizeof r);
    for (int k = 0; k < 2; k++) {
        r[k].EventType = KEY_EVENT;
        r[k].Event.KeyEvent.bKeyDown = k == 0;
        r[k].Event.KeyEvent.wRepeatCount = 1;
        r[k].Event.KeyEvent.wVirtualKeyCode = vk;
        r[k].Event.KeyEvent.wVirtualScanCode = vk == VK_PACKET ? 0 : (WORD)MapVirtualKeyA(vk, 0);
        r[k].Event.KeyEvent.uChar.UnicodeChar = ch;
        r[k].Event.KeyEvent.dwControlKeyState = ctrl;
    }
    WriteConsoleInputW(hIn, r, 2, &n);
    Sleep(120);
}
static void key(char ch) { sendKey((WORD)(VkKeyScanA(ch) & 0xff), (WCHAR)(unsigned char)ch, 0); }
static void enter(void) { sendKey(VK_RETURN, '\r', 0); }
static void esc(void) { sendKey(VK_ESCAPE, 27, 0); }
static void backspace(void) { sendKey(VK_BACK, 8, 0); }
static void arrow(WORD vk) { sendKey(vk, 0, ENHANCED_KEY); }
static void down(int n) { while (n--) arrow(VK_DOWN); }
static void text(const char *s) { while (*s) key(*s++); }
/* Go chuoi UTF-8 (co dau) nhu bo go Unikey/EVKey: moi chu la 1 phim VK_PACKET mang ma Unicode. */
static void typeUtf8(const char *s) {
    WCHAR w[128];
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, w, 128);
    for (int k = 0; k < n - 1; k++) {
        if (w[k] < 128) key((char)w[k]);
        else sendKey(VK_PACKET, w[k], 0);
    }
}

static char cell(int r, int c) { readScreen(); return (char)wscr[2 * r + 2][4 * c + 5]; }
static int countMarks(char m) {
    readScreen(); int n = 0;
    for (int r = 0; r < 12; r++) for (int c = 0; c < 12; c++) if (wscr[2 * r + 2][4 * c + 5] == (WCHAR)m) n++;
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

static void readAll(const char *name, char *buf, size_t cap) {
    FILE *f = fopen(name, "r");
    buf[0] = '\0';
    if (f) { size_t n = fread(buf, 1, cap - 1, f); buf[n] = '\0'; fclose(f); }
}

int main(int argc, char **argv) {
    const char *exe = argc > 1 ? argv[1] : "caro.exe";
    logf = fopen("uitest.log", "w");

    /* tat am thanh, dat ten de kiem tra (ten O co dau: doc cai dat UTF-8) */
    FILE *cfg = fopen("caro_settings.cfg", "w");
    fputs("sound=0\nvolume=50\nname1=An\nname2=Bình\n", cfg);
    fclose(cfg);
    remove("caro_history.txt");

    STARTUPINFOA si;
    memset(&si, 0, sizeof si); si.cb = sizeof si;
    char cmd[MAX_PATH]; snprintf(cmd, sizeof cmd, "\"%s\"", exe);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        logp("Khong chay duoc %s (loi %lu)\n", exe, GetLastError()); return 2;
    }
    FreeConsole();
    int attached = 0;                            /* cua so game co the mo cham: thu lai toi da 5 giay */
    for (int t = 0; t < 50 && !attached; t++) { Sleep(100); attached = AttachConsole(pi.dwProcessId); }
    if (!attached) { logp("AttachConsole loi %lu\n", GetLastError()); TerminateProcess(pi.hProcess, 1); return 2; }
    Sleep(300);
    hIn = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    hOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    /* ================= A. Menu + 2 nguoi choi, 5 phut ================= */
    logp("\n=== A. MENU CHINH, VAN MOI 2 NGUOI, 5 PHUT ===\n");
    check("Menu chinh hien 'C A R O' + 5 muc tieng Viet", waitFor("C A R O", 3000) && onScreen("Ván mới")
          && onScreen("Tải ván") && onScreen("Lịch sử đấu") && onScreen("Cài đặt") && onScreen("Thoát"));
    check("Dong huong dan menu co dau", onScreen("W/S hoặc mũi tên: chọn    Enter: đồng ý    Esc: quay lại"));
    enter();
    check("Menu VAN MOI co du 4 che do, chu 'Trung bình' khong bi cat",
          waitFor("VÁN MỚI", 2000) && onScreen("2 người chơi") && onScreen("Chơi với máy - Dễ")
          && onScreen("Chơi với máy - Trung bình") && onScreen("Chơi với máy - Khó") && onScreen("Quay lại"));
    enter();
    check("Menu THOI GIAN co 5 / 10 / 15 phut", waitFor("THỜI GIAN", 2000) && onScreen("5 phút mỗi bên")
          && onScreen("10 phút mỗi bên") && onScreen("15 phút mỗi bên"));
    esc();
    check("Esc o menu thoi gian -> quay lai chon che do", waitFor("VÁN MỚI", 2000));
    enter();
    waitFor("THỜI GIAN", 2000);
    enter();
    check("Man hinh DAT TEN: hoi ten X, mac dinh \"An\" (tu Cai dat)",
          waitFor("ĐẶT TÊN NGƯỜI CHƠI", 2000) && onScreen("Người chơi X (mặc định \"An\")")
          && onScreen("Tối đa 20 ký tự"));
    enter();                                              /* Enter de trong -> giu ten An */
    check("Enter de trong: giu ten X = An, hoi ten O (mac dinh \"Bình\" - doc tu file cai dat co dau)",
          waitFor("Người chơi X: An", 1500) && onScreen("Người chơi O (mặc định \"Bình\")"));
    text("aN"); enter();                                  /* trung ten X (khac hoa thuong) */
    check("Ten O trung ten X (khac hoa thuong) -> bao loi, hoi lai", waitFor("Tên bị trùng", 1500)
          && onScreen("Người chơi O (mặc định \"Bình\")"));
    enter();                                              /* giu ten Bình */
    check("Vao ban co: HUD 'Giờ mỗi bên: 5 phút'", waitFor("Giờ mỗi bên: 5 phút", 3000));
    dump("bat dau van 2 nguoi 5 phut");
    check("HUD: che do 2 nguoi, ten An / Bình, con lai 05:00", onScreen("Chế độ: 2 người chơi") && onScreen("[X] An")
          && onScreen("[O] Bình") && onScreen("Còn lại: 05:00"));
    check("HUD: huong dan phim co dau", onScreen("W/A/S/D, mũi tên: di chuyển") && onScreen("Enter: đánh cờ")
          && onScreen("L: lưu ván    T: tải ván") && onScreen("Esc: thoát về menu"));
    check("Che do 2 nguoi KHONG hien dong 'U: đi lại'", !seenNow("U: đi lại"));
    readScreen();
    check("O dang chon (0,0) duoc to nen sang", (attr[2][5] >> 4) == 7);
    /* bang thong so bat dau o cot 55: chu co dau (1 o / chu) khong duoc lam lech cot */
    check("Chu co dau khong lam lech cot: 'Giờ mỗi bên', 'Còn lại' bat dau dung cot 55",
          wcsncmp(&wscr[2][55], L"Gi\x1EDD m\x1ED7i b\x00EAn: 5 ph\x00FAt", 19) == 0
          && wcsncmp(&wscr[8][55], L"    C\x00F2n l\x1EA1i: 05:00", 18) == 0);
    curR = curC = 0;

    enter();
    check("Danh X tai (0,0)", cell(0, 0) == 'X');
    check("Doi luot sang O (Bình)", waitFor("Lượt chơi: O (Bình)", 1000));
    enter();
    check("Danh vao o da co -> bao loi", waitFor("Ô này đã được đánh rồi", 1000));
    key('u');
    check("Bam U khi choi 2 nguoi -> 'Chỉ được đi lại khi chơi với máy!'", waitFor("Chỉ được đi lại khi chơi với máy!", 1000));
    check("U khong xoa quan nao", cell(0, 0) == 'X');

    /* dong ho chi chay cho ben dang toi luot (O) */
    readScreen();
    char xBefore[3 * W + 1]; strcpy(xBefore, scr[8]);
    Sleep(2300);
    readScreen();
    check("Dong ho X (khong toi luot) dung yen", strcmp(xBefore, scr[8]) == 0);
    check("Dong ho O (dang toi luot) giam xuong 04:5x", strstr(scr[12], "Còn lại: 04:5") != NULL);
    check("Dong ho ben dang toi luot co dau '<'", strstr(scr[12], "<") != NULL && strstr(scr[8], "<") == NULL);

    playAt(0, 1);                                         /* O */
    check("Danh O tai (0,1)", cell(0, 1) == 'O');
    readScreen();
    check("To sang da chuyen sang o moi, o cu het to", (attr[2][9] >> 4) == 7 && (attr[2][5] >> 4) == 0);
    int xr[] = { 2, 2, 2, 2, 2 }, xc[] = { 0, 1, 2, 3, 4 };
    int orow[] = { 4, 4, 4, 4 }, oc[] = { 0, 1, 2, 3 };
    for (int k = 0; k < 4; k++) { playAt(xr[k], xc[k]); playAt(orow[k], oc[k]); }
    playAt(xr[4], xc[4]);
    check("5 quan X lien tiep -> 'CHÚC MỪNG An (X) ĐÃ CHIẾN THẮNG'", waitFor("CHÚC MỪNG An (X) ĐÃ CHIẾN THẮNG", 3000));
    check("Menu cuoi van hien 'Chơi lại!' / 'Về menu!'", waitFor("Chơi lại!", 3000) && onScreen("Về menu!"));
    dump("thang van 1");
    check("Diem: X Thắng 1, Tổng số ván 1", onScreen("Thắng: 1   Thua: 0") && onScreen("Tổng số ván: 1"));
    key('y');
    check("Choi lai: ban co trong, dong ho day lai 05:00", waitFor("Lượt chơi: X (An)", 3000) && waitFor("Còn lại: 05:00", 2000) && cell(2, 0) == ' '
          && countMarks('X') == 0 && onScreen("Còn lại: 05:00"));
    check("Choi lai giu diem (Tổng số ván: 1) va muc 5 phut", onScreen("Tổng số ván: 1") && onScreen("Giờ mỗi bên: 5 phút"));
    curR = curC = 0;

    /* ---- luu van ---- */
    playAt(3, 3);                                         /* X */
    key('l');
    check("L: hien o nhap ten + danh sach file", waitFor("Nhập tên file để lưu", 1500) && onScreen("(Chưa có file lưu nào)"));
    dump("dang nhap ten file luu");
    text("con"); enter();
    check("Ten 'con' (thiet bi Windows) bi tu choi", waitFor("Tên file không hợp lệ", 1500));
    sendKey(VK_PACKET, 0x1EC7, 0);                        /* go chu 'ệ': ten file khong nhan chu co dau */
    text("van1"); enter();
    check("Luu thanh cong van1.caro (chu co dau trong ten file bi bo qua)", waitFor("Đã lưu ván cờ vào file: van1.caro", 1500));
    check("Sau khi luu: phan huong dan phim duoc ve lai, khong bi cat chu", waitFor("W/A/S/D, mũi tên: di chuyển", 1500)
          && onScreen("Esc: thoát về menu") && onScreen("L: lưu ván"));
    {
        char buf[2000]; readAll("van1.caro", buf, sizeof buf);
        check("File van1.caro co 'mode 0', 'clock 300 ...' va ten 'p2 Bình' (UTF-8)",
              strstr(buf, "mode 0") && strstr(buf, "clock 300 ") && strstr(buf, "p2 Bình\n"));
    }
    key('l'); text("van1"); enter();
    check("Luu trung ten -> hoi ghi de", waitFor("File đã tồn tại. Ghi đè? (Y: có / N: không)", 1500));
    key('y');
    check("Ghi de thanh cong", waitFor("Đã lưu ván cờ vào file: van1.caro", 1500));

    /* ---- tai van ---- */
    playAt(3, 4);                                         /* O */
    check("Danh them O (3,4) truoc khi tai", cell(3, 4) == 'O');
    key('t');
    check("T: danh sach file co van1.caro", waitFor("Nhập tên file cần tải", 1500) && onScreen("van1.caro"));
    text("khongco"); enter();
    check("Tai file khong ton tai -> bao loi", waitFor("Không mở được file", 1500));
    text("van1"); enter();
    check("Tai lai van1: (3,3)=X, (3,4) trong", waitFor("Đã tải ván cờ", 2000) && cell(3, 3) == 'X' && cell(3, 4) == ' ');
    check("Sau khi tai: den luot O (ten co dau doc lai tu file), gio 5 phut", onScreen("Lượt chơi: O (Bình)")
          && onScreen("Giờ mỗi bên: 5 phút"));

    esc();
    check("Esc: hoi thoat ve menu", waitFor("Thoát về menu?", 1500));
    key('x');
    check("Phim khac: o lai van", waitFor("Lượt chơi: O (Bình)", 1500) && cell(3, 3) == 'X');
    esc(); key('n');
    check("Esc + N: ve menu chinh", waitFor("C A R O", 2000));

    /* ================= B. Dau voi may De, 10 phut + di lai ================= */
    logp("\n=== B. DAU VOI MAY DE, 10 PHUT, DI LAI ===\n");
    enter(); waitFor("VÁN MỚI", 2000); down(1); enter();
    waitFor("THỜI GIAN", 2000); down(1); enter();
    check("Dau may: man hinh DAT TEN chi hoi ten nguoi (X)", waitFor("ĐẶT TÊN NGƯỜI CHƠI", 2000)
          && onScreen("Người chơi X (mặc định \"An\")") && !seenNow("Người chơi O"));
    typeUtf8("  Trần Lâm");                               /* go tieng Viet nhu Unikey */
    check("Go ten co dau: hien 'Trần Lâm' trong o nhap", waitFor("Trần Lâm", 1500));
    backspace(); typeUtf8("n "); enter();                 /* Backspace xoa ca chu 'm'; khoang trang 2 dau bi cat */
    check("Dat ten moi 'Trần Lân' + hien doi thu 'Máy (Dễ)'", waitFor("Người chơi X: Trần Lân", 1500)
          && waitFor("Đối thủ       O: Máy (Dễ)", 1500));
    check("Vao van: Máy (Dễ), 10 phut, co dong 'U: đi lại'", waitFor("Giờ mỗi bên: 10 phút", 3000) && onScreen("[O] Máy (Dễ)")
          && onScreen("Chế độ: đấu với máy - Dễ") && onScreen("U: đi lại (đã dùng 0 lần)") && onScreen("Còn lại: 10:00"));
    curR = curC = 0;
    playAt(5, 5);
    Sleep(1200); syncCursor();
    check("May tu danh 1 quan O sau nuoc cua nguoi", countMarks('X') == 1 && countMarks('O') == 1);
    check("Lai den luot nguoi (ten moi Trần Lân)", onScreen("Lượt chơi: X (Trần Lân)") && onScreen("[X] Trần Lân"));
    dump("sau nuoc dau voi may De");
    key('u');
    check("U: xoa ca nuoc may + nuoc nguoi", waitFor("Đã đi lại", 1500) && countMarks('X') == 0 && countMarks('O') == 0);
    check("HUD: 'đã dùng 1 lần', so nuoc di ve 0", onScreen("U: đi lại (đã dùng 1 lần)") && onScreen("Nước đi: 0"));
    key('u');
    check("U khi chua co nuoc -> 'Không còn nước nào để đi lại!'", waitFor("Không còn nước nào để đi lại!", 1500));
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
    enter(); waitFor("VÁN MỚI", 2000); down(3); enter();
    waitFor("THỜI GIAN", 2000); down(2); enter();
    check("Van moi: ten mac dinh lai la An (ten Trần Lân chi ap dung van truoc)",
          waitFor("ĐẶT TÊN NGƯỜI CHƠI", 2000) && onScreen("Người chơi X (mặc định \"An\")"));
    esc();                                                /* Esc = giu ten mac dinh */
    check("Vao van: Máy (Khó), 15 phut, Esc giu ten An", waitFor("Giờ mỗi bên: 15 phút", 4000) && waitFor("[X] An", 2000)
          && waitFor("[O] Máy (Khó)", 2000) && waitFor("Còn lại: 15:00", 2000));
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
    check("Dong ho cua may (O) da bi tru (chay ca khi may nghi)", strstr(scr[12], "Còn lại: 15:00") == NULL);
    esc(); key('n'); waitFor("C A R O", 2000);

    /* ================= D. Het gio (file luu con it giay) ================= */
    logp("\n=== D. HET GIO ===\n");
    const char *empty[12], *one[12];
    for (int i = 0; i < 12; i++) { empty[i] = "0 0 0 0 0 0 0 0 0 0 0 0"; one[i] = empty[i]; }
    one[6] = "0 0 0 0 0 1 0 0 0 0 0 0";
    writeFile("hetgio.caro", "CARO-SAVE 1\nsize 12\np1 An\np2 Bình\nwins 0 0\ndraws 0\nelapsed 0\ncursor 0 0\nmode 0\nclock 300 3000 300000\nboard\n", empty);
    writeFile("mayhetgio.caro", "CARO-SAVE 1\nsize 12\np1 An\np2 Máy (Khó)\nwins 0 0\ndraws 0\nelapsed 0\ncursor 6 5\nmode 3\nclock 300 300000 100\nboard\n", one);
    down(1); enter();
    check("Tai van: danh sach co hetgio.caro, mayhetgio.caro, van1.caro",
          waitFor("TẢI VÁN", 2000) && onScreen("File đã lưu:") && onScreen("hetgio.caro") && onScreen("mayhetgio.caro")
          && onScreen("van1.caro"));
    text("hetgio"); enter();
    check("Tai van X con 3 giay: hien 00:03, mau do", waitFor("Còn lại: 00:0", 2000));
    readScreen();
    check("Dong ho X duoi 30 giay -> mau do", (attr[8][60] & 0x0f) == 12);
    check("X het gio -> 'HẾT GIỜ! CHÚC MỪNG Bình (O)'", waitFor("HẾT GIỜ! CHÚC MỪNG Bình (O) ĐÃ CHIẾN THẮNG", 6000));
    check("Sau khi het gio: dong ho X dung o 00:00", waitFor("Còn lại: 00:00", 2000));
    dump("X het gio");
    waitFor("Về menu!", 3000);
    key('n');
    check("Ve menu sau khi het gio", waitFor("C A R O", 2000));

    down(1); enter(); waitFor("TẢI VÁN", 2000);
    text("mayhetgio"); enter();
    check("May (O) con 0,1 giay -> may het gio, nguoi thang",
          waitFor("HẾT GIỜ! CHÚC MỪNG An ĐÃ THẮNG Máy (Khó)", 5000));
    check("May khong kip danh quan nao", countMarks('O') == 0);
    waitFor("Về menu!", 3000);
    key('n'); waitFor("C A R O", 2000);

    /* ================= E. Lich su, Cai dat, Thoat ================= */
    logp("\n=== E. LICH SU, CAI DAT, THOAT ===\n");
    down(2); enter();
    check("Lich su: 3 van (1 thang thuong + 2 het gio)", waitFor("LỊCH SỬ ĐẤU", 2000) && onScreen("Tổng: 3 ván"));
    check("Lich su ghi ten co dau 'Máy (Khó)', 'Bình', ket qua 'X thắng' / 'O thắng'", onScreen("Máy (Khó)")
          && onScreen("Bình") && onScreen("X thắng") && onScreen("O thắng"));
    {
        /* cot "Kết quả" phai thang hang du ten co dau hay khong: tim vi tri (theo KY TU) tren tung dong */
        readScreen();
        int col[3], k = 0;
        for (int y = 5; y < 8 && k < 3; y++) {
            for (int x = 0; x + 7 <= W; x++)
                if (wcsncmp(&wscr[y][x], L"X th\x1EAFng", 7) == 0 || wcsncmp(&wscr[y][x], L"O th\x1EAFng", 7) == 0) {
                    col[k++] = x; break;
                }
        }
        check("Lich su: cot ket qua thang hang tren ca 3 dong", k == 3 && col[0] == col[1] && col[1] == col[2]);
    }
    dump("lich su");
    esc(); waitFor("C A R O", 2000);
    down(3); enter();
    check("Cai dat hien am thanh TẮT + ten An / Bình (dat ten khi thi dau khong doi Cai dat)", waitFor("CÀI ĐẶT", 2000)
          && onScreen("Âm thanh         : TẮT") && onScreen("Tên người chơi 1 : An") && onScreen("Tên người chơi 2 : Bình"));
    {
        char buf[500]; readAll("caro_settings.cfg", buf, sizeof buf);
        check("File cai dat giu nguyen ten co dau (UTF-8)", strstr(buf, "name2=Bình") != NULL);
    }
    esc(); waitFor("C A R O", 2000);
    {
        char buf[500]; readAll("caro_settings.cfg", buf, sizeof buf);
        check("Thoat Cai dat: file cai dat ghi lai van giu ten 'Bình'", strstr(buf, "name2=Bình\n") != NULL);
    }
    down(4); enter();
    DWORD w = WaitForSingleObject(pi.hProcess, 4000);
    DWORD code = 99; GetExitCodeProcess(pi.hProcess, &code);
    check("Thoat: chuong trinh thoat, ma 0", w == WAIT_OBJECT_0 && code == 0);
    if (w != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 1);

    logp("\nTONG KET: %d dung, %d loi\n", pass, fail);
    fclose(logf);
    return fail ? 1 : 0;
}
