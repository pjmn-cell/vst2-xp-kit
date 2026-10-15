#pragma once
// Petites fonctions sans runtime C (formatage, chaînes).
static inline int slen(const char* s) { int n = 0; while (s[n]) ++n; return n; }
static inline void put(void* dst, const char* s, int max) { char* d = (char*)dst; int i = 0; for (; i < max - 1 && s[i]; ++i) d[i] = s[i]; d[i] = 0; }
static inline void cat(char* buf, const char* s) { int n = slen(buf), i = 0; while (s[i]) { buf[n + i] = s[i]; ++i; } buf[n + i] = 0; }
static inline void fmtNum(char* buf, float v, int dec) {      // dec = 0 ou 1 décimale
    float a = v < 0 ? -v : v;
    long scale = dec ? 10 : 1, q = (long)(a * (float)scale + 0.5f), ip = q / scale, fp = q % scale;
    int i = 0; if (v < 0 && q != 0) buf[i++] = 0x2d;
    char t[12]; int n = 0; if (ip == 0) t[n++] = 0x30;
    while (ip > 0 && n < 11) { t[n++] = char(0x30 + ip % 10); ip /= 10; }
    while (n > 0) buf[i++] = t[--n];
    if (dec) { buf[i++] = 0x2e; buf[i++] = char(0x30 + fp); }
    buf[i] = 0;
}
