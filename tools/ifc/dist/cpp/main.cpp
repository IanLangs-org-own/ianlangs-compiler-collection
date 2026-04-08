#include <flow/types>
#include "ifc.hpp"

#include <flow/io>

#include <flow/fs>

#include <fstream>

#include <filesystem>


using namespace flow;

int main(int argc, char** argv){
    if (argc == 1) {
        printHelp();
        return 1;
    }

    flow::vector<str> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);
    if (args.size() == 1 && (args[0] == "-h" || args[0] == "--help")) {
        printHelp();
        return 0;
    }

    if (args.size() == 1 && (args[0] == "-v" || args[0] == "--version")) {
        flow::println("ifc version ", IFC, "\n", "flow c++ version ", FCXX);
        return 0;
    }
        str compiler = "clang++";
    for(size_t i = 0; i < args.size();i++) {
        auto arg = args[i]; 
        if (arg.starts_with("-cc=")) {
            compiler = arg.substr(4);
            args.erase(args.begin() + i);
            break;
        }
    }

    initDirs();

    flow::vector<str> inputs;
    flow::vector<str> objs;
    flow::vector<str> flags;
    flow::set<str> headers;
    bool run = false;
    for (const auto& a : args) {
        if (a.size() >= 5 && a.substr(a.size()-5) == ".fcpp")
            inputs.push_back(a);
        else if (a.size() >= 2 && a.substr(a.size()-2) == ".o")
            objs.push_back(a);
        else {
            if (a != "-r") flags.push_back(a);
            else run = true;
        }
    }

    if (inputs.empty()) {
        flow::raise("No se pasaron archivos .fcpp");
        return 1;
    }

    str outName = "app";
    for (auto it = flags.begin(); it != flags.end(); ++it) {
        if (it->rfind("-o", 0) == 0 && it->size() > 2) {
            outName = it->substr(2);
            flags.erase(it);
            break;
        }
    }

    std::vector<str> cppToCompile;

    auto process = [&](const auto& files, bool collectHeaders) {
        for (const auto& path : files) {
            flow::File file = flow::openFile(path);
            str src = file.read();
            str cpp = transpile(src, (collectHeaders ? &headers : nullptr));

            str name = std::filesystem::path((std::string)path).stem().string();
            str cppPath = genCppFile(name, cpp, !collectHeaders);

            cppToCompile.push_back(cppPath);
        }
    };

    process(inputs, true);

    flow::vector<str> headersVec(headers.begin(), headers.end());
    process(headersVec, false);


    str out = compileAll(flags, cppToCompile, objs, outName, compiler);

    if (run) {
        std::system(flow::format("./{}", ROOT + "/" + out).c_str());
    }

    return 0;
}