# Do an CARO - Co so lap trinh (C, console Windows)

Viet bang **C thuan (C99)**, mo hinh Model - View - Control, chay tren Windows console.

## Cach bien dich va chay

**Quan trong:** phai chay trong cua so console that (cmd / Windows Terminal / PowerShell), khong chay trong o "Debug Console" cua VS Code.
Chuong trinh can lien ket thu vien `winmm` (am thanh).

`caro.exe` **khong** nam trong repo (da khai bao trong `.gitignore`), moi thanh vien tu bien dich tren may minh.

### VS Code
1. Cai MinGW-w64 (gcc, gdb) va extension *C/C++*.
2. Mo thu muc du an -> `Ctrl+Shift+B` (task "build caro", cau hinh trong `.vscode/tasks.json`) -> mo terminal, go `.\caro.exe`.
   Hoac bam F5 (cau hinh san trong `.vscode/launch.json`: tu build roi mo cua so console rieng).
3. Cach khac: cai extension *CMake Tools*, chon kit va bam Build/Run (da co `CMakeLists.txt`).
4. Khong dung VS Code: chay `build.bat` (MinGW) trong cmd.

### Visual Studio
- **Cach 1 (de nhat):** `File > Open > Folder...` chon thu muc du an (co `CMakeLists.txt`) -> chon target `caro.exe` -> `Ctrl+F5`.
- **Cach 2:** tao *Empty Project* (C++), them tat ca file `.c` / `.h` vao, bam chuot phai du an > Properties >
  *C/C++ > Advanced > Compile As* = **Compile as C Code (/TC)**; thu vien `winmm.lib` da duoc khai bao trong `sound.c`
  (`#pragma comment`). Sau do `Ctrl+F5`.
- **Cach 3:** mo *Developer Command Prompt*, chay `build_msvc.bat`.
- Luu y: khi chay tu Visual Studio, thu muc lam viec la thu muc du an, nen cac file luu van / lich su / cai dat nam o day.

Neu dung Windows Terminal moi (Win 11), viec tu dong doi kich thuoc cua so co the khong co tac dung; hay chay trong `cmd.exe` (conhost) hoac keo to cua so thu cong.

## Cach choi
| Phim | Tac dung |
|---|---|
| W / A / S / D (hoac mui ten) | Di chuyen (o dang chon duoc to nen sang) |
| Enter | Danh X / O (X di truoc) |
| L | Luu van (nhap ten file, luon luu thanh `ten.caro`; khong cho dung ten CON, NUL, COM1...) |
| T | Tai van (nhap ten file; co danh sach file da luu) |
| U | Di lai - **chi khi choi voi may** (xoa nuoc cua may + nuoc cua ban, danh lai) |
| Esc | Thoat ve menu (hoi co luu khong) |

Thang khi co **5 quan lien tiep** (ngang, doc, cheo), 6 quan tro len cung tinh. Ban co day ma khong ai thang -> hoa.
Cuoi van: chon *Choi lai* (Y) hoac *Ve menu* (N/Esc). Diem so cua 2 nguoi duoc giu qua cac van lien tiep.

### Gioi han thoi gian (kieu dong ho co vua)
*New Game* -> chon che do choi -> chon **5 phut**, **10 phut** hoac **15 phut** (Quay lai/Esc: chon lai che do).
- Moi ben (nguoi choi hoac may) co **tong** so phut do cho **ca van**. Dong ho cua ben nao chi chay trong
  luot cua ben do, **khong** duoc cong them sau moi nuoc. Ben nao het gio truoc thi **thua**
  (dong thong bao ghi "HET GIO!"), ket qua duoc ghi vao diem va lich su nhu thang/thua binh thuong.
- Ap dung cho ca che do 2 nguoi va choi voi may; dong ho cua may chay ca trong luc may "suy nghi".
- Bang thong so hien "Gio moi ben", thoi gian con lai cua tung ben (ben dang toi luot co dau `<`,
  con duoi 30 giay chuyen mau do) va tong thoi gian van.
- Dong ho tam dung khi luu/tai van (L/T), khi hoi thoat (Esc) va khi van ket thuc.
  Choi lai giu nguyen muc thoi gian, ca 2 ben day gio lai.
- File luu ghi thoi gian con lai cua 2 ben (`clock <gioi han> <ms X> <ms O>`); file cu chua co dong nay
  duoc tai voi 10 phut moi ben.
- Di lai (U) khong tra lai thoi gian da dung.

### Choi voi may
*New Game* -> chon **2 nguoi choi**, **Choi voi may - De**, **- Trung binh** hoac **- Kho**.
Nguoi choi cam **X** (di truoc), may cam **O**. Sau moi nuoc cua nguoi, may "suy nghi" khoang 0,35 giay
(muc Kho doi khi lau hon, toi da khoang 1 giay) roi danh, con tro nhay toi o may vua danh.
Ca 3 muc deu thang ngay neu co the va chan khi nguoi choi sap co 5.
- **De**: phong thu yeu, chon ngau nhien 1 trong 3 o kha nhat -> nguoi moi choi van thang duoc.
- **Trung binh**: luon chon o tot nhat (tan cong + phong thu) nhung chi nhin 1 nuoc, khong tinh truoc.
- **Kho**: tim chuoi "danh 4 lien tuc" (doi thu buoc phai chan) dan toi thang; neu khong co thi
  nhin truoc 4 nuoc (ca 2 ben) bang tim kiem negamax + cat tia alpha-beta.
