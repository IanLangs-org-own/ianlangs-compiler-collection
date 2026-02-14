#!/bin/bash

set -e

CXX_STD=gnu++23
OUTDIR="build"
SRCDIR="src"
INCLUDEDIR="include"

mkdir -p "$OUTDIR"
if [[ "$1" == "IFC" ]]; then
    SOURCES=$(find "$SRCDIR" -name "*.cpp")

    echo "Compilando para Linux..."
    clang++ -DIFC=3.4 -DFCXX=4.0 -DCXX $SOURCES \
        -std=$CXX_STD -O2 \
        -I"$INCLUDEDIR" \
        -Wall -Wextra -Wpedantic \
        -o "$OUTDIR/ifc"

    echo "Compilando para Windows (x86_64)..."
    clang++ -DIFC=\"3.4.1\" -DFCXX=\"4.0.1\" -DCXX $SOURCES \
        --target=x86_64-w64-windows-gnu \
        -std=$CXX_STD -O2 \
        -I"$INCLUDEDIR" \
        -Wall -Wextra -Wpedantic \
        -o "$OUTDIR/ifc.exe"

    echo "Listo."
    echo "Binarios generados en $OUTDIR/"

    echo "Copiando"

    cp $OUTDIR/ifc ../pkg/icc/bin/linux/ifc

    cp $OUTDIR/ifc.exe ../pkg/icc/bin/win/ifc.exe
fi
echo "Solo compila los .deb, .rpm y .msi"

