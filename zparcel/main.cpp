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

    { "int",    ZParcel::SINTOBJ },

    { "sint",   ZParcel::SINTOBJ },

    { "float",  ZParcel::FLOATOBJ },
    { "double", ZParcel::FLOATOBJ },

    { "uid",    ZParcel::ZUIDOBJ },
    { "uuid",   ZParcel::ZUIDOBJ },
    { "zuid",   ZParcel::ZUIDOBJ },

    { "bin",    ZParcel::BLOBOBJ },
    { "blob",   ZParcel::BLOBOBJ },
    { "binary", ZParcel::BLOBOBJ },

    { "str",    ZParcel::STRINGOBJ },
    { "string", ZParcel::STRINGOBJ },

    { "list",   ZParcel::LISTOBJ },

    { "file",   ZParcel::FILEOBJ },
};

ZUID argNewUID(ZString str){
    if(str == "time")
        return ZUID(ZUID::TIME);
    else if(str == "random")
        return ZUID(ZUID::RANDOM);
    return ZUID(str);
}

int cmd_create(ZFile *file, ZArray<ZString> args){
    ZParcel parcel;
    auto err = parcel.create(file, ZParcel::OPT_TAIL_EXTEND);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    LOG("OK - Create " << file->path());
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
    ZUID uid = argNewUID(args[0]);
    ZString stype = args[1];
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

    if(!nametotype.contains(stype)){
        LOG("FAIL - Unknown type");
        return EXIT_FAILURE;
    }

    auto type = nametotype[stype];
    switch(type){
        case ZParcel::BOOLOBJ:
            err = parcel.storeBool(uid, (value == "true" ? true : false));
            break;
        case ZParcel::UINTOBJ:
            err = parcel.storeUint(uid, value.toUint());
            break;
        case ZParcel::SINTOBJ:
            err = parcel.storeSint(uid, value.tint());
            break;
        case ZParcel::FLOATOBJ:
            err = parcel.storeFloat(uid, value.toFloat());
            break;
        case ZParcel::ZUIDOBJ:
            err = parcel.storeZUID(uid, value);
            break;
        case ZParcel::BLOBOBJ: {
            ZBinary bin;
            ZFile::readBinary(value, bin);
            err = parcel.storeBlob(uid, bin);
            break;
        }
        case ZParcel::STRINGOBJ:
            err = parcel.storeString(uid, value);
            break;
        case ZParcel::LISTOBJ: {
            ZList<ZUID> list;
            auto strs = value.explode(',');
            for(auto it = strs.begin(); it.more(); ++it){
                ZUID id(it.get());
                if(id == ZUID_NIL){
                    LOG("FAIL - Invalid UUID");
                    return EXIT_FAILURE;
                }
                list.push(id);
            }
            err = parcel.storeList(uid, list);
            break;
        }
        case ZParcel::FILEOBJ:
            err = parcel.storeFile(uid, value);
            break;

        default:
            LOG("FAIL - Unknown type");
            return EXIT_FAILURE;
            break;
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
        case ZParcel::BLOBOBJ: {
            ZFile sout(ZFile::STDOUT);
            sout.write(parcel.fetchBlob(uid));
            break;
        }
        case ZParcel::FILEOBJ: {
            ZUID nid;
            ZUID did;
            parcel.fetchFile(uid, nid, did);

            ZString path = parcel.fetchString(nid);
            if(ZFile::exists(path)){
                LOG("FAIL - File \"" << path << "\" exists");
                return EXIT_FAILURE;
            }

            ZFile ofile(path, ZFile::WRITE);
            if(!ofile.isOpen()){
                LOG("FAIL - Failed to open for output");
                return EXIT_FAILURE;
            }

            ZBinary blob = parcel.fetchBlob(did);
            if(ofile.write(blob) != blob.size()){
                LOG("FAIL - Writing file failed");
                return EXIT_FAILURE;
            }

            LOG("OK - " << path);

//            ZBinary buff;
//            while(!accessor->atEnd()){
//                buff.resize(1 << 15);
//                zu64 len = accessor->read(buff.raw(), buff.size());
//                buff.resize(len);
//                ZASSERT(ofile.write(buff), "write failed");
//            }
            break;
        }
        default:
            ELOG("Unknown type: " << ZParcel::typeName(type));
            return EXIT_FAILURE;
            break;
    }
    return EXIT_SUCCESS;
}

