#ifndef build_interface_layer_
#define build_interface_layer_ CTR

#include "compat.h"
#include "baselayer.h"

static inline void idle(void)
{
    svcSleepThread(1000000);
}

#endif // build_interface_layer_