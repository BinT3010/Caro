#pragma once
/* ===================== AI: may tu chon nuoc di =====================
 * Chi doc ban co (chessBoard) trong Model, KHONG ve man hinh, KHONG dung Windows,
 * nen kiem thu doc lap duoc. Control goi aiChooseMove roi tu danh quan.
 *
 * Muc DE  : thang ngay neu duoc, chan nuoc thang ngay cua doi thu,
 *           con lai chon ngau nhien 1 trong vai o "kha tot" -> nguoi choi thang duoc.
 * Muc KHO : nhu tren, them: tao "nuoc doi" (2 cho thang cung luc) neu co,
 *           va KHONG de doi thu tao duoc nuoc doi (vd: chan hang 3 thoang).
 */
#include "model.h"

#define AI_NONE 0       /* 2 nguoi choi */
#define AI_EASY 1
#define AI_HARD 2
#define AI_MARK (-1)    /* may luon cam quan O, nguoi choi cam X va di truoc */

extern int aiLevel;     /* che do cua van dang choi: AI_NONE / AI_EASY / AI_HARD */

const char *aiName(int level);          /* ten hien thi cua may, vd "May (Kho)" */
const char *aiModeText(int level);      /* mo ta che do cho bang thong so */

/* Chon nuoc di cho quan `me` (1 = X, -1 = O) theo muc `level`.
 * Luon tra ve 1 o TRONG (neu ban co con cho). Tra ve 0 neu ban co da day. */
int aiChooseMove(int level, int me, int *row, int *col);
