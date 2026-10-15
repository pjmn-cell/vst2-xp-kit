// Base commune des plugins VST2 pour Windows XP / Nuendo 2 (sans runtime C).
// Le plugin doit définir AVANT l'inclusion : BF_NAME, BF_UID, BF_LOGFILE, l'enum de paramètres avec P_COUNT,
// kNames[], kEdNames[], kUnits[], kDefaults[] et la struct Core :
//   void apply(const float* params); void fmt(int i, char* buf); void setSampleRate(float); void reset();
//   void process(float** in, float** out, int n, unsigned now);
//   float meter01() const; void meterText(char* buf) const; const char* meterName() const;
// Optionnel : BF_ON_DETACH() appelé au déchargement de la DLL.
#pragma once
#include "vst2_min.h"
#include "util.h"
#include <new>

#ifdef _WIN32
  #include <windows.h>
  #define BF_EXPORT extern "C"   /* exporté sous le nom "main" via le fichier .def */
  static void* bfAlloc(unsigned n) { return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n); }
  static void  bfFree(void* p)     { HeapFree(GetProcessHeap(), 0, p); }
  static unsigned bfNow()          { return GetTickCount(); }
#else
  #include <stdlib.h>
  #include <time.h>
  #define BF_EXPORT extern "C" __attribute__((visibility("default")))
  static void* bfAlloc(unsigned n) { return calloc(1, n); }
  static void  bfFree(void* p)     { free(p); }
  static unsigned bfNow()          { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (unsigned)(t.tv_sec * 1000 + t.tv_nsec / 1000000); }
#endif
#ifndef BF_ON_DETACH
#define BF_ON_DETACH()
#endif

// ---------- journal de diagnostic (seulement avec -DBF_LOG) ----------
#if defined(_WIN32) && defined(BF_LOG)
struct LB {
    char b[320]; int n;
    LB() { n = 0; s("["); d((long)GetTickCount()); s("] "); }
    LB& s(const char* t) { while (t && *t && n < 300) b[n++] = *t++; return *this; }
    LB& d(long v) { if (v < 0) { if (n < 300) b[n++] = '-'; v = -v; } char t[12]; int k = 0; if (!v) t[k++] = '0'; while (v && k < 11) { t[k++] = char('0' + v % 10); v /= 10; } while (k && n < 300) b[n++] = t[--k]; return *this; }
    LB& x(unsigned long v) { s("0x"); for (int i = 7; i >= 0; --i) { int h = (int)((v >> (i*4)) & 15); if (n < 300) b[n++] = char(h < 10 ? '0' + h : 'a' + h - 10); } return *this; }
    ~LB() {
        b[n++] = '\r'; b[n++] = '\n'; char path[300]; DWORD got; const char* name = BF_LOGFILE;
        HANDLE h = INVALID_HANDLE_VALUE;
        { char root[64]; int k = 0; const char* pre = "C:\\"; while (pre[k]) { root[k] = pre[k]; ++k; } int j = 0; while (name[j] && k < 60) root[k++] = name[j++]; root[k] = 0;
          h = CreateFileA(root, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); }
        if (h == INVALID_HANDLE_VALUE) { DWORD l = GetTempPathA(250, path);
            if (l > 0 && l < 250) { int i = 0; while (name[i]) { path[l + i] = name[i]; ++i; } path[l + i] = 0;
                h = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); } }
        if (h != INVALID_HANDLE_VALUE) { SetFilePointer(h, 0, 0, FILE_END); WriteFile(h, b, (DWORD)n, &got, 0); CloseHandle(h); }
    }
};
#else
struct LB { LB& s(const char*) { return *this; } LB& d(long) { return *this; } LB& x(unsigned long) { return *this; } };
#endif

