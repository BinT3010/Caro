#include <string.h>
#include "utf8.h"

int utf8Decode(const char *str, unsigned *cp) {
    const unsigned char *s = (const unsigned char *)str;
    unsigned c = s[0];
    int n;
    if (c == 0) { *cp = 0; return 0; }
    if (c < 0x80) { *cp = c; return 1; }
    if (c >= 0xC2 && c <= 0xDF) { n = 2; c &= 0x1F; }
    else if (c >= 0xE0 && c <= 0xEF) { n = 3; c &= 0x0F; }
    else if (c >= 0xF0 && c <= 0xF4) { n = 4; c &= 0x07; }
    else { *cp = UTF8_BAD; return 1; }
    for (int k = 1; k < n; k++) {
        if ((s[k] & 0xC0) != 0x80) { *cp = UTF8_BAD; return 1; }
        c = (c << 6) | (s[k] & 0x3F);
    }
    /* ma hoa thua (overlong), vung surrogate, vuot qua Unicode -> sai */
    if ((n == 3 && c < 0x800) || (n == 4 && (c < 0x10000 || c > 0x10FFFF)) || (c >= 0xD800 && c <= 0xDFFF)) {
        *cp = UTF8_BAD;
        return 1;
    }
    *cp = c;
    return n;
}

int utf8Encode(unsigned cp, char *out) {
    if (cp < 0x80) { out[0] = (char)cp; return 1; }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

int utf8Width(const char *s) {
    int w = 0;
    unsigned cp;
    for (int n; (n = utf8Decode(s, &cp)) > 0; s += n) w++;
    return w;
}

size_t utf8Fit(char *dst, size_t cap, const char *src, int cols, int pad) {
    size_t len = 0;
    int w = 0;
    unsigned cp;
    if (cap == 0) return 0;
    for (int n; w < cols && (n = utf8Decode(src, &cp)) > 0; src += n, w++) {
        if (len + (size_t)n >= cap) break;
        memcpy(dst + len, src, (size_t)n);
        len += (size_t)n;
    }
    while (pad && w < cols && len + 1 < cap) { dst[len++] = ' '; w++; }
    dst[len] = '\0';
    return len;
}

void utf8PopLast(char *s) {
    size_t n = strlen(s);
    if (n == 0) return;
    n--;
    while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) n--;   /* lui ve byte dau cua ky tu */
    s[n] = '\0';
}

unsigned utf8Lower(unsigned cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return cp + 32;        /* À Á Â Ã È É Ê Ì Í Ò Ó Ô Õ Ù Ú Ý */
    if (cp >= 0x100 && cp <= 0x137 && cp % 2 == 0) return cp + 1;      /* Ă Đ Ĩ ... (cap chan/le) */
    if (cp >= 0x168 && cp <= 0x177 && cp % 2 == 0) return cp + 1;      /* Ũ Ū ... */
    if (cp == 0x1A0 || cp == 0x1AF) return cp + 1;                     /* Ơ Ư */
    if (cp >= 0x1EA0 && cp <= 0x1EF9 && cp % 2 == 0) return cp + 1;    /* Ạ Ả Ấ ... Ỹ */
    return cp;
}

int utf8EqualNoCase(const char *a, const char *b) {
    unsigned ca, cb;
    for (;;) {
        int na = utf8Decode(a, &ca), nb = utf8Decode(b, &cb);
        if (na == 0 || nb == 0) return na == nb;
        if (utf8Lower(ca) != utf8Lower(cb)) return 0;
        a += na;
        b += nb;
    }
}