int cmd_show(ZFile *file, ZArray<ZString> args){
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

    LOG("UID: " << uid.str());

    auto type = parcel.getType(uid);
    if(type == ZParcel::UNKNOWNOBJ){
        ELOG("FAIL - Bad object type");
        return EXIT_FAILURE;
    }

    LOG("Type: " << ZParcel::typeName(type));

    switch(type){
        case ZParcel::NULLOBJ:
            LOG("NULL");
            break;
        case ZParcel::BOOLOBJ:
            LOG("Value: " << parcel.fetchBool(uid));
            break;
        case ZParcel::UINTOBJ:
            LOG("Value: " << parcel.fetchUint(uid));
            break;
        case ZParcel::SINTOBJ:
            LOG("Value: " << parcel.fetchSint(uid));
            break;
        case ZParcel::FLOATOBJ:
            LOG("Value: " << parcel.fetchFloat(uid));
            break;
        case ZParcel::ZUIDOBJ:
            LOG("Value: " << parcel.fetchZUID(uid).str());
            break;
        case ZParcel::BLOBOBJ:
            LOG("Data Size: " << parcel.fetchBlob(uid).size() << " bytes");
            break;
        case ZParcel::STRINGOBJ:
            LOG("Data: " << parcel.fetchString(uid));
            break;
        case ZParcel::LISTOBJ: {
            auto list = parcel.fetchList(uid);
            LOG("List:");
            for(auto it = list.begin(); it.more(); ++it){
                ZUID id = it.get();
                LOG("  " << id.str() << " " << ZParcel::typeName(parcel.getType(id)));
            }
            break;
        }
        case ZParcel::FILEOBJ: {
            ZUID nid;
            ZUID did;
            parcel.fetchFile(uid, nid, did);

            LOG("Name UID: " << nid.str());
            LOG("Data UID: " << did.str());
            LOG("Name: " << parcel.fetchString(nid));
            LOG("Data Size: " << parcel.fetchBlob(did).size() << " bytes");
            break;
        }

        default:
            LOG("FAIL - Unknown type: " << ZParcel::typeName(type));
            return EXIT_FAILURE;
            break;
    }
    return EXIT_SUCCESS;
}

int cmd_root(ZFile *file, ZArray<ZString> args){
    ZUID uid;
    if(args.size()){
        uid = args[0];
        if(uid == ZUID_NIL){
            ELOG("FAIL - Invalid UUID");
            return EXIT_FAILURE;
        }
    }

    ZParcel parcel;
    auto err = parcel.open(file);
    if(err != ZParcel::OK){
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    if(args.size()){
        err = parcel.setRoot(uid);
        if(err != ZParcel::OK){
            LOG("FAIL - " << ZParcel::errorStr(err));
            return EXIT_FAILURE;
        }
        LOG("OK - Set Root");
    } else {
        LOG(parcel.getRoot().str());
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
    auto err = parcel.create(file, ZParcel::OPT_TAIL_EXTEND);
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
        LOG(id.str() << " OK " << tst);
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
    bool argreq;
    ZString usage;
};

const ZMap<ZString, CmdEntry> cmds = {
    { "create", { cmd_create,   0, true,  "zparcel <file> create" } },
    { "list",   { cmd_list,     0, true,  "zparcel <file> list" } },
    { "store",  { cmd_store,    3, true,  "zparcel <file> store <id> <type> <value>" } },
    { "fetch",  { cmd_fetch,    1, true,  "zparcel <file> fetch <id>" } },
    { "show",   { cmd_show,     1, true,  "zparcel <file> show <id>" } },
    { "remove", { cmd_remove,   1, true,  "zparcel <file> remove <id>" } },
    { "root",   { cmd_root,     0, false, "zparcel <file> root [id]" } },
    { "test",   { cmd_test,     0, true,  "zparcel <file> test" } },
};

int main(int argc, char **argv){
    ZLog::logLevelStdOut(ZLog::INFO, "%log%");
    ZLog::logLevelStdOut(ZLog::DEBUG, "%time% (%file%:%line%) - %log%");
//    ZLog::logLevelStdOut(ZLog::DEBUG, "");
    ZLog::logLevelStdErr(ZLog::ERRORS, "%time% (%file%:%line%) - %log%");

    try {
        ZOptions options(optdef);
        if(!options.parse(argc, argv) || options.getArgs().size() < 2){
            LOG("Usage: zparcel <parcel file> <command> [command_args]");
            return -1;
        }

        auto args = options.getArgs();
        ZPath path = args[0];
        ZString cmstr = args[1];
        if(!cmds.contains(cmstr)){
            LOG("Unknown Command \"" << cmstr << "\"");
            return -1;
        }
        args.popFrontCount(2);

        CmdEntry cmd = cmds[cmstr];
        if(!cmd.argreq || args.size() == cmd.argn){
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

    } catch(ZException err){
        ELOG(err.what());
        return EXIT_FAILURE;
    }
}
