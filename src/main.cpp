#include"ogles.h"
#ifdef _RPI
#include "bcm_host.h"
#endif


int main()
{
#ifdef _RPI
    bcm_host_init();
#endif

    Ogles ogles("../test.tga");

    ogles.extractSpots();

#ifdef _RPI
    bcm_host_deinit();
#endif

    return 0;
}
