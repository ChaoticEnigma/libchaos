/*******************************************************************************
**                                  LibChaos                                  **
**                              zparcel/main.cpp                              **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "main.h"
#include "zexception.h"
#include "zlog.h"
#include "zoptions.h"

using namespace LibChaos;

static const ZMap<ZString, ZParcel::objtype> nametotype = {
    { "null",   ZParcel::NULLOBJ },
    { "uint",   ZParcel::UINTOBJ },
    { "sint",   ZParcel::SINTOBJ },
    { "float",  ZParcel::FLOATOBJ },
    { "uid",    ZParcel::ZUIDOBJ },
    { "zuid",   ZParcel::ZUIDOBJ },
    { "str",    ZParcel::STRINGOBJ },
    { "string", ZParcel::STRINGOBJ },
    { "file",   ZParcel::FILEOBJ },
    { "blob",   ZParcel::BLOBOBJ },
    { "binary", ZParcel::BLOBOBJ },
};

int cmd_create(ZFile *file, ZArray<ZString> args){
    ZParcel parcel;
    auto err = parcel.create(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    LOG("OK - Create " << file);
    return EXIT_SUCCESS;
}

int cmd_list(ZFile *file, ZArray<ZString> args){
    ZParcel parcel;
    auto err = parcel.open(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    parcel.listObjects();

    return EXIT_SUCCESS;
}

int cmd_store(ZFile *file, ZArray<ZString> args){
    ZUID uid = args[0];
    ZString type = args[1];
    ZString value = args[2];

    if(uid == ZUID_NIL){
        ELOG("FAIL - Invalid UUID");
        return EXIT_FAILURE;
    }

    ZParcel parcel;
    auto err = parcel.open(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    if(type == "null"){
        err = parcel.storeNull(uid);

    } else if(type == "bool"){
        err = parcel.storeBool(uid, (value == "true" ? true : false));

    } else if(type == "uint"){
        err = parcel.storeUint(uid, value.toUint());

    } else if(type == "int" ||
              type == "sint"){
        err = parcel.storeSint(uid, value.tint());

    } else if(type == "float" ||
              type == "double"){
        err = parcel.storeFloat(uid, value.toFloat());

    } else if(type == "uid" ||
              type == "uuid" ||
              type == "zuid"){
        err = parcel.storeZUID(uid, value);

    } else if(type == "str" ||
              type == "string"){
        err = parcel.storeString(uid, value);

    } else if(type == "bin" ||
              type == "blob" ||
              type == "binary"){
        ZBinary bin;
        ZFile::readBinary(value, bin);
        err = parcel.storeBlob(uid, bin);

    } else if(type == "file"){
        err = parcel.storeFile(uid, value);

    } else {
        ELOG("FAIL - Unknown type");
        return EXIT_FAILURE;
    }

    if(err != ZParcel::OK){
        ELOG("FAIL - Failed to store: " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    LOG("OK " << uid.str());
    return EXIT_SUCCESS;
}

int cmd_fetch(ZFile *file, ZArray<ZString> args){
    ZUID uid = args[0];
    if(uid == ZUID_NIL){
        ELOG("FAIL - Invalid UUID");
        return EXIT_FAILURE;
    }

    ZParcel parcel;
    auto err = parcel.open(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    auto type = parcel.getType(uid);
    if(type == ZParcel::UNKNOWNOBJ){
        ELOG("FAIL - Bad object type");
        return EXIT_FAILURE;
    }

    switch(type){
        case ZParcel::NULLOBJ:
            LOG("NULL");
            break;
        case ZParcel::BOOLOBJ:
            LOG(parcel.fetchBool(uid));
            break;
        case ZParcel::UINTOBJ:
            LOG(parcel.fetchUint(uid));
            break;
        case ZParcel::SINTOBJ:
            LOG(parcel.fetchSint(uid));
            break;
        case ZParcel::FLOATOBJ:
            LOG(parcel.fetchFloat(uid));
            break;
        case ZParcel::ZUIDOBJ:
            LOG(parcel.fetchZUID(uid).str());
            break;
        case ZParcel::STRINGOBJ:
            LOG(parcel.fetchString(uid));
            break;
        case ZParcel::BLOBOBJ:
            LOG(parcel.fetchBlob(uid));
            break;
        case ZParcel::FILEOBJ: {
            ZString name;
            auto accessor = parcel.fetchFile(uid, name);

            ZFile ofile(ZFile::STDOUT);
            ZBinary buff;
            while(!accessor->atEnd()){
                buff.resize(1 << 15);
                zu64 len = accessor->read(buff.raw(), buff.size());
                buff.resize(len);
                ZASSERT(ofile.write(buff), "write failed");
            }
            break;
        }
        default:
            ELOG("Unknown type: " << ZParcel::typeName(type));
            return EXIT_FAILURE;
            break;
    }
    return EXIT_SUCCESS;
}

int cmd_remove(ZFile *file, ZArray<ZString> args){
    ZUID uid = args[0];
    if(uid == ZUID_NIL){
        ELOG("FAIL - Invalid UUID");
        return EXIT_FAILURE;
    }

    ZParcel parcel;
    auto err = parcel.open(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    err = parcel.removeObject(uid);
    if(err != ZParcel::OK){
        ELOG("Failed to open: " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    LOG("OK " << uid.str());
    return EXIT_SUCCESS;
}

int cmd_test(ZFile *file, ZArray<ZString> args){
    ZParcel parcel;
    auto err = parcel.create(file);
    if(err != ZParcel::OK){
        ELOG("Failed to open: " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    ZList<ZUID> ids;

    for(zu64 i = 0; i < 100; ++i){
        ZString tst = "test string ";
        tst += i;
        ZUID id(ZUID::RANDOM);
        ids.push(id);
        err = parcel.storeString(id, tst);
        if(err != ZParcel::OK){
            ELOG("FAIL " << ZParcel::errorStr(err));
            return EXIT_FAILURE;
        }
    }

    for(auto it = ids.begin(); it.more(); ++it){
        LOG(it.get().str() << " " << parcel.fetchString(it.get()));
    }

//    parcel.listObjects();

    return EXIT_SUCCESS;
}

const ZArray<ZOptions::OptDef> optdef = {};

typedef int (*cmd_func)(ZFile *file, ZArray<ZString> args);

struct CmdEntry {
    cmd_func func;
    unsigned argn;
    ZString usage;
};

const ZMap<ZString, CmdEntry> cmds = {
    { "create", { cmd_create,   0, "zparcel <file> create" } },
    { "list",   { cmd_list,     0, "zparcel <file> list" } },
    { "store",  { cmd_store,    3, "zparcel <file> store <id> <type> <value>" } },
    { "fetch",  { cmd_fetch,    1, "zparcel <file> fetch <id>" } },
    { "remove", { cmd_remove,   1, "zparcel <file> remove <id>" } },
    { "test",   { cmd_test,     0, "zparcel <file> test" } },
};

int main(int argc, char **argv){
    ZLog::logLevelStdOut(ZLog::INFO, "%log%");
    ZLog::logLevelStdOut(ZLog::DEBUG, "%time% (%file%:%line%) - %log%");
    ZLog::logLevelStdErr(ZLog::ERRORS, "%time% (%file%:%line%) - %log%");

    try {
        ZOptions options(optdef);
        if(!options.parse(argc, argv)){
            LOG("Usage: zparcel <file> <command> [command_args]");
            return-1;
        }

        auto args = options.getArgs();
        if(args.size()){
            ZPath path = args[0];
            args.popFront();

            ZString cmstr = args[0];
            if(cmds.contains(cmstr)){
                CmdEntry cmd = cmds[cmstr];
                args.popFront();
                if(args.size() == cmd.argn){
                    ZFile file;
                    if(!file.open(path, ZFile::READWRITE)){
                        LOG("Failed to open " << path);
                        return -5;
                    }

                    return cmd.func(&file, args);
                } else {
                    LOG("Usage: " << cmd.usage);
                    return -4;
                }
            } else {
                LOG("Unknown Command \"" << cmstr << "\"");
                return -3;
            }
        } else {
            LOG("No Command");
            return -2;
        }

    } catch(ZException err){
        ELOG(err.what());
        return EXIT_FAILURE;
    }
}
