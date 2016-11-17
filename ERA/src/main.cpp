#include "Era.h"
#include "som.hpp"
//#include "Node.hpp"
//#include <iostream>
//#include <cstdlib>
//#include <ctime>


//========================================================================
int main( ){

    Era *era = new Era();

    era->openPorts("/era");

    era->init();
    era->run();
    era->clean();



    return 0;
}
