#ifndef ZALLOCATORPOOL_H
#define ZALLOCATORPOOL_H

#include "ztypes.h"
#include "zallocator.h"

namespace LibChaos {

template <typename T = void> class ZPoolAllocator : public ZAllocator<T> {
public:
    enum nodeflag {
        ALLOC   = 1,
        NEXT    = 2,
        TAIL    = 4,
    };

    struct AllocNode {
        zu64 size;
        zu8 flag;
    };

public:
    ZPoolAllocator(void *pool, zu64 size) : _pool(pool), _poolsize(size){
        AllocNode *head = (AllocNode *)_pool;
        head->size = _poolsize;
        head->flag = TAIL;
    }

    T *alloc(zu64 count){
        if(!count)
            return nullptr;
        AllocNode *head = (AllocNode *)_pool;
        AllocNode *node = head;
        const zu64 nsize = sizeof(T) * count;
        while((node + sizeof(AllocNode)) < (_pool + _poolsize)){
            // skip allocated blocks
            if(!(node->flag & ALLOC)){
                // find needed size
                if(node->size >= nsize){
                    if(node->size > nsize){
                        // split
                        zu64 lsize = node->size - nsize;
                        node->size = nsize;
                        // new node (may move tail)
                        AllocNode *next = node + sizeof(AllocNode) + node->size;
                        next->size = lsize;
                        next->flag = node->flag & TAIL;
                    }
                    // allocate and return
                    node->flag |= ALLOC;
                    return (T *)(node + sizeof(AllocNode));
                }
            }

            if(node->flag & TAIL){
                // this is the last node, alloc failed
                return nullptr;
            }

            node = node + sizeof(AllocNode) + node->size;
        }
        // this is a bug
        return nullptr;
    }

    void dealloc(T *ptr){
        if(!ptr)
            return;
        if(ptr <= _pool || ptr > _pool + _poolsize)
            return;
        AllocNode *node = (AllocNode *)(ptr - sizeof(AllocNode));
        node->flag &= ~((zu8)ALLOC);
    }

private:
    void *_pool;
    zu64 _poolsize;
};

}

#endif // ZALLOCATORPOOL_H
