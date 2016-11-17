#include "Hogs.hpp"

//#include "Node.hpp"
//#include <iostream>
//#include <cstdlib>
#include <unistd.h>


//========================================================================
int main( ){

    Hogs *ho = new Hogs();

    ho->init();
    ho->run();

    return 0;
}
