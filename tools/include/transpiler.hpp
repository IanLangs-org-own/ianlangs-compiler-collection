#pragma once
#include <string>
#include <set>
namespace flowcpp {

// El .cpp lo implementás vos
std::string transpile(const std::string& rawCode, std::set<std::string>* outHeaders);

}
