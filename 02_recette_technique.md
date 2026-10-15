# 02 — Recette technique

## 1. Principe d'un plugin VST2 (rappel)
Une DLL Win32 qui exporte **`main`** (convention **cdecl**) : `AEffect* main(audioMasterCallback)`. Elle renvoie un pointeur
vers une structure `AEffect` dont les champs pointent vers 4 fonctions cdecl : `dispatcher`, `process` (ancien, accumulation),
`processReplacing`, `setParameter` / `getParameter`. L'hôte pilote tout via `dispatcher(effect, opcode, index, value, ptr, opt)`.
Les paramètres sont des **floats 0..1** ; le plugin les convertit et fournit leur texte (`effGetParamDisplay`).
Le callback hôte : `audioMaster(effect, opcode, index, value, ptr, opt)` — `0 = audioMasterAutomate` (index = paramètre, `opt` = valeur),
`1 = audioMasterVersion` (Nuendo 2 répond 2300). **`vst2_min.h`** est un en-tête réécrit (le SDK Steinberg n'est pas utilisé).

## 2. Structure `AEffect` (32 bits : 144 octets ; 64 bits : 192)
| champ | offset | champ | offset |
|---|---|---|---|
| magic (`'VstP'` = 0x56737450) | 0 | initialDelay | 48 |
| dispatcher | 4 | realQualities / offQualities | 52 / 56 |
| process (ancien) | 8 | **ioRatio (mettre 1.0)** | 60 |
| setParameter | 12 | object / user | 64 / 68 |
| getParameter | 16 | **uniqueID** (4 caractères) | 72 |
| numPrograms / numParams | 20 / 24 | version | 76 |
| numInputs / numOutputs | 28 / 32 | **processReplacing** | 80 |
| **flags** | 36 | processDoubleReplacing | 84 |
| resvd1 / resvd2 | 40 / 44 | future[56] | 88 |

Flags : `hasEditor=1`, `hasClip=2`, `hasVu=4`, `canMono=8`, `canReplacing=16`, `programChunks=32`, `isSynth=256`.
Valeurs utilisées : `canReplacing | canMono | hasEditor`, 2 entrées / 2 sorties, 1 programme, `ioRatio=1.0`.

## 3. Opcodes gérés (`common/vstbase.h`)
| op | nom | réponse |
|---|---|---|
| 0 | effOpen | 0 |
| 1 | effClose | libère l'instance, renvoie **1** |
| 3 / 5 | getProgram / getProgramName | 0 / « Default » (24 car. max) |
| 6 / 7 / 8 | getParamLabel / Display / Name | chaînes de **8 caractères + NUL** max |
| 10 | effSetSampleRate | float dans `opt` |
| 12 | effMainsChanged | `value=1` : reset de l'état |
| 13 / 14 / 15 / 19 | editGetRect / editOpen / editClose / editIdle | `ERect**` dans `ptr` (**ERect = {top,left,bottom,right}**) ; `ptr` = HWND parent à l'ouverture ; 1 |
| **22** | **effIdentify** | **`'NvEf'` = 0x4e764566 — OBLIGATOIRE pour Nuendo 2** |
| 26 | effCanBeAutomated | 1 si l'index est valide |
| 35 | effGetPlugCategory | 1 (effet) |
| 45 / 47 / 48 | nom d'effet (32 car.) / vendeur (64) / produit (64) | 1 |
| 49 / 58 | vendorVersion / vstVersion | 1000 / **2300** |
| autres | | 0 |

## 4. Options de compilation (`build.sh`) et pourquoi
| Option | Raison |
|---|---|
| `-nostdlib -lkernel32 -luser32 -lgdi32 -lgcc` | **aucun runtime** : plus de `GetThreadId`, de TLS, de msvcrt. `-lgcc` fournit les assistants (décalages 64 bits…) |
| `-Wl,-e,_DllMain@12` | point d'entrée stdcall décoré (32 bits), notre `DllMain` renvoie TRUE |
| `-march=i586 -mtune=generic -mfpmath=387 -mno-sse -mno-mmx` | **x87 pur** : tourne sur tout CPU, pas de SSE2 |
| `-mstackrealign` | certains hôtes appellent avec une pile mal alignée |
| `-fno-tree-loop-distribute-patterns` | sinon GCC transforme nos boucles en appels `memset`/`strlen`, qui n'existent pas sans runtime (`memset`/`memcpy` sont fournis dans `vstbase.h`) |
| `-fno-math-errno` | `sqrtf` devient l'instruction `fsqrt` (pas d'appel libm) |
| `-fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector` | aucun support C++ à l'exécution |
| `-s` | pas de symboles de debug (sections DWARF) |
| `--image-base 0x10000000`, `--file-alignment/--section-alignment 0x1000`, `--subsystem windows:4.0`, `--major-os-version 4`, `--disable-dynamicbase/nxcompat` | même forme de fichier que les plugins VC7 qui marchent |
| `main=VSTPluginMain` dans le `.def` | **export unique `main`** ; ne pas mettre `__declspec(dllexport)` sur la fonction (sinon `VSTPluginMain` serait exporté aussi) |
| `windres version.rc` | ressource VERSIONINFO comme les plugins de référence |

