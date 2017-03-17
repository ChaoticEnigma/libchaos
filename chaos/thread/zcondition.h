/*******************************************************************************
**                                  LibChaos                                  **
**                                zcondition.h                                **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#ifndef ZCONDITION_H
#define ZCONDITION_H

#include "ztypes.h"
#include "zmutex.h"

#ifdef ZMUTEX_WINTHREADS
    struct _RTL_CONDITION_VARIABLE;
    typedef _RTL_CONDITION_VARIABLE CONDITION_VARIABLE;
#endif

namespace LibChaos {

/*! Thread notification controller.
 *  \ingroup Thread
 */
class ZCondition {
public:
    enum condoption {
        PSHARE = 1,
    };

public:
    ZCondition(int options = 0);
    ~ZCondition();

    ZCondition(const ZCondition &) = delete;
    ZCondition &operator=(const ZCondition &) = delete;

    void waitOnce();

    void lock();
    void wait();
    void unlock();

    void signal();

    void broadcast();

#ifdef ZMUTEX_WINTHREADS
    inline CONDITION_VARIABLE *getHandle(){
        return cond;
    }
    inline CRITICAL_SECTION *getMutex(){
        return mutex;
    }
#else
    inline pthread_cond_t *getHandle(){
        return &_cond;
    }
    inline pthread_mutex_t *getMutex(){
        return &_mutex;
    }
#endif

private:
#ifdef ZMUTEX_WINTHREADS
    CRITICAL_SECTION *mutex;
    CONDITION_VARIABLE *cond;
#else
    pthread_mutex_t _mutex;
    pthread_mutexattr_t _mattr;
    pthread_cond_t _cond;
    pthread_condattr_t _cattr;
#endif
};

} // namespace LibChaos

#endif // ZCONDITION_H
