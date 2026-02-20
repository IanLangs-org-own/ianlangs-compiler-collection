#pragma once
#include <string>
#include <vector>

namespace flowcpp {

std::vector<std::string> compileObjs(
    const std::vector<std::string>& flags,
    const std::vector<std::string>& cppFilesToCompile,
    const std::string& compiler
);

void linkObjs(const std::string& outName,
              const std::vector<std::string>& objs,
              const std::string& compiler);

const std::string compileAll(const std::vector<std::string>& flags,
                const std::vector<std::string>& cppFilesToCompile,
                const std::string& outName = "app",
                const std::string& compiler = "clang++");

}