## 5. Règles pour écrire du code (pas de runtime !)
- **Pas de libm** : `bf::mexp`, `bf::mlog`, `bf::msincos`, `bf::msqrt`, `bf::mfabs` (`common/mathlite.h`, précision ~1e-11).
- **Pas de printf/snprintf/std::** : `fmtNum`, `put`, `cat`, `slen` (`common/util.h`).
- **Pas de new/malloc** : `bfAlloc`/`bfFree` (HeapAlloc). L'instance est allouée à zéro sur le tas ; de gros tableaux dans `Core`
  (lignes à retard…) sont donc possibles. Placement `new (&p->core) Core()` est utilisé.
- **Pas d'objet global avec constructeur** (rien ne s'exécute au chargement). Un global « POD » à zéro est OK (ex. `Bus`).
- Pas de `thread_local`, d'exceptions, de RTTI. Tableaux locaux < 4 Ko (sinon `___chkstk`).
- **x87 : dénormaux très lents** -> ajouter `1e-18f` à l'entrée et ramener à 0 les états qui décroissent (voir `Receiver`).
- Calculs internes en `double` pour `exp`/`log`/coefficients de filtres ; échantillons en `float`.
- Tailles de blocs : l'hôte peut donner n'importe quel `n` (512 typique). Ne pas supposer de borne ; `tmp[2][4096]` ne sert que pour `process` (ancien).
- Plugin **stéréo 2 in / 2 out** seulement ; `canMono` déclaré.
- **32 bits seulement** : l'éditeur utilise `SetWindowLongA(GWL_USERDATA)` avec un cast `(LONG)` (tronquerait en 64 bits ; il faudrait `SetWindowLongPtr`).

## 6. Éditeur graphique (`vstbase.h`)
Fenêtre enfant 300 × (6 + (N+1)·30 + 6) pixels : N curseurs (clic/glisse, `audioMasterAutomate` à chaque changement) + 1 afficheur
en lecture seule (rafraîchi par `SetTimer` toutes les 60 ms), dessin en **double tampon** (`BitBlt`). Classe de fenêtre `<BF_NAME>Editor`.
Imports : user32 (fenêtres, timer), gdi32 (brosses, texte, BitBlt). Tous anciens (Windows 95/NT4+).

## 7. Mémoire partagée entre DLL (bus)
Voir `common/bus.h` : `CreateFileMappingA(INVALID_HANDLE_VALUE, …, "LinkCompBus_v1")` + `MapViewOfFile` ; les variables globales ne sont
PAS partagées entre DLL différentes. Protocole double tampon + horodatage (périmé > 250 ms). Changer la structure `Bus` = changer le nom (`_v2`).
