#include <iostream>
#include <vector>
#if defined(IFC)
    #include "ifc.hpp"
#elif defined(ILUA)
    extern "C" {
        #include "ilua.h"
    }
#endif

#if defined(IFC) && !defined(FCXX)
#error "need fc++ version if compile for ifc"
#endif

int main(int argc, char* argv[]) {

    if (argc == 1) {
        #if defined(IFC) || defined(ILUA)
            printHelp();
        #endif
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);
    
    #if defined(IFC)
        return ifc_main(args);
    #elif defined(ILUA)
        return ilua_main(argv);
    #else
        std::cerr << "no have a compiler" << std::endl;
        return 1;
    #endif
}
