#!/bin/bash
# Compile des plugins VST2 pour Windows XP / Nuendo 2 (32 bits, x87 pur, sans runtime C). Sortie : bin/
# Usage : ./build.sh <dossier-projet> [clean|log]     ex. ./build.sh linkrecv log
#         ./build.sh all                              (tous les projets, versions normale ET journal)
#   clean = version normale ; log = version qui écrit un journal (C:\<nom>_log.txt, sinon %TEMP%) pour diagnostiquer.
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"; mkdir -p "$ROOT/bin"
build_one() {
  local PRJ=$1 MODE=${2:-clean}
  ( cd "$ROOT/$PRJ"; source ./project.conf
    local EXTRA="" OUTN="$FILE" N="$NAME" U="$PUID"
    if [ "$MODE" = log ]; then EXTRA="-DBF_LOG"; OUTN="${FILE}_log"; N="$LOGNAME"; U="$LOGUID"; fi
    local BASE="-O2 -fno-tree-loop-distribute-patterns -std=gnu++17 -DBF_NOCRT -DBF_EDITOR -fno-exceptions -fno-rtti -fno-math-errno -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -fno-threadsafe-statics -shared -nostdlib -s"
    local LD="-Wl,-e,_DllMain@12 -Wl,--subsystem,windows:4.0 -Wl,--major-os-version,4 -Wl,--disable-dynamicbase -Wl,--disable-nxcompat -Wl,--image-base,0x10000000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000"
    local X87="-march=i586 -mtune=generic -mfpmath=387 -mno-sse -mno-mmx -mstackrealign"
    i686-w64-mingw32-windres -O coff version.rc -o ver.o
    i686-w64-mingw32-g++ -I"$ROOT/common" -I. $BASE $EXTRA "-DBF_NAME=\"$N\"" -DBF_UID=$U $X87 $LD -o "$ROOT/bin/$OUTN.dll" plugin.cpp ver.o "$DEF" -lkernel32 -luser32 -lgdi32 -lgcc
    rm -f ver.o; echo "OK -> bin/$OUTN.dll" )
}
if [ "${1:-}" = all ]; then
  for d in "$ROOT"/*/; do
    if [ -f "$d/project.conf" ]; then for m in clean log; do build_one "$(basename "$d")" $m; done; fi
  done
else build_one "${1:?usage: ./build.sh <projet> [clean|log] | all}" "${2:-clean}"; fi
