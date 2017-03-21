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

int cmd_create(ZPath file, ZArray<ZString> args){
    ZParcel parcel;
    ZParcel::parcelerror err = parcel.create(file);
    if(err == ZParcel::OK){
        LOG("OK - Create " << file);
        return EXIT_SUCCESS;
    } else {
        LOG("FAIL - " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }
}

int cmd_list(ZPath file, ZArray<ZString> args){

    return -1;
}

int cmd_store(ZPath file, ZArray<ZString> args){
    ZUID uid = args[0];
    ZString type = args[1];
    ZString value = args[2];

    if(uid == ZUID_NIL){
        ELOG("FAIL - Invalid UUID");
        return EXIT_FAILURE;
    }

    ZParcel parcel;
    ZParcel::parcelerror err;

    err = parcel.open(file);
    if(err != ZParcel::OK){
        ELOG("FAIL - " << ZParcel::errorStr(err));
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

int cmd_fetch(ZPath file, ZArray<ZString> args){
    ZParcel parcel;
    ZParcel::parcelerror err = parcel.open(file);
    if(err != ZParcel::OK){
        ELOG("Failed to open: " << ZParcel::errorStr(err));
        return EXIT_FAILURE;
    }

    ZParcel::objtype type = parcel.getType(ZUID(args[1]));
    if(type == ZParcel::UNKNOWNOBJ){
        ELOG("FAIL - Bad object type");
        return EXIT_FAILURE;
    }

    switch(type){
        case ZParcel::NULLOBJ:
            LOG("NULL");
            break;
        case ZParcel::BOOLOBJ:
            LOG(parcel.fetchBool(ZUID(args[2])));
            break;
        case ZParcel::UINTOBJ:
            LOG(parcel.fetchUint(ZUID(args[2])));
            break;
        case ZParcel::SINTOBJ:
            LOG(parcel.fetchSint(ZUID(args[2])));
            break;
        case ZParcel::FLOATOBJ:
            LOG(parcel.fetchFloat(ZUID(args[2])));
            break;
        case ZParcel::ZUIDOBJ:
            LOG(parcel.fetchZUID(ZUID(args[2])).str());
            break;
        case ZParcel::STRINGOBJ:
            LOG(parcel.fetchString(ZUID(args[2])));
            break;
        case ZParcel::BLOBOBJ:
            LOG(parcel.fetchBlob(ZUID(args[2])));
            break;
        case ZParcel::FILEOBJ: {
            zu64 offset;
            zu64 size;
            ZString name = parcel.fetchFile(ZUID(args[1]), &offset, &size);
            ZFile pfile = parcel.getHandle();
            pfile.seek(offset);

            ZFile ofile(ZFile::STDOUT);
            ZBinary buff;
            zu64 len = 0;
            while(len < size){
                buff.clear();
                size = size - pfile.read(buff, MIN(1 << 15, size));
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

int cmd_delete(ZPath file, ZArray<ZString> args){
    return EXIT_FAILURE;
}

const ZArray<ZOptions::OptDef> optdef = {};

typedef int (*cmd_func)(ZPath file, ZArray<ZString> args);
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
    { "delete", { cmd_delete,   1, "zparcel <file> delete <id>" } },
};

int mainwrap(int argc, char **argv){



    return EXIT_SUCCESS;
}

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
        ZPath file = args[0];
        args.popFront();

        if(args.size()){
            ZString cmstr = args[0];
            if(cmds.contains(cmstr)){
                CmdEntry cmd = cmds[cmstr];
                args.popFront();
                if(args.size() == cmd.argn){
                    cmd.func(file, args);
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
