#include <flow/types>
#include <flow/collections>

#include <filesystem>

#include <fstream>


using namespace flow;

#define ROOT str("dist")

#define IR (ROOT+"/cpp")

#define OBJ (ROOT+"/obj")

namespace fs = std::filesystem;

namespace flowcpp {

void initDirs(){
    for (const auto& path : (str[]){IR, ROOT, OBJ}) {
        if (!fs::exists((std::string)path)) {
            fs::create_directories((std::string)path);
        }
    }
}

str genCppFile(const str name, const str code, bool header){
    str ext = header ? ".hpp" : ".cpp";
    str path = IR + "/" + name + ext;

    std::ofstream out(path);
    out << code;
    out.close();

    return path;
}

}
