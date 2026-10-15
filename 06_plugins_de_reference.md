# 06 — Plugins de référence (qui marchent sur la machine cible)

Fournis par l'utilisateur ; analysés avec `pefile`, `objdump` et l'hôte de test sous Wine. **Les DLL elles-mêmes ne sont pas dans le kit** (elles appartiennent à
leurs auteurs) : redemander à l'utilisateur d'en renvoyer un si une nouvelle comparaison est nécessaire.

## Ce qu'ils ont tous en commun
- Compilés avec **Visual C++ 7.x** (linker 7.10 le 2005-06-20 pour `midFILTER` et `rnddelay`, 7.0 le 2003-03-18 pour `DaTube`), **exportent uniquement `main`**, ImageBase **0x10000000**, FileAlignment **0x1000**, sous-système 4.0,
  caractéristiques 0x210e, ressources présentes ; **tous ont un éditeur graphique** (`flags` avec `hasEditor`).
- Répondent **`'NvEf'` à l'opcode 22** ; `effGetVstVersion` = 2300 ; `effCanBeAutomated` = 1 ; `effClose` renvoie 1.
- `main` appelle d'abord **`audioMaster(0, audioMasterVersion, …)`** et renvoie NULL si l'hôte ne répond pas.
- Fonctions de rappel en cdecl simple. `AEffect.ioRatio` = 1.0, `uniqueID` propre.

## Détail
| Fichier | Nature | Observations (relevées en exécutant la DLL sous Wine) |
|---|---|---|
| `midFILTER.dll` | wrapper **SynthEdit** (`se_vst.dll`), synthé | `uniqueID='md88'` ; flags **0x139** = hasEditor + canMono + canReplacing + programChunks + isSynth ; 0 entrée / 0 sortie à l'init ; 1 paramètre ; catégorie **2** (synthé) ; appelle l'hôte : opcode 1 (version) dans `main`, puis **14 (NeedIdle)** et **6 (WantMidi)** |
| `rnddelay.dll` | même moteur SynthEdit, **effet** | `uniqueID='rddl'` ; flags **0x31** = hasEditor + canReplacing + programChunks ; **1 entrée / 1 sortie** ; 8 paramètres ; catégorie **0** ; appelle l'hôte : opcode 1 puis **14 (NeedIdle)** |
| `DaTube.dll` | effet, **compressé** (sections `Oreloc`, `.neolit`) | `main` appelle l'hôte (opcode 1) puis **renvoie NULL sous Wine** quelle que soit la version annoncée (2, 2200, 2300, 2400) : le packer dépend de choses non émulées -> inexploitable |

Valeurs communes de `AEffect` : `numPrograms=1`, `initialDelay=0`, `ioRatio=1.0`, `version=1`, `uniqueID` = 4 caractères ASCII lisibles.

## Comment l'analyse a été faite (reproductible)
1. `tools/pe_info.py` sur chaque DLL -> même famille de fichiers, un seul export `main`.
2. `host_generic.exe` sous Wine -> champs `AEffect`, réponses aux opcodes.
3. **Comparaison avec nos DLL -> seule différence de comportement non corrigée : `effIdentify`** (voir docs/03).
