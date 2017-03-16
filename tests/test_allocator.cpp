#include "tests.h"
#include "zallocator.h"
#include "zpoolallocator.h"

namespace LibChaosTest {

void allocator_char(){
    ZAllocator<char> alloc;
    char *test = alloc.alloc(20);
    alloc.dealloc(test);
}

void allocator_void(){
    ZAllocator<void> alloc2;
    void *test2 = alloc2.alloc(20);
    alloc2.dealloc(test2);
}

void allocator_pool(){
    ZBinary pool;
    pool.fill(0, 1024);

    ZPoolAllocator<zbyte> *palloc = new ZPoolAllocator<zbyte>(pool.raw(), pool.size());
    TASSERT(palloc);

    const zu8 tnum = 10;
    zbyte *ptrs[tnum];

    LOG("1");
    palloc->debug();

    for(zu8 i = 0; i < tnum; ++i){
        const zu16 tsize = 32;
        zbyte *ptr = palloc->alloc(tsize);
        TASSERT(ptr);
        memset(ptr, i+1, tsize);
        ptrs[i] = ptr;
    }

    LOG("2");
    palloc->debug();
//    RLOG(pool.dumpBytes());

    for(zu8 i = 0; i < tnum; ++i){
        palloc->dealloc(ptrs[i]);
    }

    LOG("3");
    palloc->debug();
//    RLOG(pool.dumpBytes());

    const zu16 tsize = 64;
    zbyte *ptr = palloc->alloc(tsize);
    TASSERT(ptr);
    memset(ptr, 0xff, tsize);

    LOG("4");
    palloc->debug();
//    RLOG(pool.dumpBytes());

    palloc->dealloc(ptr);

    LOG("5");
    palloc->debug();
//    RLOG(pool.dumpBytes());

    delete palloc;
}

ZArray<Test> allocator_tests(){
    return {
        { "allocator-char", allocator_char, true, { "allocator-void" } },
        { "allocator-void", allocator_void, true, {} },
        { "allocator-pool", allocator_pool, true, {} },
    };
}

}
