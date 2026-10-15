# 03 — Historique des blocages (symptôme -> cause réelle -> correction)

| # | Version livrée | Retour de l'utilisateur | Cause | Correction |
|---|---|---|---|---|
| 1 | 1re DLL 32 bits : libstdc++ liée en statique, `-msse2`, `new`/`thread_local` | « kernel erreur » | La DLL importait **`GetThreadId`** (Windows Vista+) via libstdc++ ; absente de XP. Aussi : instructions SSE2. | Plus aucune dépendance C++ ; x87 pur |
| 2 | sans `GetThreadId`, mais SSE2 | « désactivé au démarrage » | SSE2 (CPU ancien ?) et runtime MinGW | `-march=i586 -mfpmath=387 -mno-sse` |
| 3 | x87 pur, runtime MinGW + libstdc++ | « pas chargé » | Section **TLS**, code de démarrage MinGW, symboles DWARF (risques réels sous XP, mais pas la cause finale) | `-nostdlib`, `-s`, DllMain maison |
| 4 | **sans aucun runtime** (11 Ko, 1 import) | « marche toujours pas » | (pas la cause) | — |
| 5 | comportements copiés d'un plugin SynthEdit : `main` appelle `audioMasterVersion`, `ioRatio=1`, `canMono`, `effCanBeAutomated=1`, export `main` seul, format PE identique | « marche toujours pas » | (pas la cause) | — |
| 6 | **version avec journal** + éditeur | journal reçu | Nuendo charge bien la DLL, `main()` réussit, puis **opcode 22 (`effIdentify`) -> on répondait 0** -> déchargement silencieux | répondre **`'NvEf'` (0x4e764566)** |
| 7 | + `effIdentify` | « cette fois c'est bon ! » | — | — |

## Ce qu'il faut en retenir
- Les corrections 1 à 5 étaient de vrais durcissements (surtout `GetThreadId`, qui aurait suffi à empêcher le chargement sous
  XP) et restent obligatoires ; mais **la cause qui bloquait dans Nuendo 2 était `effIdentify`**.
- **Les hôtes modernes (et mes hôtes de test) ne posent pas `effIdentify`** : aucun test sous Wine ne pouvait le révéler.
  Seule la comparaison avec un plugin qui marche (il répond `'NvEf'`) puis le **journal d'une vraie session** l'ont trouvé.
- Erreur de méthode à éviter : empiler des hypothèses. Dès le 2e échec, livrer une version qui **journalise** et comparer
  avec un plugin qui marche.
- Autres pièges rencontrés : Nuendo peut mémoriser un plugin comme défaillant -> **changer nom de fichier, nom d'effet et
  `uniqueID`** à chaque essai ; retirer les anciennes versions du dossier ; ne pas mettre version normale et `_log` ensemble.
