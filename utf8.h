#pragma once
#include <stddef.h>   /* size_t */
/* ===================== UTF8: xu ly chuoi tieng Viet co dau =====================
 * Moi chuoi trong chuong trinh (chu tren man hinh, ten nguoi choi, file luu,
 * lich su, cai dat) deu la UTF-8. Mot chu co dau (vd "ệ") chiem 2-3 byte nhung
 * chi chiem 1 o tren man hinh, nen moi phep can le / cat chuoi phai dem theo
 * KY TU chu khong theo byte. Module nay khong phu thuoc Windows.
 */

/* Doc 1 ky tu tai s. Tra ve so byte da doc (0 neu het chuoi), *cp = ma Unicode.
 * Byte sai UTF-8: tra ve 1 va *cp = UTF8_BAD. */
#define UTF8_BAD 0xFFFDu
int utf8Decode(const char *s, unsigned *cp);
/* Ghi ma cp ra out (toi da 4 byte, khong them '\0'). Tra ve so byte. */
int utf8Encode(unsigned cp, char *out);
/* So ky tu (= so o tren man hinh) cua chuoi. */
int utf8Width(const char *s);
/* Chep toi da `cols` ky tu cua src vao dst (dung cap byte, khong cat doi 1 ky tu).
 * pad != 0: them khoang trang cho du `cols` o. Tra ve so byte da ghi. */
size_t utf8Fit(char *dst, size_t cap, const char *src, int cols, int pad);
/* Xoa ky tu cuoi cung (phim Backspace). */
void utf8PopLast(char *s);
/* Chu thuong tuong ung (A-Z, chu Latin co dau, Đ, Ă, Ơ, Ư va cac chu tieng Viet). */
unsigned utf8Lower(unsigned cp);
/* So sanh 2 chuoi khong phan biet hoa/thuong (ca chu co dau: "ÂN" == "ân"). */
int utf8EqualNoCase(const char *a, const char *b);
