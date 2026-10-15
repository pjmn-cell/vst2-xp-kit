// SKELETON : plugin d'exemple minimal (un gain). Point de départ pour un nouvel effet.
// 1) copier ce dossier (ex. cp -r skeleton monplugin)  2) adapter BF_NAME / BF_UID / project.conf / .def / version.rc
// 3) modifier l'enum, les tableaux et la struct Core  4) ./build.sh monplugin   5) ./tools/run_tests.sh
#include "util.h"
#include "mathlite.h"
#ifndef BF_NAME
#define BF_NAME "Skeleton"
#endif
#ifndef BF_UID
#define BF_UID 0x536B6C31 /* 'Skl1' : 4 caractères UNIQUES pour chaque plugin */
#endif
#define BF_LOGFILE "skeleton_log.txt"

// ---- paramètres : toujours des floats 0..1 côté hôte ; on les convertit dans Core::apply() ----
enum { P_GAIN, P_COUNT };
static const char* const kNames[P_COUNT]   = { "Gain" };      // nom court (8 caractères max) vu par l'hôte
static const char* const kEdNames[P_COUNT] = { "Gain" };      // nom affiché dans notre fenêtre
static const char* const kUnits[P_COUNT]   = { "dB" };
static const float kDefaults[P_COUNT]      = { 0.5f };        // 0.5 -> 0 dB

// ---- le coeur DSP : l'interface attendue par vstbase.h ----
struct Core {
    float gainDb = 0.0f, lin = 1.0f, peak = 0.0f;
    void apply(const float* p) { gainDb = -24.0f + p[P_GAIN] * 48.0f; lin = (float)bf::mexp((double)gainDb * 0.11512925464970229); }
    void fmt(int i, char* b) const { b[0] = 0; if (i == P_GAIN) fmtNum(b, gainDb, 1); }   // texte affiché pour le paramètre i
    void setSampleRate(float) {}
    void reset() { peak = 0.0f; }
    void process(float** in, float** out, int n, unsigned /*now*/) {
        float pk = 0.0f;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { float y = in[c][i] * lin; out[c][i] = y; float a = bf::mfabs(y); if (a > pk) pk = a; }
        peak = pk;
    }
    // afficheur en lecture seule en bas de la fenêtre
    const char* meterName() const { return "Output peak"; }
    float meter01() const { return peak > 1e-6f ? (float)(8.685889638065035 * bf::mlog((double)peak) + 60.0) / 60.0f : 0.0f; }
    void meterText(char* b) const { b[0] = 0; if (peak <= 1e-6f) { cat(b, "-- (silence)"); return; } fmtNum(b, (float)(8.685889638065035 * bf::mlog((double)peak)), 1); cat(b, " dB"); }
};
#include "vstbase.h"
