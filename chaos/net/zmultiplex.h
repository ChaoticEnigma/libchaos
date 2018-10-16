#ifndef ZMULTIPLEX_H
#define ZMULTIPLEX_H

#include "zsocket.h"

namespace LibChaos {

class ZMultiplex {
public:
    ZMultiplex();

    bool wait();

private:
    int handle;
};

} // namespace LibChaos

#endif // ZMULTIPLEX_H
