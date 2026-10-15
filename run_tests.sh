#!/bin/bash
# Construit tout, audite les DLL, puis exécute les hôtes de test SOUS WINE (vraies DLL Windows).
# Affiche PASS / FAIL pour chaque contrôle. À lancer AVANT de livrer un plugin.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
export WINEDEBUG=-all WINEPREFIX=${WINEPREFIX:-/tmp/wp} WINEARCH=win32
W=/tmp/kit_tests; mkdir -p $W; FAIL=0
res() { if [ "$1" = 0 ]; then echo "  PASS  $2"; else echo "  FAIL  $2"; FAIL=1; fi; }
echo "== 1) compilation de tous les projets"; ./build.sh all > $W/build.log 2>&1; res $? "build.sh all"
echo "== 2) audit des DLL (fonctions XP, SSE, TLS, exports...)"; python3 tools/audit_dll.py bin/*.dll; res $? "audit_dll.py"
echo "== 3) compilation des hôtes de test"
for t in host_generic host_identify host_linkcomp host_linkbus host_skeleton; do
  i686-w64-mingw32-g++ -O1 -std=gnu++17 -I common -o $W/$t.exe tests/$t.cpp -static -static-libgcc -static-libstdc++ -lgdi32 -luser32 -lkernel32 2>$W/$t.err; res $? "$t.exe"; done
cp bin/*.dll $W/ 2>/dev/null
echo "== 4) effIdentify = 'NvEf' (exigé par Nuendo 2) pour chaque DLL"
n=$(ls bin/*.dll | wc -l); ok=$(cd $W && wine host_identify.exe $(cd "$ROOT/bin" && ls *.dll) 2>&1 | grep -ac "NvEf'$\|'NvEf'")
[ "$ok" -ge "$n" ]; res $? "$ok/$n DLL répondent 'NvEf'"
echo "== 5) LinkSend + LinkRecv : deux DLL distinctes, 3 liens, transparence, éditeurs (FPU strict)"
out=$(cd $W && xvfb-run -a -s "-screen 0 800x600x24" env FPUTRAP=1 wine host_linkbus.exe LinkSend.dll LinkRecv.dll edit 2>&1 | tr -d "\r")
# les lignes ressemblent à : "R1 (lien 1)   16.7   |   3.8"  -> valeur de gauche = pendant le kick du lien 1, de droite = lien 2
left()  { echo "$out" | awk -F'|' -v k="$1" '$0 ~ k { n=split($1,a," "); print a[n] }'; }
right() { echo "$out" | awk -F'|' -v k="$1" '$0 ~ k { gsub(/ /,"",$2); print $2 }'; }
num()   { echo "$1" | grep -Eq '^-?[0-9]+(\.[0-9]+)?$'; }
r1=$(left 'R1 \(lien 1\)'); r2=$(right 'R2 \(lien 2\)'); r3a=$(left 'R3 \(lien 3'); r3b=$(right 'R3 \(lien 3')
num "$r1" && awk -v a="$r1" 'BEGIN{exit !(a+0>10)}'; res $? "lien 1 : basse réduite de ${r1} dB pendant son kick (> 10 attendu)"
num "$r2" && awk -v a="$r2" 'BEGIN{exit !(a+0>10)}'; res $? "lien 2 : basse réduite de ${r2} dB pendant son kick (> 10 attendu)"
num "$r3a" && num "$r3b" && awk -v a="$r3a" -v b="$r3b" 'BEGIN{exit !(a+0<0.5 && b+0<0.5)}'; res $? "lien 3 (sans émetteur) non touché (${r3a} / ${r3b} dB)"
echo "$out" | grep -aq "transparent.*: oui"; res $? "LinkSend transparent bit pour bit"
echo "$out" | grep -aq "FIN OK"; res $? "séquence complète sans plantage"
echo "== 6) LinkComp (DLL tout-en-un) : Send / Cmp+Send / Receive"
out=$(cd $W && env FPUTRAP=1 wine host_linkcomp.exe LinkComp.dll 2>&1 | tr -d "\r")
echo "$out" | grep -a "Send (kick inchange)" | grep -aq "reduite de  *1[0-9]\."; res $? "Send -> Receive : basse réduite de plus de 10 dB"
echo "$out" | grep -aq "FIN OK"; res $? "séquence complète"
echo "== 7) Skeleton (gain +/-12 dB, éditeur)"
out=$(cd $W && xvfb-run -a -s "-screen 0 800x600x24" wine host_skeleton.exe Skeleton.dll 2>&1 | tr -d "\r"); echo "$out" | grep -a "param="; echo "$out" | grep -aq "^PASS"; res $? "squelette : gain exact et éditeur"
echo "== 8) moteur LinkSend/LinkRecv natif (seuil, hold)"
g++ -O2 -fno-math-errno -I common -o $W/engine tests/engine_linkbus.cpp 2>$W/engine.err && $W/engine > $W/engine.out; res $? "engine_linkbus"
! grep -q "NON" $W/engine.out; res $? "Send transparent dans tous les cas"
echo; if [ $FAIL = 0 ]; then echo "TOUS LES CONTRÔLES PASSENT"; else echo "AU MOINS UN CONTRÔLE ÉCHOUE (voir ci-dessus, journaux dans $W)"; fi
exit $FAIL
