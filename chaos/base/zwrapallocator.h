#ifndef ZWRAPALLOCATOR_H
#define ZWRAPALLOCATOR_H

#include "ztypes.h"
#include "zallocator.h"

namespace LibChaos {

template <typename T> class ZWrapAllocator : public ZAllocator<T> {
public:
    ZWrapAllocator(ZAllocator<zbyte> *alloc) : _alloc(alloc){

    }

    ZWrapAllocator(const ZWrapAllocator &) = delete;

    T *alloc(zu64 count = 1){
        return (T *)_alloc->alloc(count * sizeof(T));
    }

    void dealloc(T *ptr){
        _alloc->dealloc((zbyte *)ptr);
    }

private:
    ZAllocator<zbyte> *_alloc;
};

}

#endif // ZWRAPALLOCATOR_H
