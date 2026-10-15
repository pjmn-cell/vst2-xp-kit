# 07 — Limites et aspects légaux

## Limites connues
- **Pas de vrai side-chain en VST2 dans Nuendo** (même récent : réservé au VST3 ; il n'a jamais fait partie de la spécification VST2). Contournement
  réalisé : plugins **liés par mémoire partagée nommée** (LinkSend -> LinkRecv). Limites : même machine, même Nuendo, instances des plugins du kit ; un
  bloc de latence possible selon l'ordre de traitement des pistes ; données ignorées si elles ont plus de 250 ms (transport arrêté).
- 32 bits uniquement pour `vstbase.h` (éditeur) ; stéréo 2/2 ; pas de MIDI/instruments ; pas de presets/chunks ; pas de `processDoubleReplacing`.
- Éditeur minimal en GDI (curseurs) ; pas de potards ni de courbes.
- Tout est **testé sous Wine, jamais sur Nuendo 2** sauf FuzzPi (confirmé par l'utilisateur après la correction `effIdentify`).
- Non vérifié : comportement avec plusieurs instances très nombreuses, avec des blocs très grands (> 8192), avec d'autres hôtes que Nuendo 2.

## Légal
- Le **SDK VST2 de Steinberg n'est plus distribué** ; `common/vst2_min.h` est un en-tête **réécrit** (structures et constantes nécessaires à l'ABI).
  Usage personnel : sans souci pratique ; **distribution publique** d'un VST2 : soumise à la licence Steinberg (plus délivrée aux nouveaux développeurs).
- « **Big Muff** » est une marque d'Electro-Harmonix : le plugin s'appelle FuzzPi.
- Les plugins de référence (`midFILTER`, `rnddelay`, `DaTube`) sont la propriété de leurs auteurs ; seuls leurs *comportements observables* sont
  consignés ici (pas de code repris).
