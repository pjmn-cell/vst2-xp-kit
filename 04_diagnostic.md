# 04 — Diagnostic : la méthode qui a marché

## Protocole quand l'utilisateur dit « ça ne marche pas »
1. `python3 tools/audit_dll.py bin/MonPlugin.dll` : doit afficher `[OK ]` (exports = `main` seul, imports kernel32/user32/gdi32, pas de SSE/TLS/debug).
2. Livrer la **version `_log`** (`./build.sh monplugin log`) ; l'utilisateur la met seule dans `C:\Program Files\Steinberg\Vstplugins`
   (sans les anciennes versions), lance Nuendo, puis envoie **`C:\<nom>_log.txt`** (sinon `%TEMP%\<nom>_log.txt`).
3. Lire le journal (voir `docs/exemple_journal_nuendo.txt`) :
   - **aucun fichier** : Nuendo n'a jamais chargé la DLL -> dossier non scanné, plugin mis sur liste noire (changer nom de fichier/d'effet/`uniqueID`),
     fonction Windows manquante (relancer l'audit), mauvais format.
   - `DLL_PROCESS_ATTACH` seul : la DLL est chargée mais `main` n'est pas appelé/échoue.
   - le journal s'arrête après `main() OK` ou après un `op=N -> r` : **c'est la réponse à cet opcode que l'hôte a refusée** (c'était `op=22`).
   - `DLL_PROCESS_DETACH` juste après : l'hôte a déchargé le plugin.
4. Demander un **plugin qui marche** de même catégorie (effet / synthé) et le comparer (ci-dessous).

## Comparer avec un plugin qui marche
    python3 tools/pe_info.py leur.dll notre.dll                       # format du fichier, imports, exports, sections
    i686-w64-mingw32-g++ -O1 -std=gnu++17 -I common -o host_generic.exe tests/host_generic.cpp -static -static-libgcc -static-libstdc++ -lgdi32 -luser32 -lkernel32
    WINEDEBUG=-all WINEPREFIX=/tmp/wp WINEARCH=win32 wine host_generic.exe leur.dll
`host_generic` charge la DLL comme un hôte, affiche tous les champs d'`AEffect`, puis interroge les opcodes 3 à 80, `canDo`, les paramètres et fait
tourner `processReplacing`. **`host_identify.exe`** affiche la réponse à l'opcode 22 (c'est ainsi qu'on a vu que les plugins de référence répondent
`'NvEf'` et pas nous). Variables : `HOSTVER=2300` (réponse à `audioMasterVersion`), `FPUTRAP=1` (processeur en mode strict, précision 24 bits).
Pour lire le code d'un plugin de référence : trouver l'adresse de l'export `main` (pefile), puis
`i686-w64-mingw32-objdump -d -M intel plugin.dll` à cette adresse (on y a vu l'appel à `audioMasterVersion` puis la construction d'`AEffect`).
**Un plugin compressé par un packer** (`DaTube.dll`, sections `Oreloc`/`.neolit`) ne démarre pas sous Wine : inexploitable pour comparer.

## Wine (rappels)
- `WINEDEBUG=-all WINEPREFIX=/tmp/wp WINEARCH=win32` ; premier lancement lent (création du préfixe).
- Fenêtres : `xvfb-run -a -s "-screen 0 800x600x24" wine …`. Capture : `BitBlt` de la fenêtre vers un BMP (voir `capture()` dans
  `tests/host_linkbus.cpp`), converti en PNG avec Pillow, puis regardé.
- Les sorties de Wine finissent en `\r\n` : `tr -d '\r'` avant de parser.
- **Limite** : les tests Wine ne reproduisent PAS les exigences des vieux hôtes (`effIdentify`…). Ils prouvent que la DLL est valide et que le DSP/éditeur
  marchent, pas que Nuendo 2 l'acceptera. D'où la version `_log` et les plugins de référence.
