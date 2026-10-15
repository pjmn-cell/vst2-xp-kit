# 01 — Contexte

## La machine cible (mesurée, pas supposée)
- **Windows XP** : `GetVersion() = 0x0a280105` dans le journal de Nuendo (build 2600, version 5.1).
- **Hôte : Nuendo 2** — `C:\Program Files\Steinberg\Nuendo 2\Nuendo2.exe`.
- **Dossier de plugins qui fonctionne :** `C:\Program Files\Steinberg\Vstplugins` (là où sont aussi les 3 plugins de référence).
- Nuendo 2 répond **2300** à `audioMasterVersion` (VST 2.3) et envoie **`effIdentify` en tout premier** (voir docs/03).
- Le processeur est probablement ancien : on interdit SSE/MMX par prudence (x87 pur, `-march=i586`).

## Ce que Claude peut et ne peut pas faire
- Travaille dans un sandbox Linux **sans Windows** : cross-compilation MinGW-w64 + exécution des vraies DLL sous **Wine** avec
  des hôtes VST de test maison (`tests/`). On peut donc vérifier le chargement, l'ABI, l'audio, l'éditeur (capture d'écran
  sous Xvfb), les cas limites (FPU strict).
- **Ne peut PAS** : tester dans Nuendo 2 / XP, écouter le résultat. Ce qui n'a pas été vu sur la vraie machine doit être
  présenté comme tel. C'est pourquoi chaque plugin est livré avec une version `_log`.

## L'utilisateur
- Francophone. Préfère des réponses **courtes** (ses préférences de réponse, déjà dans ses réglages : s'en tenir strictement à la
  demande, pas d'étapes ni de fichiers non sollicités, pas de récapitulatif final, pas de formules de politesse en fin de réponse).
- A fourni, au fil de la session : 3 plugins qui marchent chez lui (`midFILTER.dll`, `rnddelay.dll`, `DaTube.dll`), le journal
  de Nuendo qui a donné la clé, et des retours courts (« marche pas », « cette fois c'est bon »).
- Demandes réalisées : distorsion type Big Muff -> compresseur avec side-chain entre deux pistes (sans MIDI) -> version
  simplifiée en deux DLL (Send avec seuil et durée, Receive avec réglages de compression).
- Les fichiers sont livrés dans `/mnt/user-data/outputs/` puis présentés avec l'outil `present_files`.
