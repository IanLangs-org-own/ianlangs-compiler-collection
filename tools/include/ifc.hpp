#include <iostream>
#include <vector>

#include "gen_files.hpp"
#include "compile.hpp"
#include "transpiler.hpp"

using namespace flowcpp;

inline void printHelp() {
    std::cout <<
R"(Uso: ifc archivo1.fcpp [archivo2.fcpp ...] [flags clang (o el especificado)]

Opciones:
  -o<nombre de archivo ejecutable>
  -cpp<nombre de archivo cpp>
  -c<nombre de archivo objecto>
  -cc=<compilador c++ /por defecto clang++>
  <mas flags del compilador elegido>
)";
}

std::string readFile(const std::string& path);

int c_main(std::vector<std::string> args);