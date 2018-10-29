#include "tests.h"

#include "zthread.h"
#include "zmutex.h"
#include "zlock.h"

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
    #include <windows.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <assert.h>
    #include <iostream>
#endif

#define RET_MAGIC 0x5a5b5c5d

#define MUTEX_TEST_NTHREAD  1000
#define MUTEX_TEST_NLOCK    1000

namespace LibChaosTest {

void *thread_func2(ZThread::ZThreadArg zarg){
    ztid *arg = (ztid *)zarg.arg;
    *arg = ZThread::thisTid();
    LOG("running " << *arg);
    return (void *)RET_MAGIC;
}

void thread(){
    ztid ttid = 0;
    ZThread thr2(thread_func2);
    LOG("starting thread");
    thr2.exec(&ttid);
    void *ret = thr2.join();
    LOG("joined thread");
    TASSERT(ttid != ZThread::thisTid());
    TASSERT((zu64)ret == RET_MAGIC);
}

struct MutexTest {
    ZMutex mutex;
    zu32 count;
};

void *mutex_thread_func(ZThread::ZThreadArg zarg){
    MutexTest *test = (MutexTest *)zarg.arg;
    for(int i = 0; i < MUTEX_TEST_NLOCK; ++i){
        test->mutex.lock();
        // try to add time between load and store
        zu32 tmp = test->count;
        tmp += 5;
        tmp -= 5;
        test->count = tmp + 1;
        test->mutex.unlock();
    }
    return nullptr;
}

void mutex(){
    MutexTest test;
    test.count = 0;

    ZList<ZPointer<ZThread>> threads;
    for(int i = 0; i < MUTEX_TEST_NTHREAD; ++i){
        threads.push(new ZThread(mutex_thread_func));
    }

    for(auto it = threads.begin(); it.more(); ++it){
        it.get()->exec(&test);
    }

    for(auto it = threads.begin(); it.more(); ++it){
        it.get()->join();
    }

    LOG("Count: " << test.count);
    TASSERT(test.count == MUTEX_TEST_NTHREAD * MUTEX_TEST_NLOCK);
}

ZArray<Test> thread_tests(){
    return {
        { "thread", thread, true, {} },
        { "mutex",  mutex,  true, {} },
    };
}

}
