#include "tests.h"
#include "zexception.h"
#include "zworkqueue.h"

#include <stdio.h>

using namespace LibChaosTest;

class TestRunner : public ZThread::ZThreadContainer {
    // ZThreadContainer interface
private:
    void *run(void *arg){
        LOG("Test Thread Start");
        Test *test;
        while(!stop()){
            test = queue->getWork();

        }
        return nullptr;
    }

private:
    ZWorkQueue<Test*> *queue;
};

//! Add a test to testout and testmapout, resolving and adding recursive dependencies first.
void addTest(Test &test, ZMap<ZString, Test*> &testmap, ZArray<Test> &testout, ZMap<ZString, Test> &testmapout){
    // Check for dependencies
    for(auto i = test.pre_deps.begin(); i.more(); ++i){
        if(testmap.contains(*i)){
            addTest(*testmap[*i], testmap, testout, testmapout);
        } else {
            LOG("Skipping unknown dependency " << *i);
        }
    }
    // Add test
    if(!testmapout.contains(test.name)){
        testout.push(test);
        testmapout.add(test.name, test);
    }
}

//! Run tests as specified on the command line.
int main(int argc, char **argv){
    try {
        //ZLog::init(); // BUG: threaded zlog sometimes crashes
        ZLog::defaultWorker()->logLevelStdOut(ZLog::INFO, "%clock% %thread% N %log%");
        ZLog::defaultWorker()->logLevelStdOut(ZLog::DEBUG, "\x1b[35m%clock% %thread% D %log%\x1b[m");
        ZLog::defaultWorker()->logLevelStdErr(ZLog::ERRORS, "\x1b[31m%clock% %thread% E [%function%|%file%:%line%] %log%\x1b[m");
        ZPath lgf = ZPath("logs") + ZLog::genLogFileName("testchaos_");
        ZLog::defaultWorker()->logLevelFile(ZLog::INFO, lgf, "%time% %thread% N %log%");
        ZLog::defaultWorker()->logLevelFile(ZLog::DEBUG, lgf, "%time% %thread% D [%function%|%file%:%line%] %log%");
        ZLog::defaultWorker()->logLevelFile(ZLog::ERRORS, lgf, "%time% %thread% E [%function%|%file%:%line%] %log%");

        LOG("Testing LibChaos: " << LibChaosDescribe());
        zu64 conf = LibChaosBuildConfig();
        const ZArray<ZString> comp = { "NONE", "GCC", "MinGW", "Clang", "MSVC" };
        const ZArray<ZString> plat = { "NONE", "Linux", "FreeBSD", "Windows", "MacOSX", "Cygwin" };
        const ZArray<ZString> type = { "NONE", "Debug", "Release", "Release+Debug" };
        LOG("Library Config: " << ZString::ItoS(conf, 16)
            << " " << comp[(conf >> 16) & 0xf]
            << " " << plat[(conf >> 8) & 0xf]
            << " " << type[(conf) & 0xf]
        );

        // List tests
        ZArray<Test> alltests;
        for(auto i = regtests.cbegin(); i.more(); ++i)
            alltests.append(i.get()());

        // Build map
        ZMap<ZString, Test*> testmap;
        for(auto i = alltests.begin(); i.more(); ++i)
            testmap.add(i.get().name, &i.get());

        // Command line arguments
        bool predisable = true;
        bool hideout = true;
        for(int i = 1; i < argc; ++i){
            ZString arg = argv[i];
            if(arg == "-v"){
                hideout = false;
            } else if(arg == "++"){
                // Enable all tests
                for(auto j = alltests.begin(); j.more(); ++j)
                    j.get().run = true;
            } else if(arg == "--"){
                // Disable all tests
                for(auto j = alltests.begin(); j.more(); ++j)
                    j.get().run = false;
            } else if(arg.beginsWith("+")){
                // Enable specified test
                ZString name = ZString::substr(arg, 1);
                if(testmap.contains(name))
                    testmap[name]->run = true;
                else
                    LOG("No test: " << name);
            } else if(arg.beginsWith("-")){
                // Disable specified test
                ZString name = ZString::substr(arg, 1);
                if(testmap.contains(name))
                    testmap[name]->run = false;
                else
                    LOG("No test: " << name);
            } else {
                // Enable all tests beginning with argument, disable all other tests
                if(predisable){
                    for(auto j = alltests.begin(); j.more(); ++j)
                        j.get().run = false;
                    predisable = false;
                }
                for(auto j = alltests.begin(); j.more(); ++j){
                    if(j.get().name.beginsWith(arg))
                        j.get().run = true;
                }
            }
        }

        // Add tests and resolve dependencies
        ZArray<Test> tests;
        ZMap<ZString, Test> testm;
        for(auto it = alltests.begin(); it.more(); ++it){
            if(it.get().run)
                addTest(it.get(), testmap, tests, testm);
        }

//        ZWorkQueue<Test *> testqueue;
//        TestRunner tr;
//        ZArray<ZPointer<ZThread>> threads;
//        for(zsize i = 0; i < 1; ++i){
//            threads.push(new ZThread(&tr));
//        }
//        for(zsize i = 0; i < 1; ++i){
//            threads[i]->join();
//        }

        // Run tests
        ZMap<ZString, test_status> teststatus;
        zu64 failed = 0;
        for(zu64 i = 0; i < tests.size(); ++i){
            Test test = tests[i];
            ZString status = " PASS";

            bool skip = false;
            for(auto j = test.pre_deps.begin(); j.more(); ++j){
                if(teststatus.contains(*j) && teststatus[*j] == FAIL){
                    skip = true;
                    status = ZString("-SKIP: ") + *j;
                }
            }

            LOG("* " << ZString(i).lpad(' ', 2) << " " << ZString(test.name).pad(' ', 30) << (hideout ? ZLog::NOLN : ZLog::NEWLN));

            ZClock clock;
            if(!skip && test.func){
                if(hideout){
                    ZLog::defaultWorker()->setStdOutEnable(false);
                    ZLog::defaultWorker()->setStdErrEnable(false);
                }
                try {
                    clock.start();
                    test.func();
                    clock.stop();
                    // PASS
                    teststatus[test.name] = PASS;
                } catch(int e){
                    // FAIL
                    status = ZString("!FAIL: line ") + e;
                    teststatus[test.name] = FAIL;
                    ++failed;
                } catch(ZException e){
                    // FAIL
                    status = ZString("!FAIL: ZException: ") + e.what();
                    teststatus[test.name] = FAIL;
                    ++failed;
                } catch(zexception e){
                    // FAIL
                    status = ZString("!FAIL: zexception: ") + e.what;
                    teststatus[test.name] = FAIL;
                    ++failed;
                }
                if(hideout){
                    ZLog::defaultWorker()->setStdOutEnable(true);
                    ZLog::defaultWorker()->setStdErrEnable(true);
                }
            }

            ZString result = status;
            if(teststatus.contains(test.name) && teststatus[test.name] == PASS){
                // Test pass
                double time = clock.getSecs();
                ZString unit = "s";
                if(time < 1){
                    time = time * 1000;
                    unit = "ms";
                    if(time < 1){
                        time = time * 1000;
                        unit = "us";
                    }
                }
                result = result + " [" + time + " " + unit + "]";
            }

            if(hideout)
                RLOG(result << ZLog::NEWLN);
            else
                LOG("* " << ZString(i).lpad(' ', 2) << " " << ZString(test.name).pad(' ', 30) << result);

        }

        LOG("Result: " << tests.size() - failed << "/" << tests.size() << " passed, " << failed << " failed");

        // Return number of tests failed
        return failed;

    } catch(ZException e){
        printf("Catastrophic Failure: %s - %d\n%s\n", e.what().cc(), e.code(), e.traceStr().cc());
    } catch(zexception e){
        printf("CATACLYSMIC FAILURE: %s\n", e.what);
    } catch(zallocator_exception e){
        printf("Allocator Failure: %s\n", e.what);
    }

    // Fatal error, early exit
    return -1;
}
