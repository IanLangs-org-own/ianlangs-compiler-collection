#include <iostream>
#include <vector>

#include "gen_files.hpp"
#include "compile.hpp"
#include "transpiler.hpp"

namespace flowcpp {
	constexpr bool have_fcpp = 
		#ifdef IFC
			true
		#else
			false
		#endif
	;
}

using namespace flowcpp;

inline void printHelp() {
    std::cout <<
R"(Uso: ifc archivo1.fcpp [archivo2.fcpp ...] [flags]

Opciones:
  -o<nombre de archivo ejecutable>
  -cpp<nombre de archivo cpp>
  -c<nombre de archivo objecto>
  -cc=<compilador c++ /por defecto clang++>
  <mas flags del compilador elegido>
)";
}

std::string readFile(const std::string& path);

int ifc_main(std::vector<std::string> args);