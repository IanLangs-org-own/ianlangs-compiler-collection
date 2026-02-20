#include "compile.hpp"
#include "gen_files.hpp"
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;
#if defined(NOHEADER)
#define HEADER "/"
#elif defined(CXX)
#define HEADER ".hpp"
#else
#error "need language to define header"
#endif

namespace flowcpp {

static int execCmd(const std::string& cmd) {
    return std::system(cmd.c_str());
}

std::vector<std::string> compileObjs(const std::vector<std::string>& flags, const std::vector<std::string>& cppFilesToCompile, const std::string& compiler)
{
    std::vector<std::string> objs;

    for (const auto& cpp : cppFilesToCompile) {

        if (cpp.size() >= 4 &&
            cpp.substr(cpp.size()-std::string(HEADER).size()) == HEADER)
            continue;

        std::string name = fs::path(cpp).stem().string();
        std::string objPath = OBJ + "/" + name + ".o";

        std::string cmd = compiler + " -c " + cpp + " -o " + objPath;
        for (const auto& f : flags)
            cmd += " " + f;
        
        #ifdef IFC
        cmd += (std::string)" -D__flow_c_plusplus=" + FCXX ;
        #endif

        int res = execCmd(cmd);
        if (res != 0) {
            std::cerr << "Error compilando " << cpp << std::endl;
            std::exit(EXIT_FAILURE);
        }

        objs.push_back(objPath);
    }

    return objs;
}

void linkObjs(const std::string& outName, const std::vector<std::string>& objs, const std::string& compiler)
{
    if (objs.empty()) {
        std::cerr << "No hay objetos para linkear" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::string cmd = compiler;
    for (const auto& o : objs)
        cmd += " " + o;

    cmd += " -o " + ROOT + "/" + outName;

    #ifdef IFC
    cmd += (std::string)" -D__flow_c_plusplus=" + FCXX + " ";
    #endif

    int res = execCmd(cmd);
    if (res != 0) {
        std::cerr << "Error linkeando" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

const std::string compileAll(const std::vector<std::string>& flags,
                const std::vector<std::string>& cppFilesToCompile,
                const std::string& outName,
                const std::string& compiler)
{
    initDirs();
    auto objs = compileObjs(flags, cppFilesToCompile, compiler);
    linkObjs(outName, objs, compiler);
    return outName;
}

}
