/*******************************************************************************
**                                  LibChaos                                  **
**                                  zmutex.cpp                                **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#include "zmutex.h"
#include "zexception.h"

#ifdef ZMUTEX_WINTHREADS
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#include <chrono>

namespace LibChaos {

#ifdef ZMUTEX_WINTHREADS

struct ZMutex::MutexData {
    HANDLE mutex;
    CRITICAL_SECTION critical;
    DWORD owner;
    zu32 depth;
};

ZMutex::ZMutex() : _data(new MutexData){
    _data->owner = 0;
    _data->depth = 0;
    _data->mutex = CreateMutex(NULL, FALSE, NULL);
    InitializeCriticalSection(&_data->critical);
}

ZMutex::~ZMutex(){
    DeleteCriticalSection(&_data->critical);
    CloseHandle(_data->mutex);
    delete _data;
}

void ZMutex::lock(){
    EnterCriticalSection(&_data->critical);
    DWORD tid = GetCurrentThreadId();
    if(tid != _data->owner){
        // This thread does not own the mutex
        LeaveCriticalSection(&_data->critical);
        while(true){
            DWORD ret = WaitForSingleObject(_data->mutex, INFINITE);
            if(ret == WAIT_OBJECT_0){
                // This thread owns the mutex
                EnterCriticalSection(&_data->critical);
                _data->owner = tid;
                break;
            }
        }
    }
    // This thread owns the mutex
    _data->depth++;
    LeaveCriticalSection(&_data->critical);
}

bool ZMutex::trylock(){
    EnterCriticalSection(&_data->critical);
    DWORD tid = GetCurrentThreadId();
    if(tid != _data->owner){
        // This thread does not own the mutex
        LeaveCriticalSection(&_data->critical);
        while(true){
            DWORD ret = WaitForSingleObject(_data->mutex, 0);
            if(ret == WAIT_OBJECT_0){
                // This thread owns the mutex
                EnterCriticalSection(&_data->critical);
                _data->owner = tid;
                break; // Mutex was locked
            } else if(ret == WAIT_TIMEOUT){
                return false; // Mutex was not locked
            }
        }
    }
    // This thread owns the mutex
    _data->depth++;
    LeaveCriticalSection(&_data->critical);
    return true;
}

bool ZMutex::timelock(zu32 milliseconds){
    EnterCriticalSection(&_data->critical);
    DWORD tid = GetCurrentThreadId();
    if(tid != _data->owner){
        LeaveCriticalSection(&_data->critical);
        while(true){
            DWORD ret = WaitForSingleObject(_data->mutex, milliseconds);
            if(ret == WAIT_OBJECT_0){
                EnterCriticalSection(&_data->critical);
                _data->owner = tid;
                break; // Mutex was locked
            } else if(ret == WAIT_TIMEOUT){
                return false; // Mutex was not locked
            }
        }
    }
    _data->depth++;
    LeaveCriticalSection(&_data->critical);
    return true;
}

void ZMutex::unlock(){
    EnterCriticalSection(&_data->critical);
    DWORD tid = GetCurrentThreadId();
    if(tid != _data->owner){
        LeaveCriticalSection(&_data->critical);
        lock(); // Make sure we own the mutex first
        EnterCriticalSection(&_data->critical);
    }
    _data->depth--;
    if(_data->depth == 0){
        ReleaseMutex(_data->mutex);
        // Mutex is unlocked
        _data->owner = 0;
    }
    LeaveCriticalSection(&_data->critical);
}

#else

struct ZMutex::MutexData {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
};

ZMutex::ZMutex() : _data(new MutexData){
    pthread_mutexattr_init(&_data->attr);
//    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_data->mutex, &_data->attr);
}

ZMutex::~ZMutex(){
    pthread_mutex_destroy(&_data->mutex);
    delete _data;
}

void ZMutex::lock(){
    pthread_mutex_lock(&_data->mutex);
    // We own the mutex
}

bool ZMutex::trylock(){
    if(pthread_mutex_trylock(&_data->mutex) == 0){
        return true; // We now own the mutex
    }
    return false; // Another thread owns the mutex
}

bool ZMutex::timelock(zu32 timeout_microsec){
    auto end = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(timeout_microsec);
    while(!trylock()){
        // Loop until trylock() succeeds OR timeout is exceeded
        if(std::chrono::high_resolution_clock::now() > end)
            return false; // Timeout exceeded, mutex is locked by another thread
    }
    return true; // Locked, and this thread owns it
}

void ZMutex::unlock(){
    pthread_mutex_unlock(&_data->mutex);
    // Mutex is unlocked
}

#endif // ZMUTEX_VERSION

}
