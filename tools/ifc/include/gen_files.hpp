#pragma once
#include <string>

namespace flowcpp {

extern const std::string cacheDir;
extern const std::string distDir;
extern const std::string objDir;

void initDirs();

std::string genCppFile(const std::string& name,
                       const std::string& code,
                       bool hpp = false);

}
