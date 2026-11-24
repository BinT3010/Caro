#pragma once
#include "model.h"
#include "storage.h"
/* ===================== VIEW: moi thu lien quan den man hinh =====================
 * Day la noi nhom chinh sua giao dien. Control chi goi cac ham duoi day va
 * KHONG in gi truc tiep, nen doi mau/bo cuc o day khong anh huong logic.
 *   - Giu nguyen tu code cu: goToXY, setColor, drawBoard, fixConsole,
 *     resizeConsole, printChessChar, winPlayer, endMenu, endMenuBorder,
 *     currPosHighlight.
 *   - Cac man hinh moi (HUD, menu chinh, cai dat, lich su) o muc TOI GIAN,
 *     chi du de dung duoc chuc nang.
 *   - O dang chon tren ban co duoc to nen sang (placeCursor -> currPosHighlight).
 */

#define SCREEN_W 90
#define SCREEN_H (TOP + 2 * BOARD_SIZE + 5)
#define MSG_Y (TOP + 2 * BOARD_SIZE + 1)     /* dong thong bao duoi ban co */
#define PROMPT_Y (MSG_Y + 2)                 /* dong nhap lieu */
#define PANEL_X (LEFT + 4 * BOARD_SIZE + 4)  /* cot bat dau bang thong so ben phai */
#define HUD_W 34                             /* chieu rong bang thong so (PANEL_X + HUD_W < SCREEN_W) */
#define HELP_Y (TOP + 16)                    /* 4 dong huong dan phim trong bang thong so */
#define HISTORY_ROWS 18                      /* so dong lich su moi trang */
#define SETTINGS_ITEMS 6

/* Danh sach file da luu: tieu de + SAVE_LIST_ROWS ten + 1 dong "con N file" */
#define SAVE_LIST_ROWS 8
#define SAVE_LIST_W 30
/* Trong van choi: danh sach file hien tam o cot ben phai (de len phan huong dan phim),
 * tu dong TOP+15 den MSG_Y-1, KHONG de len dong thong bao / dong nhap lieu. */
#define GAME_LIST_Y (TOP + 15)

/* ---- co ban ---- */
void goToXY(int x, int y);
void setColor(int textColor, int bgColor);
void clearScreen(void);
void showCursor(int visible);
void fixConsole(void);
void resizeConsole(int width, int height);

/* ---- man hinh choi ---- */
void drawBoard(void);                 /* khung ban co */
void drawMarks(void);                 /* ve lai tat ca quan (sau khi tai van) */
void drawGameScreen(void);            /* xoa man hinh + ban co + quan + bang thong so */
void placeCursor(void);               /* dua con tro ve o hien hanh */
void drawHud(void);                   /* bang thong so 2 nguoi choi (muc 4.4) */
void drawTime(void);                  /* cap nhat cac dong thoi gian (dong ho 2 ben + tong) */
long nowMs(void);                     /* mili giay tu luc chuong trinh chay (cho dong ho) */
void showMessage(const char *msg, int color);
void clearMessage(void);
void printChessChar(int turn);        /* in X/O tai con tro */
void eraseCell(int r, int c);         /* xoa quan tai o (r, c) - dung khi di lai */
void winPlayer(int res);              /* dong chu thang/thua/hoa */
void winEffect(int res);              /* hieu ung thang/hoa (muc 4.3) */
void setTimeoutFinish(int on);        /* 1 = van ket thuc vi het gio (dong thong bao ghi "HET GIO!") */
void endMenu(int retryText, int retryBg, int exitText, int exitBg);
void endMenuBorder(int row);
void currPosHighlight(int mark);

/* ---- menu & man hinh phu ---- */
void drawMenu(const char *title, const char *const items[], int n, int sel);
void drawSettings(int sel);
void drawHistory(int offset);
void drawInputField(int x, int y, const char *label, const char *text);
void clearInputField(int x, int y);
void clearRegion(int x, int y, int w, int h);
void drawSaveList(int x, int y, FileName *names, int n);
void clearSaveList(int x, int y);
void drawHelpLine(int y, const char *text);