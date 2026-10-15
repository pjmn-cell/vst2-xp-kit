# Kit « plugins VST2 pour Windows XP / Nuendo 2 »

Tout ce qu'il faut pour créer d'autres plugins VST2 (32 bits) qui se chargent dans **Nuendo 2 sous Windows XP**,
cross-compilés depuis Linux et testés sous Wine. Rédigé pour être relu par Claude dans une nouvelle conversation
(Claude ne garde aucun souvenir d'une conversation à l'autre : ce kit EST la mémoire).

**Commencer par :** `PROMPT_DEMARRAGE.md` (texte à coller), puis `docs/02` et `docs/05`.

## Contenu
| Dossier / fichier | Rôle |
|---|---|
| `PROMPT_DEMARRAGE.md` | Texte à coller dans une nouvelle conversation avec le kit |
| `docs/01_contexte.md` | La machine, l'hôte, les préférences de l'utilisateur, l'état de chaque plugin |
| `docs/02_recette_technique.md` | **La recette** : options de compilation, contraintes, ABI VST2, opcodes à gérer |
| `docs/03_historique_des_blocages.md` | Les échecs successifs, leurs symptômes et leurs vraies causes (dont `effIdentify`) |
| `docs/04_diagnostic.md` | La méthode qui a marché (comparer, exécuter sous Wine, journaliser) |
| `docs/05_creer_un_nouveau_plugin.md` | Pas à pas pour un nouvel effet, à partir du squelette |
| `docs/06_plugins_de_reference.md` | Analyse des 3 plugins qui marchent sur la machine cible |
| `docs/07_limites_et_legal.md` | Ce qui n'est pas géré, ce qui n'est pas vérifié, aspects légaux |
| `docs/exemple_journal_nuendo.txt` | Vrai journal envoyé par Nuendo 2 (le plugin rejeté à cause de `effIdentify`) |
| `common/` | Code partagé : en-tête VST2 réécrit, maths sans libm, base de plugin (`vstbase.h`), bus de liaison |
| `skeleton/` | **Point de départ** d'un nouveau plugin (un gain, ~40 lignes) |
| `fuzzpi/` `linkcomp/` `linksend/` `linkrecv/` | Les plugins déjà réalisés (exemples complets) |
| `tests/` | Hôtes VST de test exécutés sous Wine + test de moteur |
| `tools/` | `run_tests.sh` (tout vérifier), `audit_dll.py`, `pe_info.py`, `setup_env.sh` |
| `bin/` | DLL déjà compilées (normales + versions `_log`) |
| `build.sh` | `./build.sh <projet> [log]` ou `./build.sh all` -> `bin/` |

## Démarrage rapide (nouvel environnement Linux)
    ./tools/setup_env.sh          # installe mingw-w64, wine32, xvfb, pefile (root)
    ./tools/run_tests.sh          # construit TOUT, audite, exécute 18 contrôles sous Wine -> "TOUS LES CONTRÔLES PASSENT"
    cp -r skeleton monplugin      # puis adapter (voir docs/05)
    ./build.sh monplugin

## État des plugins sur la machine cible (Nuendo 2 / XP)
| Plugin | Rôle | Statut |
|---|---|---|
| **FuzzPi** | distorsion type Big Muff | **Confirmé : fonctionne** dans Nuendo 2 (version livrée après la correction `effIdentify`) |
| LinkComp | compresseur tout-en-un, side-chain lié (une DLL) | testé sous Wine seulement, pas de retour de l'utilisateur |
| LinkSend + LinkRecv | émetteur (seuil + hold) / récepteur (compresseur), 2 DLL | testé sous Wine seulement, pas de retour de l'utilisateur |
| Skeleton | gain d'exemple | testé sous Wine seulement |
> Les DLL de `bin/` sont reconstruites par `build.sh` : leur code est identique à celles livrées, seuls l'horodatage
> et le checksum du fichier diffèrent.

## Les 5 règles d'or
1. L'hôte réel (Nuendo 2) envoie **`effIdentify` (opcode 22) en premier** : répondre `'NvEf'` (0x4e764566), sinon il décharge le plugin sans message.
2. **Aucun runtime C/C++** (`-nostdlib`), **x87 pur** (pas de SSE), **un seul export : `main`**. `tools/audit_dll.py` le vérifie.
3. **Toujours livrer en plus la version `_log`** : le journal d'une vraie session Nuendo vaut mieux que dix hypothèses.
4. Tester les **vraies DLL sous Wine** (`tools/run_tests.sh`) avant de livrer ; dire honnêtement que ça n'a pas été testé sur Nuendo.
5. Si ça échoue chez l'utilisateur : demander (a) le journal, (b) un plugin qui marche, de même catégorie, pour comparer.