#ifdef BF_NOCRT
extern "C" {
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void* memset(void* d, int v, __SIZE_TYPE__ n) { unsigned char* p = (unsigned char*)d; while (n--) *p++ = (unsigned char)v; return d; }
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void* memcpy(void* d, const void* s, __SIZE_TYPE__ n) { unsigned char* p = (unsigned char*)d; const unsigned char* q = (const unsigned char*)s; while (n--) *p++ = *q++; return d; }
}
#ifdef _WIN32
static HINSTANCE g_hinst;
extern "C" BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    g_hinst = h;
#ifdef BF_LOG
    if (reason == 1) { char dll[260], host[260]; dll[0] = host[0] = 0; GetModuleFileNameA(h, dll, 259); GetModuleFileNameA(0, host, 259);
        LB().s("DLL_PROCESS_ATTACH dll=").s(dll); LB().s("   hote=").s(host).s(" winver=").x(GetVersion()); }
    else if (reason == 0) LB().s("DLL_PROCESS_DETACH");
#endif
    if (reason == 0) { BF_ON_DETACH(); }
    return TRUE;
}
#endif
#endif

// ---------------------------------- plugin ----------------------------------
struct Plugin {
    AEffect fx;
    audioMasterCallback master;
#ifdef BF_EDITOR
    HWND hwnd; int dragRow;
#endif
    Core core;
    float params[P_COUNT];
    float tmp[2][4096];
    void apply() { core.apply(params); }
};

// ---------------- éditeur graphique minimal (GDI, double tampon) ----------------
#ifdef BF_EDITOR
static void notifyHost(Plugin* p, int i) { if (p->master) p->master(&p->fx, 0 /*audioMasterAutomate*/, i, 0, 0, p->params[i]); }
static const int ROW_H = 30, ED_W = 300, ED_H = 6 + (P_COUNT + 1) * ROW_H + 6, SL_L = 15, SL_R = 285;
static ERect g_edRect = { 0, 0, (short)ED_H, (short)ED_W };
static int rowTop(int i) { return 6 + i * ROW_H; }
static void edSet(Plugin* p, int i, int x) {
    float v = (float)(x - SL_L) / (float)(SL_R - SL_L); if (v < 0) v = 0; if (v > 1) v = 1;
    p->params[i] = v; p->apply(); notifyHost(p, i);
}
static void edPaint(HWND h, Plugin* p) {
    PAINTSTRUCT ps; HDC wdc = BeginPaint(h, &ps);
    HDC dc = CreateCompatibleDC(wdc); HBITMAP bmp = CreateCompatibleBitmap(wdc, ED_W, ED_H); HGDIOBJ old = SelectObject(dc, bmp);
    HBRUSH bg = CreateSolidBrush(RGB(36, 36, 40)), tr = CreateSolidBrush(RGB(70, 70, 76));
    HBRUSH or_ = CreateSolidBrush(RGB(230, 120, 30)), cy = CreateSolidBrush(RGB(70, 200, 200));
    RECT r = { 0, 0, ED_W, ED_H }; FillRect(dc, &r, bg);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(235, 235, 235));
    for (int i = 0; i < P_COUNT; ++i) {
        int t = rowTop(i); char buf[40], val[56]; p->core.fmt(i, buf);
        int k = 0; for (int j = 0; buf[j] && k < 40; ++j) val[k++] = buf[j];
        if (kUnits[i][0]) { val[k++] = ' '; for (int j = 0; kUnits[i][j]; ++j) val[k++] = kUnits[i][j]; }
        val[k] = 0;
        TextOutA(dc, SL_L, t, kEdNames[i], slen(kEdNames[i]) > 24 ? 24 : slen(kEdNames[i]));
        TextOutA(dc, SL_R - 8 * k, t, val, k);
        RECT tk = { SL_L, t + 16, SL_R, t + 24 }; FillRect(dc, &tk, tr);
        RECT fi = { SL_L, t + 16, SL_L + (int)(p->params[i] * (SL_R - SL_L)), t + 24 }; FillRect(dc, &fi, or_);
    }
    {   // afficheur (lecture seule) : niveau envoyé / réduction de gain
        int t = rowTop(P_COUNT); char mt[48]; mt[0] = 0; p->core.meterText(mt);
        const char* nm = p->core.meterName(); TextOutA(dc, SL_L, t, nm, slen(nm));
        TextOutA(dc, SL_R - 8 * slen(mt), t, mt, slen(mt));
        RECT tk = { SL_L, t + 16, SL_R, t + 24 }; FillRect(dc, &tk, tr);
        float m = p->core.meter01(); if (m < 0) m = 0; if (m > 1) m = 1;
        RECT fi = { SL_L, t + 16, SL_L + (int)(m * (SL_R - SL_L)), t + 24 }; FillRect(dc, &fi, cy);
    }
    BitBlt(wdc, 0, 0, ED_W, ED_H, dc, 0, 0, SRCCOPY);
    DeleteObject(bg); DeleteObject(tr); DeleteObject(or_); DeleteObject(cy);
    SelectObject(dc, old); DeleteObject(bmp); DeleteDC(dc);
    EndPaint(h, &ps);
}
static LRESULT CALLBACK edProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    Plugin* p = (Plugin*)GetWindowLongA(h, GWL_USERDATA);
    switch (m) {
    case WM_PAINT: if (p) edPaint(h, p); else { PAINTSTRUCT ps; BeginPaint(h, &ps); EndPaint(h, &ps); } return 0;
    case WM_ERASEBKGND: return 1;
    case WM_TIMER: InvalidateRect(h, 0, FALSE); return 0;          // rafraîchit l'afficheur
    case WM_LBUTTONDOWN:
        if (p) { int y = (short)HIWORD(l), x = (short)LOWORD(l); int row = (y - 4) / ROW_H;
                 if (row >= 0 && row < P_COUNT) { p->dragRow = row; SetCapture(h); edSet(p, row, x); InvalidateRect(h, 0, FALSE); } }
        return 0;
    case WM_MOUSEMOVE: if (p && GetCapture() == h) { edSet(p, p->dragRow, (short)LOWORD(l)); InvalidateRect(h, 0, FALSE); } return 0;
    case WM_LBUTTONUP: if (GetCapture() == h) ReleaseCapture(); return 0;
    }
    return DefWindowProcA(h, m, w, l);
}
static intptr_t edOpen(Plugin* p, void* parent) {
    WNDCLASSA wc; memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = edProc; wc.hInstance = g_hinst; wc.hCursor = LoadCursorA(0, IDC_ARROW); wc.lpszClassName = BF_NAME "Editor";
    RegisterClassA(&wc);
    HWND w = CreateWindowExA(0, BF_NAME "Editor", "", WS_CHILD | WS_VISIBLE, 0, 0, ED_W, ED_H, (HWND)parent, 0, g_hinst, 0);
    if (!w) return 0;
    SetWindowLongA(w, GWL_USERDATA, (LONG)p); p->hwnd = w; SetTimer(w, 1, 60, 0); return 1;
}
static void edClose(Plugin* p) { if (p->hwnd) { KillTimer(p->hwnd, 1); DestroyWindow(p->hwnd); p->hwnd = 0; } }
#endif

