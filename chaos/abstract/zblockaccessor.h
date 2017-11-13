#ifndef ZBLOCKACCESSOR_H
#define ZBLOCKACCESSOR_H

#include "zposition.h"
#include "zreader.h"
#include "zwriter.h"

namespace LibChaos {

class ZBlockAccessor : public ZPosition, public ZReader, public ZWriter {
public:
    virtual ~ZBlockAccessor(){}
};

}

#endif // ZBLOCKACCESSOR_H
