# 05 — Créer un nouveau plugin (pas à pas)

## 1. Partir du squelette
    cp -r skeleton monplugin
Dans `monplugin/` :
- `project.conf` : `NAME` (nom affiché), `FILE` (nom du .dll), `PUID` (**uniqueID : 4 caractères en hexadécimal, uniques**, ex. `'Dly1'` = 0x446C7931),
  `LOGNAME` / `LOGUID` (nom et ID de la version journal, **différents**), `DEF`.
- `Skeleton.def` -> `MonPlugin.def` (contenu : `EXPORTS` / `main=VSTPluginMain`). `version.rc` : remplacer les noms.
- `plugin.cpp` : `BF_NAME`, `BF_UID`, `BF_LOGFILE`, puis l'enum, les tableaux et la struct `Core`.
Chaque essai chez l'utilisateur : **nouveau nom de fichier + nom d'effet + uniqueID** (Nuendo peut mémoriser un plugin défaillant).

## 2. Ce que `plugin.cpp` doit définir avant `#include "vstbase.h"`
    enum { P_A, P_B, ..., P_COUNT };                       // paramètres
    static const char* const kNames[P_COUNT]   = {...};    // noms courts (<= 8 caractères) pour l'hôte
    static const char* const kEdNames[P_COUNT] = {...};    // noms affichés dans notre fenêtre
    static const char* const kUnits[P_COUNT]   = {...};    // "dB", "ms", ":1", "" ...
    static const float kDefaults[P_COUNT]      = {...};    // valeurs normalisées 0..1
    struct Core {                                          // le coeur DSP
      void  apply(const float* p);                // p[] = paramètres 0..1 -> valeurs réelles (appelé à chaque changement)
      void  fmt(int i, char* buf) const;          // texte du paramètre i (sans l'unité)
      void  setSampleRate(float sr);  void reset();
      void  process(float** in, float** out, int n, unsigned now_ms);   // in == out possible ; 2 canaux
      const char* meterName() const;  float meter01() const;  void meterText(char* buf) const;   // afficheur en bas de la fenêtre
    };
    // optionnel avant l'include : #define BF_ON_DETACH() ...   (libérer des ressources au déchargement de la DLL)

## 3. Conventions de correspondance paramètre 0..1 -> valeur
| Type | Formule (v = 0..1) | Exemple |
|---|---|---|
| linéaire | `lo + v*(hi-lo)` | seuil : `-60 + v*60` dB |
| logarithmique | `lo * exp(v*ln(hi/lo))` : `0.1f*(float)bf::mexp(v*6.907755)` | attaque 0.1..100 ms |
| plus fin vers le bas | `max*v*v` | hold 0..500 ms, ratio `1+19*v*v` |
| à zones (sélecteur) | `v<0.25 ? 0 : v<0.5 ? 1 : …` | modes |
| entier | `(int)(v*3.999f)` | numéro de lien 1..4 |
Le `fmt()` doit refléter la valeur réelle (après `apply`). Penser à **lisser** les paramètres dans `process` s'ils produisent des clics.

## 4. Compiler, auditer, tester
    ./build.sh monplugin            # -> bin/MonPlugin.dll
    ./build.sh monplugin log        # -> bin/MonPlugin_log.dll   (à livrer aussi)
    python3 tools/audit_dll.py bin/MonPlugin.dll
    ./tools/run_tests.sh            # tout, y compris les autres projets (non-régression)
Ajouter un test dédié si le DSP a un comportement mesurable : copier `tests/host_skeleton.cpp` (règle un paramètre, traite un signal connu,
compare à la valeur attendue, affiche PASS/FAIL), le déclarer dans `tools/run_tests.sh`. Vérifier aussi **visuellement** l'éditeur
(capture sous Xvfb, voir docs/04) et les cas limites (silence, blocs de 1 et 8192 échantillons, FPU strict `FPUTRAP=1`).

## 5. Livrer
- `bin/MonPlugin.dll` + un zip contenant `MonPlugin_log.dll`, déposés dans `/mnt/user-data/outputs/` puis `present_files`.
- Dire à l'utilisateur : retirer les anciennes versions du dossier de plugins, copier la nouvelle DLL, redémarrer Nuendo ; en cas d'échec, essayer la
  version `_log` **seule** et renvoyer `C:\<nom>_log.txt`. Dire clairement ce qui a été testé (Wine) et ce qui ne l'a pas été (Nuendo 2).
- Mettre à jour ce kit (README : statut du plugin ; docs si une leçon nouvelle) et reproposer le kit à l'utilisateur.

## 6. Motifs DSP utiles déjà dans le kit
- Biquad passe-bas, filtres à un pôle, **sur-échantillonnage ×2** (anti-aliasing d'une saturation) : `fuzzpi/dsp.h`.
- Détecteur de crête, calcul de gain en dB avec genou doux, lissage attaque/relâchement : `common/bus.h` (`Receiver`).
- Liaison entre pistes / entre DLL : `common/bus.h` (mémoire partagée nommée) ; voir `linksend/` et `linkrecv/`.
- Afficheur (niveau, réduction de gain) : `meter01()` / `meterText()`.
- Pour un **delay/reverb/chorus** : mettre le tampon (tableau `float`) directement dans `Core` (l'instance est allouée à zéro sur le tas), indices modulo.
  Pas de `malloc`. Attention à la mémoire (ex. 2 s à 96 kHz stéréo = 1,5 Mo : acceptable).

## 7. Faisabilité (ce que l'on sait faire / ne pas faire)
| Faisable avec `vstbase.h` | Demande une extension du kit |
|---|---|
| effets stéréo : distorsion, compresseur, EQ, filtres, delay, chorus, tremolo, gate, limiteur | **instruments / MIDI** (flag `isSynth`, `effProcessEvents`) |
| liaison entre pistes (ducking, « side-chain » maison) | **vrai side-chain** (impossible en VST2 sous Nuendo ; voir docs/07) |
| curseurs + afficheur de niveau | interface graphique riche (potards, courbes) : à dessiner en GDI |
| paramètres automatisables | presets (banques, `programChunks`) |
