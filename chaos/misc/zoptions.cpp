#include "zoptions.h"
#include "zlog.h"

namespace LibChaos {

ZOptions::ZOptions(ZArray<OptDef> optdef){
    define = optdef;
}

bool ZOptions::parse(int argc, const char *const *argv){
    bool nextarg = false;
    OptDef nextdef;

    auto parseArg = [&](OptDef def, ZString arg){
        if(def.type == LIST){
            if(opts[def.name].size())
                opts[def.name] += ("," + arg);
            else
                opts[def.name] = arg;
        } else {
            opts[def.name] = arg;
        }
    };

    for(int i = 1; i < argc; ++i){
        ZString arg = argv[i];
        if(arg.beginsWith("--") && arg.size() > 2){
            // Long option
            arg.substr(2);
            bool ok = false;
            for(zu64 j = 0; j < define.size(); ++j){
                ZString pref = define[j].name + "=";
                if(arg == define[j].name){
                    if(define[j].type == NONE){
                        opts[define[j].name] = "";
                    } else {
                        nextdef = define[j];
                        nextarg = true;
                    }
                    ok = true;
                    break;
                } else if(arg.beginsWith(pref)){
                    arg.substr(pref.size());
                    parseArg(define[j], arg);
                    ok = true;
                    break;
                }
            }
            if(!ok){
                DLOG("error: unknown long option: " << arg);
                return false;
            }

        } else if(arg.beginsWith("-") && arg.size() > 1){
            // Short option
            arg.substr(1);
            bool ok = false;
            bool noarg = true;
            // multiple flags possible
            for(zu64 k = 0; noarg && k < arg.size(); ++k){
                // check options
                for(zu64 j = 0; j < define.size(); ++j){
                    if(arg[k] == define[j].flag){
                        if(define[j].type == NONE){
                            opts[define[j].name] = "";
                        } else {
                            noarg = false;
                            arg.substr(k+1);
                            if(arg.isEmpty()){
                                nextdef = define[j];
                                nextarg = true;
                            } else {
                                parseArg(define[j], arg);
                            }
                        }
                        ok = true;
                        break;
                    }
                }
                if(!ok){
                    DLOG("error: unknown short option: " << arg);
                    return false;
                }
            }

        } else if(nextarg){
            // Option argument
            parseArg(nextdef, arg);
            nextarg = false;

        } else {
            // Normal arg
            args.push(arg);
        }
    }

    if(nextarg){
        DLOG("error: no value for option: " << nextdef.name);
        return false;
    }
    return true;
}

}
