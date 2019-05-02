#include "vkContext.h"
#include <iostream>

int main()
{
    vkContext r;
    try
    {
        r.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}