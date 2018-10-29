/*******************************************************************************
**                                  LibChaos                                  **
**                                  zmutex.h                                  **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#ifndef ZMUTEX_H
#define ZMUTEX_H

#include "ztypes.h"

#if LIBCHAOS_COMPILER == LIBCHAOS_COMPILER_MSVC
    #define ZMUTEX_WINTHREADS
#endif

namespace LibChaos {

typedef zu64 ztid;

/*! Cross-thread shared resource access synchronization controller.
 *  \ingroup Thread
 *  \warning Relatively untested.
 *  Recursize mutual exclusion object.
 *  Only the thread that locked a mutex is normally allowed to unlock it.
 *  Any thread may lock an unlocked mutex.
 *  If a thread tries to lock a mutex multiple times, it must unlock it as many times for other threads to lock it.
 *  Uses a pthread_mutex_t on POSIX.
 *  Uses a Critical Section on Windows.
*/
class ZMutex {
public:
    ZMutex();
    ~ZMutex();

    ZMutex(const ZMutex &other) = delete;
    ZMutex &operator=(const ZMutex &) = delete;

    /*! If mutex is unlocked, mutex is locked by calling thread.
     *  If mutex is locked by other thread, function blocks until mutex is unlocked by other thread, then mutex is locked by calling thread.
     */
    void lock();

    //! Locks mutex and returns true if unlocked, else returns false.
    bool trylock();

    //! Tries to lock the mutex for <milliseconds> milliseconds, then returns false.
    bool timelock(zu32 milliseconds);

    /*! If mutex is unlocked, returns true.
     *  If mutex is locked by calling thread, mutex is unlocked.
     *  If mutex is locked by other thread, blocks until mutex is unlocked by other thread.
     */
    void unlock();

private:
    struct MutexData;
    MutexData *_data;
};

// //////////////////////////////////////////////////////////////////////////////

template <class T> class ZMutexV : public ZMutex {
public:
    ZMutexV() : ZMutex(), obj(){}
    ZMutexV(const T &o) : ZMutex(), obj(o){}

    //! Block until mutex owned, return refrence to obj
    T &lockdata(){
        lock();
        return obj;
    }

    //! Return refrence to obj. Thread responsibly.
    inline T &data(){
        return obj;
    }

private:
    T obj;
};

// //////////////////////////////////////////////////////////////////////////////

//! Scope-controller ZMutex wrapper.
class ZCriticalSection {
public:
    ZCriticalSection(ZMutex *mutex) : _mutex(mutex){
        if(_mutex)
            _mutex->lock();
    }
    ZCriticalSection(ZMutex &mutex) : ZCriticalSection(&mutex){

    }
    ~ZCriticalSection(){
        if(_mutex)
            _mutex->unlock();
    }

private:
    ZMutex *_mutex;
};

} // namespace LibChaos

#endif // ZMUTEX_H
