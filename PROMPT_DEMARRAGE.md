# Texte à coller dans une nouvelle conversation (avec le kit joint)

---
Voici mon kit `vst2-xp-kit` (archive jointe). Décompresse-le, lis `README.md`, `docs/02_recette_technique.md` et
`docs/05_creer_un_nouveau_plugin.md`, puis fais-moi un nouveau plugin VST2 :

**Plugin souhaité :** *(décris l'effet : ex. « delay stéréo avec feedback et filtre », « compresseur à 3 bandes »…)*
**Paramètres voulus :** *(liste, ou « à toi de proposer »)*

Contraintes habituelles (déjà dans le kit) : DLL 32 bits pour Windows XP / Nuendo 2, sans MIDI, avec fenêtre à curseurs,
livrer la DLL **et** la version `_log` (dans un zip), après avoir lancé `./tools/run_tests.sh` sous Wine.
Si je te renvoie un journal ou un plugin qui marche, utilise `docs/04_diagnostic.md`.
Réponds en français, de façon brève (voir mes préférences de réponse).
---

## Notes pour Claude (à lire avant d'agir)
- Aucun souvenir d'une conversation à l'autre : tout est dans ce kit. Commencer par `tools/setup_env.sh` puis `tools/run_tests.sh`
  (confirme que l'environnement fonctionne : « TOUS LES CONTRÔLES PASSENT »).
- Ne PAS repartir de zéro : copier `skeleton/` (ou `linkrecv/` pour un exemple plus complet), adapter l'enum de paramètres, les
  tableaux et la struct `Core` ; le reste (VST, éditeur, journal, `effIdentify`…) est dans `common/vstbase.h`.
- Les livrables sont demandés explicitement par l'utilisateur dans ce contexte (DLL + zip des versions journal).
- Après livraison : demander le retour de l'utilisateur ; s'il échoue, lui faire essayer la version `_log` et lire `C:\<nom>_log.txt`.
