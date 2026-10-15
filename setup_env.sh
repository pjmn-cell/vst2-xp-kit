#!/bin/bash
# Installe tout ce qu'il faut (Ubuntu/Debian, en root). À lancer une fois dans un nouvel environnement.
set -e
dpkg --add-architecture i386 || true
apt-get update -qq
apt-get install -y -qq g++-mingw-w64-i686 g++-mingw-w64-x86-64 wine wine32:i386 xvfb python3-pip
pip install pefile pillow --break-system-packages -q
echo "OK. Premier lancement de wine : WINEARCH=win32 WINEPREFIX=/tmp/wp wineboot (ou simplement lancer un test)."
