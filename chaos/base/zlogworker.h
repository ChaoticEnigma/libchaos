/*******************************************************************************
**                                  LibChaos                                  **
**                                zlogworker.h                                **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#ifndef ZLOGWORKER_H
#define ZLOGWORKER_H

#include "zstring.h"
#include "zbinary.h"
#include "zpath.h"
#include "zqueue.h"
#include "zmap.h"

#include "zthread.h"
#include "zmutex.h"
#include "zcondition.h"

#include "ztime.h"
#include "zclock.h"

// Default Log formats
#define LOGONLY     "%log%"
#define TIMELOG     "%time% - %log%"
#define TIMETHREAD  "%time% %thread% - %log%"
#define DETAILLOG   "%time% %thread% %function% (%file%:%line%) - %log%"

namespace LibChaos {

//! ZLog entry handler, processor, formatter, writer.
class ZLogWorker : private ZThread::ZThreadContainer {
public:
    struct LogJob {
        int level;

        ZTime time;
        ZClock clock;
        ztid thread;

        ZString file;
        ZString line;
        ZString func;

        bool stdio;
        bool newln;
        bool raw;
        ZString log;
    };

    struct LogHandler {
        enum lhtype {
           STDOUT, STDERR, FILE
        };
        LogHandler(lhtype t, ZString f) : type(t), fmt(f){}
        lhtype type;
        ZString fmt;
        ZPath file;
    };

    ZLogWorker();
    ~ZLogWorker();

    void startWorker();
    void stopWorker();

    void queue(LogJob *job);
    void doLog(LogJob *job);

    void logLevelStdOut(int type, ZString fmt);
    void logLevelStdErr(int level, ZString fmt);
    void logLevelFile(int level, ZPath file, ZString fmt);

    void setStdOutEnable(bool set);
    void setStdErrEnable(bool set);

private:
    void *run(void *zarg);
    static void sigHandle(int sig);

    static ZString getThread(ztid thread);
    static ZString makeLog(const LogJob *job, ZString fmt);

    ZThread worker;

    ZMutex jobmutex;
    ZCondition jobcondition;
    ZQueue<LogJob*> jobs;

    ZMutex formatmutex;
    ZMap<int, ZList<LogHandler>> logformats;

    bool enablestdout = true;
    bool enablestderr = true;
};

} // namespace LibChaos

#endif // ZLOGWORKER_H