// ---------------------------------- audio ----------------------------------
static void processReplacing(AEffect* e, float** in, float** out, int32_t n) {
    Plugin* p = (Plugin*)e->object; p->core.process(in, out, n, bfNow());
}
static void processLegacy(AEffect* e, float** in, float** out, int32_t n) {   // ancien mode "accumulating"
    Plugin* p = (Plugin*)e->object;
    for (int32_t done = 0; done < n; ) {
        int32_t b = n - done; if (b > 4096) b = 4096;
        float* i2[2] = { in[0] + done, in[1] + done }; float* o2[2] = { p->tmp[0], p->tmp[1] };
        p->core.process(i2, o2, b, bfNow());
        for (int c = 0; c < 2; ++c) for (int i = 0; i < b; ++i) out[c][done + i] += p->tmp[c][i];
        done += b;
    }
}
static void setParameter(AEffect* e, int32_t i, float v) {
    Plugin* p = (Plugin*)e->object;
    if (i < 0 || i >= P_COUNT) return;
    p->params[i] = v < 0 ? 0 : (v > 1 ? 1 : v); p->apply();
}
static float getParameter(AEffect* e, int32_t i) { Plugin* p = (Plugin*)e->object; return (i >= 0 && i < P_COUNT) ? p->params[i] : 0.0f; }

