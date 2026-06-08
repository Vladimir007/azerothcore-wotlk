#include <iostream>
#include <string>

#include "TileAssembler.h"

void Usage(const char* progName)
{
    std::cout << "Usage: " << progName << " <data dir>" << std::endl;
    exit(1);
}

int main(const int argc, char* argv[])
{
    std::string inputPath = ".";
    std::string src = "./Buildings";
    std::string dest = "./vMaps";

    if (argc > 2)
        Usage(argv[0]);

    if (argc > 1)
    {
        inputPath = std::string(argv[1]);
        if (inputPath.empty())
            Usage(argv[0]);
        if (inputPath.back() == '/')
            inputPath.pop_back();
        src = inputPath + "/Buildings";
        dest = inputPath + "/vMaps";
    }
    std::cout << "Using '" << src << "' as source directory and writing output to '" << dest << "'" << std::endl;

    const auto ta = new VMAP::TileAssembler(src, dest);

    if (!ta->convertWorld())
    {
        std::cout << "Exit with errors" << std::endl;
        delete ta;
        return 1;
    }

    delete ta;
    std::cout << "Ok, all done." << std::endl;
    return 0;
}