Do manh (may dau voi may, 20 van, 4 nuoc dau ngau nhien): Trung binh thang De 19/20; Kho thang De 20/20;
Kho gap Trung binh: thang 13, thua 2, hoa 5.
Che do choi duoc ghi vao file luu (`mode 0/1/2/3`) nen tai lai van se tiep tuc dau voi may;
file luu cu (chua co dong `mode`) duoc tai o che do 2 nguoi.
Lich su ghi ten may la `May (De)` / `May (Trung binh)` / `May (Kho)`.

**Di lai (phim U)** - chi co khi choi voi may, che do 2 nguoi bam U se bao "Chi duoc di lai khi choi voi may!".
Moi lan bam U xoa nuoc vua roi cua may **va** nuoc cua ban ngay truoc do, con tro ve dung o ban vua danh
de ban danh lai. Bam nhieu lan de lui nhieu nuoc; khong gioi han so lan, bang thong so hien so lan da dung.
Lich su nuoc di bat dau lai khi sang van moi hoac khi tai van tu file (chi lui duoc cac nuoc danh sau khi tai).

## Doi chieu voi yeu cau de bai
| Muc | Noi dung | Nam o dau |
|---|---|---|
| 4.1 | Luu/tai bang phim L/T, nhap ten file | `control.c` (promptSave/promptLoad), `storage.c` (saveGame/loadGame) |
| 4.2 | Nhan biet thang/thua/hoa sau moi nuoc di | `model.c` (checkWin, testBoard) |
| 4.3 | Hieu ung thang/hoa: day quan thang nhap nhay, dong chuc mung doi mau, nhac thang/hoa | `view.c` (winEffect), `sound.c` |
| 4.4 | Bang thong so: ten, luot, so nuoc di, thang/thua/hoa, so van, thoi gian | `view.c` (drawHud) |
| 4.5 | Menu chinh New Game / Load Game / History / Settings / Exit | `control.c` (mainMenu), `view.c` (drawMenu) |
| Them | Am thanh chinh duoc (bat/tat + am luong 0-100%, nghe thu) | `sound.c`, man hinh Settings |
| Them | Luu lich su dau (ngay gio, 2 nguoi, ket qua, so nuoc, thoi gian), xem/xoa trong menu History | `storage.c` -> `caro_history.txt` |
| Them | Doi ten nguoi choi (luu lai) | Settings -> `caro_settings.cfg` |
| Them | Choi voi may (AI) 3 muc De / Trung binh / Kho | `ai.c` (aiChooseMove), `control.c` (newGameMenu, aiMove) |
| Them | Di lai (phim U), chi khi choi voi may | `model.c` (undoMove), `control.c` (undoVsAi) |
| Them | Gioi han thoi gian 5/10/15 phut moi ben (kieu co vua), het gio la thua | `model.c` (turnClock...), `control.c` (timeMenu, readKey), `view.c` (drawTime) |

## Cau truc ma nguon
- `model.c/h`: du lieu ban co + luat choi (khong dung Windows nen test duoc).
- `storage.c/h`: doc/ghi file (van da luu, lich su, cai dat).
- `sound.c/h`: tu tong hop am thanh (song sin) -> WAV trong bo nho -> `PlaySound`. Am luong chinh bien do song.
- `view.c/h`: **toan bo phan ve man hinh**. Nhom tu thiet ke UI chi can sua file nay.
  Cac ham cu cua nhom (drawBoard, endMenu, endMenuBorder, printChessChar...) duoc giu nguyen; menu chinh, cai dat,
  lich su, bang thong so chi o muc toi gian de dung duoc chuc nang.
- `ai.c/h`: may chon nuoc di (chi doc ban co, khong ve man hinh, khong dung Windows nen test duoc).
  Cham diem moi o trong theo cac "cua so" 5 o (tan cong + phong thu), muc Kho them chuoi nuoc 4 (VCF) va tim kiem 4 nuoc (alpha-beta).
- `control.c/h`, `main.c`: luong game, doc phim, menu.
- `tests/test_logic.c`: kiem thu phan logic (khong bat buoc de nop bai). Chay trong VS Code:
  `Terminal > Run Task... > test logic`, hoac trong cmd:
  `gcc -std=c99 -Wall -Wextra -I. tests/test_logic.c model.c storage.c sound.c ai.c -o test_logic.exe -lwinmm && test_logic.exe`

## Khac voi code cu
- Bo phim h/j/k/l de di chuyen (trung phim L = luu); con WASD va mui ten.
- Sua loi `checkWin` cu (tinh sai khi gan mep ban co); `printChessChar`, `drawBoard` khong con tu reset du lieu.
- Dung hang `BOARD_SIZE`, `LEFT`, `TOP` nhu huong dan.
- Bo tham so mac dinh trong khai bao ham (khong hop le trong C) -> truyen tham so ro rang.

- Dung lai ham `currPosHighlight` de to sang o dang chon (mau quan O da sua cho khop voi ban co).

## File duoc tao ra khi chay
`*.caro` (van da luu), `caro_history.txt` (lich su), `caro_settings.cfg` (cai dat). Co the mo bang Notepad.
Cac file nay rieng cho tung may nen khong dua len git (xem `.gitignore`).