static intptr_t dispatcherImpl(AEffect* e, int32_t op, int32_t idx, intptr_t val, void* ptr, float opt) {
    Plugin* p = (Plugin*)e->object; char buf[40];
    switch (op) {
    case effClose:
#ifdef BF_EDITOR
        edClose(p);
#endif
        p->~Plugin(); bfFree(p); return 1;
#ifdef BF_EDITOR
    case effEditGetRect: *(ERect**)ptr = &g_edRect; return 1;
    case effEditOpen:  return edOpen(p, ptr);
    case effEditClose: edClose(p); return 1;
    case effEditIdle:  return 0;
#endif
    case effIdentify: return kIdentifyNvEf;            // "NvEf" : exigé par Nuendo 2
    case 26: /* effCanBeAutomated */ return (idx >= 0 && idx < P_COUNT) ? 1 : 0;
    case effGetProgram: return 0;
    case effGetProgramName: put(ptr, "Default", 24); return 0;
    case effGetParamName:  if (idx >= 0 && idx < P_COUNT) put(ptr, kNames[idx], 9); return 0;
    case effGetParamLabel: put(ptr, (idx >= 0 && idx < P_COUNT) ? kUnits[idx] : "", 9); return 0;
    case effGetParamDisplay: buf[0] = 0; if (idx >= 0 && idx < P_COUNT) p->core.fmt(idx, buf); put(ptr, buf, 9); return 0;
    case effSetSampleRate: p->core.setSampleRate(opt); p->apply(); return 0;
    case effMainsChanged:  if (val) p->core.reset(); return 0;
    case effGetPlugCategory: return kPlugCategEffect;
    case effGetEffectName:    put(ptr, BF_NAME, 32); return 1;
    case effGetVendorString:  put(ptr, "Perso", 64); return 1;
    case effGetProductString: put(ptr, BF_NAME, 64); return 1;
    case effGetVendorVersion: return 1000;
    case effGetVstVersion:    return 2300;
    default: return 0;
    }
}
static intptr_t dispatcher(AEffect* e, int32_t op, int32_t idx, intptr_t val, void* ptr, float opt) {
    LB().s("  dispatcher  op=").d(op).s(" idx=").d(idx).s(" val=").d((long)val).s(" ...");
    intptr_t r = dispatcherImpl(e, op, idx, val, ptr, opt);
    LB().s("  dispatcher  op=").d(op).s(" -> ").d((long)r);
    return r;
}

BF_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    LB().s("main() appele, callback hote=").x((unsigned long)(void*)audioMaster);
    if (audioMaster) {                                  // comme le SDK Steinberg : on interroge l'hôte d'abord
        intptr_t v = audioMaster(0, 1, 0, 0, 0, 0);
        LB().s("audioMasterVersion -> ").d((long)v);
        if (v == 0) return 0;
    }
    Plugin* p = (Plugin*)bfAlloc(sizeof(Plugin));
    if (!p) return 0;
    new (&p->core) Core();
    p->master = audioMaster;
    for (int i = 0; i < P_COUNT; ++i) p->params[i] = kDefaults[i];
    AEffect& fx = p->fx;
    fx.magic = kEffectMagic; fx.dispatcher = dispatcher; fx.process = processLegacy;
    fx.setParameter = setParameter; fx.getParameter = getParameter; fx.processReplacing = processReplacing;
    fx.numPrograms = 1; fx.numParams = P_COUNT; fx.numInputs = 2; fx.numOutputs = 2;
    fx.flags = effFlagsCanReplacing | (1 << 3);          // canReplacing + canMono
#ifdef BF_EDITOR
    fx.flags |= effFlagsHasEditor;
#endif
    fx.ioRatio = 1.0f; fx.uniqueID = BF_UID; fx.version = 1; fx.object = p;
    p->core.setSampleRate(44100.0f); p->apply();
    LB().s("main() OK, AEffect cree");
    return &fx;
}
