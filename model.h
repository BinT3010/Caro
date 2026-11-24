#pragma once
/* ===================== MODEL: du lieu + luat choi =====================
 * File nay KHONG dung windows.h / conio.h, chi chua du lieu ban co va luat
 * thang/thua/hoa, nen co the kiem thu doc lap.
 */

/* ---- Hang so (theo huong dan) ---- */
#define BOARD_SIZE 12   /* Kich thuoc ma tran ban co */
#define LEFT 3          /* Toa do trai man hinh ban co */
#define TOP 1           /* Toa do tren man hinh ban co */
#define WIN_LEN 5       /* So quan lien tiep de thang */
#define NAME_LEN 21     /* Ten nguoi choi: toi da 20 ky tu + '\0' */

/* ---- Ket qua van co (TestBoard) ---- */
#define RES_O_WIN (-1)  /* nguoi choi 2 (O) thang */
#define RES_X_WIN 1     /* nguoi choi 1 (X) thang */
#define RES_DRAW 0      /* hoa */
#define RES_NONE 2      /* chua ai thang, van tiep tuc */

/* X, Y: toa do tren man hinh console; mark: 0 = trong, 1 = X, -1 = O */
typedef struct { int X, Y, mark; } point;

/* Thong tin 1 phien choi (nhieu van lien tiep cua cung 2 nguoi) */
typedef struct {
    char name[2][NAME_LEN]; /* [0] = nguoi X, [1] = nguoi O */
    int wins[2];
    int draws;
    int games;              /* so van da choi xong */
} Session;

/* Cai dat (luu ra file) */
typedef struct {
    int soundOn;            /* 1 = bat am thanh */
    int volume;             /* 0..100 */
    char name[2][NAME_LEN];
} Settings;

extern point chessBoard[BOARD_SIZE][BOARD_SIZE];
extern int currX, currY;    /* vi tri con tro: currX = DONG, currY = COT */
extern int turn;            /* 1 = luot X (nguoi 1), -1 = luot O (nguoi 2) */
extern int moveCount[2];    /* so nuoc da di: [0] = X, [1] = O */
extern int totalMoves;
extern int lastX, lastY;    /* o vua danh (-1 neu chua co) */
extern int winCount;        /* so o trong duong thang (0 neu chua thang) */
extern int winX[BOARD_SIZE], winY[BOARD_SIZE]; /* dong/cot cac o thang */
extern Session session;
extern Settings settings;

int  playerIndex(int mark);          /* 1 -> 0, -1 -> 1 */
void resetData(void);                /* ban co moi, giu nguyen session */
void resetSession(void);             /* xoa diem, lay ten tu settings */
void clampCursor(void);              /* giu con tro trong ban co */
int  checkLegalEnter(int t);         /* danh quan t tai con tro; 1 = hop le, 0 = o da co */
/* ---- Di lai (chi dung khi choi voi may) ----
 * Moi nuoc danh qua checkLegalEnter duoc ghi vao lich su cua van. Lich su bi xoa khi
 * bat dau van moi (resetData) va khi tai van tu file (chi di lai duoc cac nuoc sau khi tai). */
extern int undoCount;                /* so lan da di lai trong van nay */
int  moveHistoryLength(void);        /* so nuoc trong lich su nuoc di */
int  lastMoveMark(void);             /* quan cua nuoc cuoi (1/-1), 0 neu chua co */
int  undoMove(int *r, int *c);       /* xoa nuoc cuoi, tra ve o vua xoa; 0 neu khong con nuoc */
int  checkWin(int t);                /* nuoc vua danh co tao 5 lien tiep? tra ve t hoac 0 */
int  boardFull(void);
int  testBoard(void);                /* RES_X_WIN / RES_O_WIN / RES_DRAW / RES_NONE */
void switchTurn(void);
void recordResult(int res);          /* cong diem vao session */

/* Dong ho van co (khong tinh luc dang nhap ten file, luc hien menu ket thuc...) */
void clockReset(void);
void clockStart(void);
void clockStop(void);
int  clockElapsed(void);
void clockSet(int seconds);

/* Nap trang thai tu file da doc. Tra ve 0 neu OK, khac 0 neu du lieu sai luat. */
int  loadState(int marks[BOARD_SIZE][BOARD_SIZE], int cursorRow, int cursorCol,
               const Session *s, int elapsedSeconds);
