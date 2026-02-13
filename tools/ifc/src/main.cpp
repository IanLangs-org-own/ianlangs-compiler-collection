#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include "gen_files.hpp"
#include "compile.hpp"
#include "transpiler.hpp"
namespace fs = std::filesystem;
using namespace flowcpp;

static std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Archivo no existe: " << path << "\n";
        std::exit(EXIT_FAILURE);
    }
    return std::string(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
}

static void printHelp() {
    std::cout <<
R"(Uso: ifc archivo1.fcpp [archivo2.fcpp ...] [flags g++]

Opciones:
  -o<nombre>
  -cpp<nombre>
  -c<nombre>
)";
}

int main(int argc, char* argv[]) {

    if (argc == 1) {
        printHelp();
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    if (args.size() == 1 && args[0] == "-h") {
        printHelp();
        return 0;
    }

    if (args.size() == 1 && args[0] == "-v") {
        std::cout << "ifc version 3.1\nflow c++ version 3.2\n";
        return 0;
    }

    // detectar compiler
    std::string compiler = "clang++";
    for(size_t i = 0; i < args.size();i++) {
        auto arg = args[i]; 
        if (arg.starts_with("-cc=")) {
            compiler = arg.substr(4);
            args.erase(args.begin() + i);
            break;
        }
    }

    initDirs();

    std::vector<std::string> inputs;
    std::vector<std::string> flags;

    for (const auto& a : args) {
        if (a.size() >= 5 &&
            a.substr(a.size()-5) == ".fcpp")
            inputs.push_back(a);
        else
            flags.push_back(a);
    }

    if (inputs.empty()) {
        std::cerr << "No se pasaron archivos .fcpp\n";
        return 1;
    }

    std::string outName = "app";
    for (auto it = flags.begin(); it != flags.end(); ++it) {
        if (it->rfind("-o", 0) == 0 && it->size() > 2) {
            outName = it->substr(2);
            flags.erase(it);
            break;
        }
    }

    std::vector<std::string> cppToCompile;

    for (const auto& file : inputs) {
        std::string src = readFile(file);
        std::string cpp = transpile(src);

        std::string name =
            fs::path(file).stem().string();

        std::string cppPath =
            genCppFile(name, cpp, false);

        cppToCompile.push_back(cppPath);
    }

    compileAll(flags, cppToCompile, outName, compiler);

    return 0;
}
