#ifndef ZMULTIPLEX_H
#define ZMULTIPLEX_H

#include "zsocket.h"

namespace LibChaos {

class ZMultiplex {
public:
    ZMultiplex();

    bool add();
    bool wait();
    zsize count(){ return ready_count; }

private:
    int handle;
    zsize ready_count;
};

} // namespace LibChaos

#endif // ZMULTIPLEX_H
