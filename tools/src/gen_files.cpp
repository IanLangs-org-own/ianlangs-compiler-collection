#include "gen_files.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace flowcpp {

void initDirs() {
    for (const auto& path : {IR, ROOT, OBJ}) {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    }
}

std::string genCppFile(const std::string& name,
                       const std::string& code,
                       bool header)
{
    std::string ext = header ? ".hpp" : ".cpp";
    std::string path = IR + "/" + name + ext;

    std::ofstream out(path);
    out << code;
    out.close();

    return path;
}

}
