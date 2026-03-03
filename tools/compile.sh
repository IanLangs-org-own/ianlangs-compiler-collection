#!/bin/bash

set -e

CXX_STD=gnu++23
OUTDIR="build"
SRCDIR="src"
INCLUDEDIR="include"
OBJDIR="obj"
mkdir -p "$OUTDIR"
mkdir -p "$OBJDIR"
if [[ "$1" == "IFC" ]]; then
    SOURCES="src/main.cpp src/transpile.cpp src/gen_files.cpp src/compile.cpp src/ifc.cpp"
    VERSION="-DIFC=\"3.4.4\" -DFCXX=\"0.4.5\""

    echo "Compilando para Linux..."
    clang++ -DCXX $VERSION $SOURCES \
        -std=$CXX_STD -O2 \
        -I"$INCLUDEDIR" \
        -Wall -Wextra -Wpedantic \
        -o "$OUTDIR/ifc"

    echo "Compilando para Windows (x86_64)..."
    clang++ $VERSION -DCXX $SOURCES \
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
elif [[ "$1" == "ILUA" ]]; then
    SOURCES="src/main.cpp"
    C_FILES=(../lua/*.c src/ilua.c)
    VERSION="-DILUA=\"1.0\""
    echo "Compilando para Linux..."
    for file in "${C_FILES[@]}"; do
        clang -c $VERSION "$file" -I../lua -DLUA_USE_POPEN -DLUA_USE_LINUX \
            -O2 \
            -I"$INCLUDEDIR" \
            -Wall -Wextra -Wpedantic \
            -o "$OBJDIR/$(basename "${file%.c}.o")"
    done
    clang++ $VERSION $SOURCES $OBJDIR/*.o -I../lua \
        -std=$CXX_STD -O2 \
        -I"$INCLUDEDIR" \
        -Wall -Wextra -Wpedantic \
        -o "$OUTDIR/ilua"

    echo "Compilando para Windows (x86_64)..."

    for file in "${C_FILES[@]}"; do
        clang -c $VERSION "$file" -I../lua -DLUA_USE_POPEN \
            --target=x86_64-w64-windows-gnu \
            -O2 \
            -I"$INCLUDEDIR" \
            -Wall -Wextra -Wpedantic \
            -o "$OBJDIR/$(basename "${file%.c}.o")"
    done
    clang++ $VERSION $SOURCES $OBJDIR/*.o -I../lua \
        --target=x86_64-w64-windows-gnu \
        -std=$CXX_STD -O2 \
        -I"$INCLUDEDIR" \
        -Wall -Wextra -Wpedantic \
        -o "$OUTDIR/ilua.exe"

    echo "Listo."
    echo "Binarios generados en $OUTDIR/"

    echo "Copiando"

    cp $OUTDIR/ilua ../pkg/icc/bin/linux/ilua

    cp $OUTDIR/ilua.exe ../pkg/icc/bin/win/ilua.exe
fi
rm -rf obj
echo "Solo compila los .deb, .rpm y .msi"

