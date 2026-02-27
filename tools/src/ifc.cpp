#include "ifc.hpp"
#include <fstream>
#include <filesystem>
#include <format>
#include <set>

namespace fs = std::filesystem;

std::string readFile(const std::string& path) {
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

int ifc_main(std::vector<std::string> args) {
    if (args.size() == 1 && (args[0] == "-h" || args[0] == "--help")) {
        printHelp();
        return 0;
    }

    if (args.size() == 1 && (args[0] == "-v" || args[0] == "--version")) {
        std::cout << "ifc version " << IFC << std::endl << "flow c++ version " << FCXX << std::endl;
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
    std::set<std::string> headers;
    bool run = false;
    for (const auto& a : args) {
        if (a.size() >= 5 &&
            a.substr(a.size()-5) == ".fcpp")
            inputs.push_back(a);
        else
            if (a != "-r") flags.push_back(a);
            else run = true;
    }

    if (inputs.empty()) {
        std::cerr << "No se pasaron archivos .fcpp" << std::endl;
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

    auto process = [&](const auto& files, bool collectHeaders) {
        for (const auto& file : files) {
            std::string src = readFile(file);
            std::string cpp = transpile(src, collectHeaders ? &headers : nullptr);

            std::string name = fs::path(file).stem().string();
            std::string cppPath = genCppFile(name, cpp, !collectHeaders);

            cppToCompile.push_back(cppPath);
        }
    };

    process(inputs, true);

    std::vector<std::string> headersVec(headers.begin(), headers.end());
    process(headersVec, false);


    std::string out = compileAll(flags, cppToCompile, outName, compiler);

    if (run) {
        std::system(std::format("./{}", ROOT + "/" + out).c_str());
    }

    return 0;
}