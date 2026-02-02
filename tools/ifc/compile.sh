#!/bin/bash
set -e

SRC="src/main.nim"
OUT="no_github"

mkdir -p "$OUT"

echo "=== LINUX (ELF) ==="
nim c -d:release --threads:off --hints:off --out:"$OUT/ifc" "$SRC"
echo "ELF listo: $OUT/ifc"

echo
echo "=== WINDOWS (EXE) ==="
# Usa MinGW 64 bits
if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  echo "Instala mingw-w64 64 bits: sudo apt install mingw-w64"
  exit 0
fi

nim c -d:release --threads:off --hints:off \
  --os:windows --cpu:amd64 \
  --cc:gcc --gcc.exe:x86_64-w64-mingw32-gcc --gcc.linkerexe:x86_64-w64-mingw32-gcc \
  --passL:"-static -lwinpthread" \
  --out:"$OUT/ifc.exe" "$SRC"

echo "EXE listo: $OUT/ifc.exe"

echo
echo "=== Copiando (PKG) ==="

cp -f no_github/ifc ../../pkg/icc/bin/linux/ifc
cp -f no_github/ifc.exe ../../pkg/icc/bin/win/ifc.exe


echo "Copias listas"
