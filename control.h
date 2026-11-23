#pragma once
/* ===================== CONTROL: dieu khien luong game =====================
 * Doc phim, goi Model de xu ly luat, goi View de ve, goi Sound de phat tieng.
 */

void moveUp(void);
void moveDown(void);
void moveLeft(void);
void moveRight(void);
void checkLegalMove(void);   /* giu con tro trong ban co va dua con tro man hinh toi o do */
int  end(void);              /* menu cuoi van: 1 = choi lai, 0 = ve menu chinh */
void out(void);              /* don dep man hinh khi thoat */
void mainMenu(void);         /* vong lap menu chinh (New Game / Load / History / Settings / Exit) */
