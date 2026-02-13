#pragma once
#include <string>

#define ROOT std::string("dist")

#ifdef CXX
#define IR (ROOT+"/cpp")
#else
#define IR (ROOT+"/ir")
#endif

#define OBJ (ROOT+"/obj")

namespace flowcpp {

void initDirs();

std::string genCppFile(const std::string& name,
                       const std::string& code,
                       bool hpp = false);

}
