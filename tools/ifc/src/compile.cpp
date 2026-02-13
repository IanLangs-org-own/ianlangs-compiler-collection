#include "compile.hpp"
#include "gen_files.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

namespace flowcpp {

static int execCmd(const std::string& cmd) {
    return std::system(cmd.c_str());
}

std::vector<std::string> compileObjs(
    const std::vector<std::string>& flags,
    const std::vector<std::string>& cppFilesToCompile,
    const std::string& compiler)
{
    std::vector<std::string> objs;

    for (const auto& cpp : cppFilesToCompile) {

        if (cpp.size() >= 4 &&
            cpp.substr(cpp.size()-4) == ".hpp")
            continue;

        std::string name = fs::path(cpp).stem().string();
        std::string objPath = objDir + "/" + name + ".o";

        std::string cmd = compiler + " -c " + cpp + " -o " + objPath;
        for (const auto& f : flags)
            cmd += " " + f;

        int res = execCmd(cmd);
        if (res != 0) {
            std::cerr << "Error compilando " << cpp << "\n";
            std::exit(EXIT_FAILURE);
        }

        objs.push_back(objPath);
    }

    return objs;
}

void linkObjs(const std::string& outName,
              const std::vector<std::string>& objs,
              const std::string& compiler)
{
    if (objs.empty()) {
        std::cerr << "No hay objetos para linkear\n";
        std::exit(EXIT_FAILURE);
    }

    std::string cmd = compiler;
    for (const auto& o : objs)
        cmd += " " + o;

    cmd += " -o " + distDir + "/" + outName;

    int res = execCmd(cmd);
    if (res != 0) {
        std::cerr << "Error linkeando\n";
        std::exit(EXIT_FAILURE);
    }
}

void compileAll(const std::vector<std::string>& flags,
                const std::vector<std::string>& cppFilesToCompile,
                const std::string& outName,
                const std::string& compiler)
{
    initDirs();
    auto objs = compileObjs(flags, cppFilesToCompile, compiler);
    linkObjs(outName, objs, compiler);
}

}
