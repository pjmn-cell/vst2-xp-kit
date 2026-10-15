// Bus de liaison entre plugins : mémoire partagée NOMMÉE (fonctionne entre DLL différentes, même processus).
// Un émetteur (Sender) y publie le niveau de sa piste ; un récepteur (Receiver) le lit pour comprimer.
#pragma once
#include "mathlite.h"
#ifdef _WIN32
  #include <windows.h>
#endif

namespace bf {

struct Link {                      // une boîte aux lettres par numéro de liaison
    volatile int      ready;       // tampon (0/1) publié en dernier
    volatile unsigned stamp;       // heure (ms) de la dernière publication
    volatile int      n[2];        // nombre de sous-blocs valides
    volatile float    sub[2][256]; // niveau (crête linéaire) par sous-bloc
};
struct Bus { unsigned magic; unsigned version; Link links[4]; };   // mémoire à zéro à la création

static Bus g_localBus;             // repli (test natif, ou si la mémoire partagée est refusée)
#ifdef _WIN32
static HANDLE g_busMap; static Bus* g_bus; static bool g_busTried;
static inline Bus* busGet() {
    if (g_bus) return g_bus;
    if (!g_busTried) {
        g_busTried = true;
        HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(Bus), "LinkCompBus_v1");
        if (h) { void* p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Bus));
                 if (p) { g_busMap = h; g_bus = (Bus*)p; return g_bus; } CloseHandle(h); }
    }
    return g_bus ? g_bus : &g_localBus;
}
static inline void busRelease() {
    if (g_bus) { UnmapViewOfFile(g_bus); g_bus = 0; }
    if (g_busMap) { CloseHandle(g_busMap); g_busMap = 0; }
}
#else
static inline Bus* busGet() { return &g_localBus; }
static inline void busRelease() {}
#endif

// ------------------------------------------------------------------ ÉMETTEUR
// Audio inchangé. Publie la crête par sous-bloc, après un seuil (déclencheur) et un maintien (durée).
class Sender {
public:
    float thresh = -40.0f, holdMs = 50.0f; int linkId = 0;
    void setSampleRate(float sr) { fs = sr < 8000.0f ? 44100.0f : sr; reset(); }
    void reset() { held = 0.0f; age = 0; meterDb = -120.0f; }
    float levelDb() const { return meterDb; }

    void process(Bus* bus, unsigned now, float** in, float** out, int n) {
        if (n <= 0) return;
        Link* lk = &bus->links[linkId & 3];
        float thrLin = (float)mexp((double)thresh * 0.11512925464970229);
        int holdS = (int)(holdMs * 0.001f * fs);
        int subLen = (n + 255) / 256; if (subLen < 32) subLen = 32;
        int nSub = (n + subLen - 1) / subLen, w = 1 - lk->ready;
        float mx = 0.0f;
        for (int s = 0; s < nSub; ++s) {
            int a = s * subLen, b = a + subLen; if (b > n) b = n;
            float pk = 0.0f;
            for (int i = a; i < b; ++i) { float l = mfabs(in[0][i]), r = mfabs(in[1][i]); if (l > pk) pk = l; if (r > pk) pk = r; }
            float gated = pk >= thrLin ? pk : 0.0f;                       // seuil : sous le seuil, rien n'est envoyé
            if (gated >= held) { held = gated; age = 0; }                 // nouveau pic : on le tient
            else { age += b - a; if (age >= holdS) { held = gated; age = 0; } }   // durée écoulée : on retombe
            lk->sub[w][s] = held; if (held > mx) mx = held;
        }
        lk->n[w] = nSub; lk->stamp = now; lk->ready = w;
        meterDb = mx > 1e-6f ? (float)(8.685889638065035 * mlog((double)mx)) : -120.0f;
        for (int c = 0; c < 2; ++c) if (out[c] != in[c]) for (int i = 0; i < n; ++i) out[c][i] = in[c][i];
    }
private:
    float fs = 44100.0f, held = 0.0f, meterDb = -120.0f; int age = 0;
};

// ------------------------------------------------------------------ RÉCEPTEUR
// Compresse l'audio selon le niveau publié par un émetteur ayant le même numéro de liaison.
class Receiver {
public:
    float thresh = -20.0f, ratio = 4.0f, attackMs = 10.0f, releaseMs = 150.0f, makeup = 0.0f; int linkId = 0;
    void setSampleRate(float sr) { fs = sr < 8000.0f ? 44100.0f : sr; reset(); }
    void reset() { grDb = 0.0f; signal_ = false; }
    float reductionDb() const { return grDb; }
    bool  hasSignal() const { return signal_; }

    void process(Bus* bus, unsigned now, float** in, float** out, int n) {
        if (n <= 0) return;
        Link* lk = &bus->links[linkId & 3];
        int rd = lk->ready, nSub = lk->n[rd];
        bool fresh = nSub > 0 && (unsigned)(now - lk->stamp) < 250u;      // données périmées -> ignorées
        signal_ = fresh;
        float aAtk = coef(attackMs), aRel = coef(releaseMs);
        for (int i = 0; i < n; ++i) {
            float det = fresh ? lk->sub[rd][(int)(((long)i * nSub) / n)] : 0.0f;
            float lv  = det > 1e-6f ? (float)(8.685889638065035 * mlog((double)det)) : -120.0f;
            float tgt = gainDb(lv);
            grDb = (tgt < grDb) ? aAtk * grDb + (1.0f - aAtk) * tgt : aRel * grDb + (1.0f - aRel) * tgt;
            if (grDb > -1e-5f) grDb = 0.0f;                                // évite les dénormaux (x87)
            float g = (float)mexp((double)(grDb + makeup) * 0.11512925464970229);
            out[0][i] = in[0][i] * g; out[1][i] = in[1][i] * g;
        }
    }
private:
    float fs = 44100.0f, grDb = 0.0f; bool signal_ = false;
    float coef(float ms) const { return (float)mexp(-1.0 / ((double)ms * 0.001 * (double)fs)); }
    float gainDb(float L) const {                                          // genou doux de 6 dB
        const float W = 6.0f; float d = L - thresh;
        if (2.0f * d < -W) return 0.0f;
        if (2.0f * mfabs(d) <= W) { float x = d + W * 0.5f; return (1.0f / ratio - 1.0f) * x * x / (2.0f * W); }
        return d * (1.0f / ratio - 1.0f);
    }
};

} // namespace bf
