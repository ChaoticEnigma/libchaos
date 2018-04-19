#include "ztypes.h"

// Git Describe
#define LIBCHAOS_DESCRIBE _LIBCHAOS_DESCRIBE

namespace LibChaos {

const char *LibChaosDescribe(){
    return LIBCHAOS_DESCRIBE;
}

zu32 LibChaosBuildConfig(){
    return (LIBCHAOS_COMPILER << 16) | (LIBCHAOS_PLATFORM << 8) | (LIBCHAOS_BUILD);
}

}
