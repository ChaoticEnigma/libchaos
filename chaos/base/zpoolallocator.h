/*******************************************************************************
**                                  LibChaos                                  **
**                              zpoolallocator.h                              **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#ifndef ZALLOCATORPOOL_H
#define ZALLOCATORPOOL_H

#include "ztypes.h"
#include "zallocator.h"
#include "zlog.h"

namespace LibChaos {

/*! Allocates memory from a preallocated pool.
 *  Allocation info is stored inside the pool, to allow the pool memory to be from a different
 *  address space than the allocator. Specifically, this allows a region of shared memory to be
 *  given to a ZPoolAllocator, then the owning process forked. The ZPoolAllocator instance may be
 *  copied in child process, but the pool will not be, and both ZPoolAllocators will see the same
 *  allocation information.
 *
 *  ZPoolAllocator does not provide synchronization between threads / processes, that must be
 *  implemented manually.
 *
 *  Allocation data overhead for n allocated elements is 16 * (n+1).
 */
template <typename T = void> class ZPoolAllocator : public ZAllocator<T> {
public:
    enum nodeflag {
        ALLOC   = 1,
        TAIL    = 2,
    };

    struct AllocNode {
        zu64 size;
        zu8 flag;
    };

public:
    ZPoolAllocator(void *pool, zu64 size) : _pool((AllocNode *)pool), _poolsize(size){
        if(_pool && _poolsize > sizeof(AllocNode)){
            _pool->size = _poolsize;
            _pool->flag = TAIL;
        }
    }

    /*! Allocates memory to hold \p count T's from the pool.
     *  Does not construct objects.
     */
    T *alloc(zu64 count = 1){
        if(!count)
            return nullptr;
        AllocNode *node = _pool;
        const zu64 nsize = sizeof(T) * count;
        while(true){
            // find unallocated node
            if(!(node->flag & ALLOC)){
                // merge node if possible
                nodeMerge(node);
                // check node size
                if(node->size >= nsize){
                    // node found
                    if(node->size > nsize){
                        // node larger than needed
                        if(node->size > (nsize + sizeof(AllocNode))){
                            // split node
                            zu64 lsize = node->size - (nsize + sizeof(AllocNode));
                            node->size = nsize;

                            // new node
                            AllocNode *next = nextNode(node);
                            next->size = lsize;
                            next->flag = 0;
                            // tail may move
                            if(node->flag & TAIL){
                                next->flag |= TAIL;
                                node->flag &= ~((zu8)TAIL);
                            }
                        }
                    }
                    // allocate and return
                    node->flag |= ALLOC;
                    return dataPtr(node);
                }
            }

            if(node->flag & TAIL){
                // this is the last node, alloc failed
                return nullptr;
            }
            node = nextNode(node);
        }
    }

    /*! Deallocate memory from the pool.
     *  Expects \p ptr originally returned by alloc().
     *  Objects should be destroy()ed first.
     */
    void dealloc(T *ptr){
        if(!ptr)
            return;
        AllocNode *node = _pool;
        while(true){
            // looking for ptr
            if(ptr == dataPtr(node)){
                // dealloc
                node->flag &= ~((zu8)ALLOC);
//                nodeMerge(node);
                return;
            }

            if(node->flag & TAIL){
                // this is the last node, ptr not found
                return;
            }
            node = nextNode(node);
        }
    }

    static void nodeMerge(AllocNode *node){
        // can't merge allocated or tail node
        if(node->flag & ALLOC || node->flag & TAIL)
            return;
        AllocNode *next = nextNode(node);
        // merge next node first
        nodeMerge(next);
        // can't merge allocated node
        if(next->flag & ALLOC)
            return;
        // merge
        node->flag = next->flag;
        node->size = node->size + sizeof(AllocNode) + next->size;
    }

    static T *dataPtr(AllocNode *node){
        return (T *)(((zbyte *)node) + sizeof(AllocNode));
    }
    static AllocNode *nextNode(AllocNode *node){
        return (AllocNode *)(((zbyte *)node) + sizeof(AllocNode) + node->size);
    }

private:
    AllocNode *_pool;
    zu64 _poolsize;
};

}

#endif // ZALLOCATORPOOL_H
