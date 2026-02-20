#include <iostream>
#include <vector>

#if defined(IFC)
    #include "ifc.hpp"
#endif

#if defined(IFC) && !defined(FCXX)
#error "need fc++ version if compile for ifc"
#endif

int main(int argc, char* argv[]) {

    if (argc == 1) {
        #if defined(IFC)
            printHelp();
        #endif
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);
    
    #if defined(IFC)
        return c_main(args);
    #else
        std::cerr << "no have a compiler" << std::endl;
        return 1;
    #endif
}
