#!/bin/env bash

echo "no windows version for ifc and ibf"

## ifc
ifc() {
cd ifc
    echo "linux ifc"
    ./dist/ifc ./src/*.fcpp -oifc "\"-DIFC=\\\"3.4.4\\\"\"" "\"-DFCXX=\\\"0.4.5\\\"\"" -std=gnu++23
}
## ibf 
ibf() {
    cd ibf
    echo "linux ibf"
    ./dist/ifc ./src/*.fcpp -oibf -std=gnu++23
}
## ifmc
ifmc() {
    local files1=src/transpile.c 
    local files2=src/main.c
    local cpp=src/utils.cpp
    cd ifmc
    echo "linux ifmc"
    clang  -c $files1 $files2 && clang++ $cpp *.o -o ./dist/ifmc && rm -rf *.o
    echo "windows ifmc"
    clang --target=x86_64-pc-windows-gnu $files1 $files2 -c && clang++ --target=x86_64-pc-windows-gnu $cpp *.o -o ./dist/ifmc.exe && rm -rf *.o
}
## ilua
ilua() {
    cd ilua
    echo "linux ilua"
    clang  ./src/*.c ../../lua/*.c -o ./dist/ilua -lm
    echo "windows ilua"
    clang --target=x86_64-pc-windows-gnu ./src/*.c ../../lua/*.c -o ./dist/ilua.exe -lm
}

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 [ifc|ibf|ifmc|ilua]" 
    exit 1
fi
for index in $#
do

    case "${!index}" in
        ifc) ifc ;;
        ibf) ibf ;;
        ifmc) ifmc ;;
        ilua) ilua ;;
        all) ifc; cd ..; ibf; cd ..; ifmc; cd ..; ilua ;;
        *) echo "Unknown command: ${!index}" ;;
    esac

done