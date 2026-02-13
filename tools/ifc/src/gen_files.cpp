#include "gen_files.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace flowcpp {

const std::string cacheDir = "dist/cpp";
const std::string distDir  = "dist";
const std::string objDir   = "dist/obj";

void initDirs() {
    for (const auto& path : {cacheDir, objDir, distDir}) {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    }
}

std::string genCppFile(const std::string& name,
                       const std::string& code,
                       bool hpp)
{
    std::string ext = hpp ? ".hpp" : ".cpp";
    std::string path = cacheDir + "/" + name + ext;

    std::ofstream out(path);
    out << code;
    out.close();

    return path;
}

}
