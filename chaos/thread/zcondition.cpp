/*******************************************************************************
**                                  LibChaos                                  **
**                               zcondition.cpp                               **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#include "zcondition.h"
#include "zexception.h"

#ifdef ZMUTEX_WINTHREADS
    #include <windows.h>
#else
    #include <pthread.h>
#endif

namespace LibChaos {

#ifdef ZMUTEX_WINTHREADS
ZCondition::ZCondition() : mutex(new CRITICAL_SECTION), cond(new CONDITION_VARIABLE){
    InitializeCriticalSection(mutex);
    InitializeConditionVariable(cond);
}
#else
ZCondition::ZCondition(int options){
    pthread_mutexattr_init(&_mattr);
    pthread_condattr_init(&_cattr);

    if(options & PSHARE){
        pthread_mutexattr_setpshared(&_mattr, PTHREAD_PROCESS_SHARED);
        pthread_condattr_setpshared(&_cattr, PTHREAD_PROCESS_SHARED);
    }

    pthread_mutex_init(&_mutex, &_mattr);
    pthread_cond_init(&_cond, &_cattr);
}
#endif

ZCondition::~ZCondition(){
#ifdef ZMUTEX_WINTHREADS
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
    pthread_mutexattr_destroy(&_mattr);
    pthread_condattr_destroy(&_cattr);
#endif
}

void ZCondition::waitOnce(){
#ifdef ZMUTEX_WINTHREADS
    EnterCriticalSection(mutex);
    SleepConditionVariableCS(cond, mutex, INFINITE);
    LeaveCriticalSection(mutex);
#else
    pthread_mutex_lock(&_mutex);
    pthread_cond_wait(&_cond, &_mutex);
    pthread_mutex_unlock(&_mutex);
#endif
}

void ZCondition::lock(){
#ifdef ZMUTEX_WINTHREADS
    EnterCriticalSection(mutex);
#else
    pthread_mutex_lock(&_mutex);
#endif
}

void ZCondition::wait(){
#ifdef ZMUTEX_WINTHREADS
    SleepConditionVariableCS(cond, mutex, INFINITE);
#else
    pthread_cond_wait(&_cond, &_mutex);
#endif
}

void ZCondition::unlock(){
#ifdef ZMUTEX_WINTHREADS
    LeaveCriticalSection(mutex);
#else
    pthread_mutex_unlock(&_mutex);
#endif
}

void ZCondition::signal(){
#ifdef ZMUTEX_WINTHREADS
    // FIXME: Causes crash at end of program
//    EnterCriticalSection(mutex);
    WakeConditionVariable(cond);
//    LeaveCriticalSection(mutex);
#else
//    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&_cond);
//    pthread_mutex_unlock(&mutex);
#endif
}

void ZCondition::broadcast(){
#ifdef ZMUTEX_WINTHREADS
//    EnterCriticalSection(mutex);
    WakeAllConditionVariable(cond);
//    LeaveCriticalSection(mutex);
#else
//    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&_cond);
//    pthread_mutex_unlock(&mutex);
#endif
}



}
