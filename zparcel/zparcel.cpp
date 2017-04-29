/*******************************************************************************
**                                  LibChaos                                  **
**                                 zparcel.cpp                                **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zparcel.h"
#include "zmap.h"
#include "zlog.h"
#include "zerror.h"

static LibChaos::zu32 ZPARCEL_MAGIC         = 0x5a504152;
static LibChaos::zu32 ZPARCEL_TREE_MAGIC    = 0x54524545;
static LibChaos::zu32 ZPARCEL_FREE_MAGIC    = 0x66726565;
static LibChaos::zu32 ZPARCEL_OBJT_MAGIC    = 0x4f424a54;

#define ZPARCEL_SIG "ZPARCEL"
#define ZPARCEL_SIG_LEN 7

#define ZPARCEL_HEADER_LEN ParcelHeader::getNodeSize()
#define ZPARCEL_NODE_LEN ParcelTreeNode::getNodeSize()

#define ZPARCEL_FREE_NODE_COUNT 4
#define ZPARCEL_MAX_DEPTH 128

#define CHECK_COMMON(STR) if(_state != OPEN){ \
    throw ZException(ZString(STR) + ": parcel not open"); \
}

namespace LibChaos {

static const ZMap<ZParcel::objtype, ZString> typetoname = {
    { ZParcel::NULLOBJ,   "null" },
    { ZParcel::UINTOBJ,   "uint" },
    { ZParcel::SINTOBJ,   "sint" },
    { ZParcel::FLOATOBJ,  "float" },
    { ZParcel::ZUIDOBJ,   "zuid" },
    { ZParcel::STRINGOBJ, "string" },
    { ZParcel::FILEOBJ,   "file" },
    { ZParcel::BLOBOBJ,   "binary" },
};

static const ZMap<ZParcel::parcelerror, ZString> errtostr = {
    { ZParcel::OK,              "OK" },
    { ZParcel::ERR_OPEN,        "Error opening file" },
    { ZParcel::ERR_SEEK,        "Error seeking in file" },
    { ZParcel::ERR_READ,        "Error reading file" },
    { ZParcel::ERR_WRITE,       "Error writing file" },
    { ZParcel::ERR_EXISTS,      "Object exists" },
    { ZParcel::ERR_NOEXIST,     "Object does not exist" },
    { ZParcel::ERR_BADCRC,      "CRC mismatch" },
    { ZParcel::ERR_TRUNC,       "Payload is truncated by end of file" },
    { ZParcel::ERR_TREE,        "Bad tree structure" },
    { ZParcel::ERR_FREELIST,    "Bad freelist structure" },
    { ZParcel::ERR_NOFREE,      "No free nodes" },
    { ZParcel::ERR_SIG,         "Bad file signature" },
    { ZParcel::ERR_VERSION,     "Bad file header version" },
    { ZParcel::ERR_MAX_DEPTH,   "Exceeded maximum tree depth" },
    { ZParcel::ERR_MAGIC,       "Bad object magic number" },
};

struct ParcelHeader {
    zbyte sig[ZPARCEL_SIG_LEN];
    zu8 version;
    zu64 treehead;
    zu64 freehead;
    zu64 freetail;
    zu64 tailptr;

    static zu64 getNodeSize(){ return ZPARCEL_SIG_LEN + 1 + 8 + 8 + 8 + 8; }

    bool read(ZBlockAccessor *file){
        file->read(sig, ZPARCEL_SIG_LEN);
        if(::memcmp(sig, ZPARCEL_SIG, ZPARCEL_SIG_LEN) != 0)
            return false;
        version = file->readu8();
        treehead = file->readbeu64();
        freehead = file->readbeu64();
        freetail = file->readbeu64();
        tailptr = file->readbeu64();
        return true;
    }

    bool write(ZBlockAccessor *file){
        file->write(sig, ZPARCEL_SIG_LEN);
        file->writeu8(version);
        file->writebeu64(treehead);
        file->writebeu64(freehead);
        file->writebeu64(freetail);
        file->writebeu64(tailptr);
        return true;
    }
};

struct ParcelTreeNode {
    ZUID uid;
    zu64 lnode;
    zu64 rnode;
    zu8 type;
    zu8 extra;
    zu16 crc;
    zbyte payload[16];

    struct {
        zu64 size;
        zu64 offset;
    } data;

    zu64 offset;

    static zu64 getNodeSize(){ return 4 + ZUID_SIZE + 8 + 8 + 2 + 2 + 16; }

    bool read(ZBlockAccessor *file){
        if(file->seek(offset) != offset)
            return false;

        zu32 magic = file->readbeu32();
        if(magic != ZPARCEL_TREE_MAGIC)
            return false;

        zbyte id[16];
        file->read(id, 16);
        uid.fromRaw(id);
        lnode = file->readbeu64();
        rnode = file->readbeu64();
        type = file->readu8();
        extra = file->readu8();
        crc = file->readbeu16();

        file->read(payload, 16);
        if(type >= ZParcel::BLOBOBJ){
            data.size = ZBinary::decbeu64(payload);
            data.offset= ZBinary::decbeu64(payload + 8);
        }

        return true;
    }

    bool write(ZBlockAccessor *file){
        if(file->seek(offset) != offset)
            return false;

        file->writebeu32(ZPARCEL_TREE_MAGIC);
        file->write(uid.raw(), 16);
        file->writebeu64(lnode);
        file->writebeu64(rnode);
        file->writeu8(type);
        file->writeu8(extra);
        file->writebeu16(crc);

        if(type >= ZParcel::BLOBOBJ){
            ZBinary::encbeu64(payload, data.size);
            ZBinary::encbeu64(payload + 8, data.offset);
        }
        file->write(payload, 16);

        return true;
    }
};

struct ZParcel::ParcelFreeNode {
    zu64 next;
    zu64 size;

    zu64 offset;
    zu64 parent;

    static zu64 getNodeSize(){ return 4 + 8 + 8; }

    ZParcel::parcelerror read(ZBlockAccessor *file){
        if(file->seek(offset) != offset)
            return ZParcel::ERR_SEEK;

        zu32 magic = file->readbeu32();
        if(magic != ZPARCEL_FREE_MAGIC)
            return ZParcel::ERR_MAGIC;

        next = file->readbeu64();
        size = file->readbeu64();
        return ZParcel::OK;
    }

    ZParcel::parcelerror write(ZBlockAccessor *file){
        if(file->seek(offset) != offset)
            return ZParcel::ERR_SEEK;

        file->writebeu32(ZPARCEL_FREE_MAGIC);
        file->writebeu64(next);
        file->writebeu64(size);
        return ZParcel::OK;
    }
};

// /////////////////////////////////////////////////////////////////////////////

ZParcel::ZParcel() : _state(CLOSED), _backing(nullptr), _version(UNKNOWN){

}

ZParcel::~ZParcel(){
    close();
}

ZParcel::parcelerror ZParcel::create(ZPath file){
    _version = VERSION1;
    _file.open(file, ZFile::READWRITE);
    if(!_file.isOpen())
        return ERR_OPEN;
    return create(&_file);
}

ZParcel::parcelerror ZParcel::create(ZBlockAccessor *file){
    _backing = file;

    _treehead = ZU64_MAX;
    _freehead = ZPARCEL_HEADER_LEN;
    _freetail = ZPARCEL_HEADER_LEN;
    _tail = ZPARCEL_HEADER_LEN;

    if(!_writeHeader(0))
        return ERR_WRITE;

    _freeNodeAdd(_tail, file->available());

    _state = OPEN;
    return OK;
}

ZParcel::parcelerror ZParcel::open(ZPath file){
    _file.open(file, ZFile::READWRITE);
    if(!_file.isOpen())
        return ERR_OPEN;
    return open(&_file);
}

ZParcel::parcelerror ZParcel::open(ZBlockAccessor *file){
    _backing = file;

    ParcelHeader header;
    _file.seek(0);
    if(!header.read(_backing))
        return ERR_READ;
//    if(::memcmp(header.sig, ZPARCEL_SIG, ZPARCEL_SIG_LEN) != 0)
//        return ERR_SIG;
    if(header.version >= MAX_PARCELTYPE)
        return ERR_VERSION;

    _version = (parceltype)header.version;
    _treehead = header.treehead;
    _freehead = header.freehead;
    _freetail = header.freetail;
    _tail = header.tailptr;

    _state = OPEN;
    return OK;
}

void ZParcel::close(){
    _file.close();
}

// /////////////////////////////////////////////////////////////////////////////

bool ZParcel::exists(ZUID id){
    ObjectInfo info;
    parcelerror err = _getObjectInfo(id, &info);
    if(err != ZParcel::OK)
        return false;
    return true;
}

ZParcel::objtype ZParcel::getType(ZUID id){
    ObjectInfo info;
    parcelerror err = _getObjectInfo(id, &info);
    if(err != ZParcel::OK)
        return UNKNOWNOBJ;
    return info.type;
}

// /////////////////////////////////////////////////////////////////////////////

ZParcel::parcelerror ZParcel::storeNull(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    return _storeObject(id, NULLOBJ, ZBinary());
}

ZParcel::parcelerror ZParcel::storeBool(ZUID id, bool bl){
    CHECK_COMMON(__FUNCTION__);
    ZBinary data(1);
    data[0] = (bl ? 1 : 0);
    return _storeObject(id, BOOLOBJ, data);
}

ZParcel::parcelerror ZParcel::storeUint(ZUID id, zu64 num){
    CHECK_COMMON(__FUNCTION__);
    ZBinary data;
    data.writebeu64(num);
    return _storeObject(id, UINTOBJ, data);
}

ZParcel::parcelerror ZParcel::storeSint(ZUID id, zs64 num){
    CHECK_COMMON(__FUNCTION__);
    ZBinary data;
    data.writebes64(num);
    return _storeObject(id, SINTOBJ, data);
}

ZParcel::parcelerror ZParcel::storeFloat(ZUID id, double num){
    CHECK_COMMON(__FUNCTION__);
    ZBinary data;
    data.writedouble(num);
    return _storeObject(id, FLOATOBJ, data);
}

ZParcel::parcelerror ZParcel::storeZUID(ZUID id, ZUID uid){
    CHECK_COMMON(__FUNCTION__);
    return _storeObject(id, ZUIDOBJ, uid.bin());
}

ZParcel::parcelerror ZParcel::storeString(ZUID id, ZString str){
    CHECK_COMMON(__FUNCTION__);
    ZBinary bin;
    bin.writebeu64(str.size());
    bin.write(str);
    return _storeObject(id, STRINGOBJ, bin);
}

ZParcel::parcelerror ZParcel::storeBlob(ZUID id, ZBinary blob){
    CHECK_COMMON(__FUNCTION__);
    ZBinary bin;
    bin.writebeu64(blob.size());
    bin.write(blob);
    return _storeObject(id, BLOBOBJ, bin);
}

ZParcel::parcelerror ZParcel::storeFile(ZUID id, ZPath path){
    CHECK_COMMON(__FUNCTION__);
    parcelerror err;

    // Open file
    ZFile infile(path, ZFile::READ);
    if(!infile.isOpen())
        return ERR_OPEN;
    // Get filesize
    zu64 filesize = infile.fileSize();

    // String object for name
    ZUID strid(ZUID::RANDOM);
    ZString name = ZPath(path).relativeTo(ZPath::pwd()).str();  // Get relative path
    err = storeString(strid, name);
    if(err != OK)
        return err;

    // Blob object for data
    ZUID dataid(ZUID::RANDOM);
    ZBinary fbin;
    fbin.writebeu64(filesize);
    err = _storeObject(dataid, BLOBOBJ, fbin, filesize);
    if(err != OK)
        return err;

    // Add node with filename
    ZBinary bin;
    bin.write(strid.bin());
    bin.write(dataid.bin());
    err = _storeObject(id, FILEOBJ, bin);
    if(err != OK)
        return err;

    // Get data offset
    ObjectInfo info;
    err = _getObjectInfo(dataid, &info);
    if(err != OK)
        return err;

    // Write file data
    zu64 poff = info.data.offset + 8;
    if(_backing->seek(poff) != poff)
        return ERR_SEEK;
    ZBinary buff;
    while(!infile.atEnd()){
        buff.clear();
        // Read 2^15 block
        if(infile.read(buff, 1 << 15) == 0)
            return ERR_READ;
        if(_backing->write(buff.raw(), buff.size()) == 0)
            return ERR_WRITE;
    }
    return OK;
}

// /////////////////////////////////////////////////////////////////////////////

bool ZParcel::fetchBool(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    parcelerror err = _getObjectInfo(id, &info);
    if(err != OK)
        throw ZException("fetchBool: object could not be fetched");
    if(info.type != BOOLOBJ)
        throw ZException("fetchBool called for wrong Object type");
    return !!info.payload[0];
}

zu64 ZParcel::fetchUint(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != UINTOBJ)
        throw ZException("fetchUint called for wrong Object type");
    return ZBinary::decbeu64(info.payload);
}

zs64 ZParcel::fetchSint(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != SINTOBJ)
        throw ZException("fetchSint called for wrong Object type");
    return (zs64)ZBinary::decbeu64(info.payload);
}

double ZParcel::fetchFloat(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != FLOATOBJ)
        throw ZException("fetchFloat called for wrong Object type");
    return ZBinary::decdouble(info.payload);
}

ZUID ZParcel::fetchZUID(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != ZUIDOBJ)
        throw ZException("fetchZUID called for wrong Object type");
    ZUID uid;
    uid.fromRaw(info.payload);
    return uid;
}

ZString ZParcel::fetchString(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != STRINGOBJ)
        throw ZException("fetchString called for wrong Object type");

    if(_backing->seek(info.data.offset) != info.data.offset)
        throw ZException("seek error");

    zu64 len = _backing->readbeu64();
    ZBinary bin(len);
    _backing->read(bin.raw(), bin.size());
    return ZString(bin.raw(), len);
}

ZBinary ZParcel::fetchBlob(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != ZUIDOBJ)
        throw ZException("fetchBlob called for wrong Object type");

    if(_backing->seek(info.data.offset) != info.data.offset)
        throw ZException("seek error");

    zu64 len = _backing->readbeu64();
    ZBinary bin(len);
    _backing->read(bin.raw(), bin.size());
    return bin;
}

ZString ZParcel::fetchFile(ZUID id, zu64 *offset, zu64 *size){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    _getObjectInfo(id, &info);
    if(info.type != FILEOBJ)
        throw ZException("fetchFile called for wrong Object type");

    if(_backing->seek(info.data.offset) != info.data.offset)
        throw ZException("seek error");

    ZBinary nibin(16);
    _backing->read(nibin.raw(), nibin.size());
    ZBinary dibin(16);
    _backing->read(dibin.raw(), dibin.size());

    ZUID nid;
    nid.fromRaw(nibin.raw());
    ZUID did;
    did.fromRaw(dibin.raw());

    ZString name = fetchString(nid);

    ObjectInfo dinfo;
    if(_getObjectInfo(did, &dinfo) != OK)
        throw ZException("data object error");

    if(_backing->seek(dinfo.data.offset) != dinfo.data.offset)
        throw ZException("seek error");

    zu64 fsize = _backing->readbeu64();

//    ZFile file(name, ZFile::WRITE);

    // Dump payload
//    ZBinary buff;
//    zu64 len = 0;
//    _file.seek(info.offset + 8 + 2 + strlen);
//    while(len < size){
//        buff.clear();
//        size = size - _file.read(buff, MIN(1 << 15, size));
//        ZASSERT(file.write(buff), "write failed");
//    }

    if(offset != nullptr)
        *offset = info.data.offset + 8;
    if(size != nullptr)
        *size = fsize;

    return name;
}

// /////////////////////////////////////////////////////////////////////////////

ZParcel::parcelerror ZParcel::removeObject(ZUID id){
    CHECK_COMMON(__FUNCTION__);
    ObjectInfo info;
    parcelerror err = _getObjectInfo(id, &info);
    if(err != OK)
        return err;

    ParcelTreeNode node;
    if(_backing->seek(info.tree) != info.tree)
        return ERR_SEEK;
    if(!node.read(_backing))
        return ERR_READ;

    node.type = NULLOBJ;

    if(_backing->seek(info.tree) != info.tree)
        return ERR_SEEK;
    if(!node.write(_backing))
        return ERR_WRITE;

    if(info.type >= BLOBOBJ){
        err = _freeNodeAdd(node.data.offset, node.data.size + node.extra);
        if(err != OK)
            return err;
    }

    return OK;
}

// /////////////////////////////////////////////////////////////////////////////

void ZParcel::listObjects(){
    _listStep(_treehead, 0);
}

void ZParcel::_listStep(zu64 next, zu16 depth){
    ParcelTreeNode node;

    if(next == ZU64_MAX)
        return;

    node.offset = next;
    if(!node.read(_backing)){
        // Cannot read node
        return;
    }

    LOG(ZString(' ', depth) << node.uid.str() << " " << typeName(node.type));

    _listStep(node.lnode, depth + 1);
    _listStep(node.rnode, depth + 1);
}

// /////////////////////////////////////////////////////////////////////////////

ZString ZParcel::typeName(objtype type){
    if(typetoname.contains(type))
        return typetoname[type];
    else
        return "Unknown Type";
}

ZString ZParcel::errorStr(parcelerror err){
    if(errtostr.contains(err))
        return errtostr[err];
    else
        return "Unknown Error";
}

// /////////////////////////////////////////////////////////////////////////////

zu64 ZParcel::_objectSize(objtype type, zu64 size){
    switch(type){
        case NULLOBJ:
            return 0;
        case BOOLOBJ:
            return 1;
        case UINTOBJ:
        case SINTOBJ:
            return 4;
        case FLOATOBJ:
            return 10;
        case ZUIDOBJ:
            return 16;

        case BLOBOBJ:
        case STRINGOBJ:
            return size;

        case FILEOBJ:
            return 32;

        default:
            return size;
    }
}

ZParcel::parcelerror ZParcel::_storeObject(ZUID id, objtype type, const ZBinary &data, zu64 reserve){
    parcelerror err;

    // Tree node size
    zu64 tsize = ZPARCEL_NODE_LEN;

    // Find free node
    ParcelFreeNode fnode;
    err = _freeNodeFind(tsize, &fnode);
    if(err != OK)
        return err;

    zu64 offset = fnode.offset;

    // Find where to insert into tree
    zu64 next = _treehead;
    if(next == ZU64_MAX){
        // Tree head is uninitialzied
        _treehead = offset;
        // Rewrite header with new tree head
        if(!_writeHeader(0))
            return ERR_WRITE;

    } else {

        // Search tree
        bool found = false;
        ParcelTreeNode node;
        for(zu64 d = 0; d < ZPARCEL_MAX_DEPTH; ++d){
            // Read the node
            node.offset = next;
            if(!node.read(_backing)){
                return ERR_TREE;
            }
            // Compare
            int cmp = node.uid.compare(id);
            if(cmp < 0){
                if(node.rnode == ZU64_MAX){
                    // Hit a leaf
                    node.rnode = offset;
                    found = true;
                    break;
                }
                next = node.rnode;
            } else if(cmp > 0){
                if(node.lnode == ZU64_MAX){
                    // Hit a leaf
                    node.lnode = offset;
                    found = true;
                    break;
                }
                next = node.lnode;
            } else {
                // Object already exists
                if(node.type == NULLOBJ){
                    //
                    break;
                } else {
                    return ERR_EXISTS;
                }
            }
        }
        // Loop ended without finding tree leaf
        if(!found)
            return ERR_MAX_DEPTH;

        // Rewrite the previous node
        node.offset = next;
        if(!node.write(_backing))
            return ERR_WRITE;
    }

    // Allocate node
    zu64 nsize = tsize;
    err = _freeNodeAlloc(&fnode, &nsize);
    if(err != OK)
        return OK;

    ParcelTreeNode newnode;
    newnode.uid = id;
    newnode.lnode = ZU64_MAX;
    newnode.rnode = ZU64_MAX;
    newnode.type = type;
    newnode.extra = nsize - tsize;
    newnode.crc = 0;

    // Write data
    if(newnode.type >= BLOBOBJ){
        // Data node size
        zu64 dsize = _objectSize(type, data.size() + reserve);

        // Allocate data node
        ParcelFreeNode dfnode;
        _freeNodeFind(dsize, &dfnode);
        _freeNodeAlloc(&dfnode, &dsize);

        newnode.data.size = dsize;
        newnode.data.offset = dfnode.offset;

        // Write data node
        if(_backing->seek(newnode.data.offset) != newnode.data.offset)
            return ERR_SEEK;
        if(_backing->write(data.raw(), data.size()) != data.size())
            return ERR_WRITE;

    } else {
        // Copy data into payload
        memcpy(newnode.payload, data.raw(), MIN(data.size(), 16));
    }

    // Write the node
    newnode.offset = offset;
    if(!newnode.write(_backing))
        return ERR_WRITE;

    // Update tail
    if(offset + nsize > _tail)
        _tail = offset + nsize;
    if(!_writeHeader(0))
        return ERR_WRITE;

    return OK;
}

ZParcel::parcelerror ZParcel::_getObjectInfo(ZUID id, ObjectInfo *info){
    // Check cache
    if(_cache.contains(id)){
        *info = _cache[id];
        return OK;
    }

    // Search the tree
    ParcelTreeNode node;
    zu64 next = _treehead;
    zu64 prev = 0;

    for(zu64 d = 0; d < ZPARCEL_MAX_DEPTH; ++d){
        if(next == ZU64_MAX){
            // Hit bottom of tree
            return ERR_NOEXIST;
        }

        node.offset = next;
        if(!node.read(_backing)){
            return ERR_TREE;
        }

        int cmp = node.uid.compare(id);
        if(cmp < 0){
            prev = next;
            next = node.rnode;
        } else if(cmp > 0){
            prev = next;
            next = node.lnode;
        } else {
            // Get the object info
            // TODO: Check object CRC

            info->tree = next;
            info->parent = prev;
            info->lnode = node.lnode;
            info->rnode = node.rnode;

            info->type = node.type;
            if(node.type >= BLOBOBJ){
                info->data.offset = node.data.offset;
                info->data.size = node.data.size;
            } else {
                memcpy(info->payload, node.payload, 16);
            }

            // Add to cache
            _cache.add(id, *info);
            return OK;
        }
    }
    return ERR_MAX_DEPTH;
}

ZParcel::parcelerror ZParcel::_freeNodeFind(zu64 size, ParcelFreeNode *fnode){
    parcelerror err;

    // Search the free list
    fnode->offset = _freehead;   // Position of free node
    fnode->parent = 0;           // Position of parent free node

    while(true){
        if(fnode->offset == ZU64_MAX){
            // No free nodes found
            return ERR_NOFREE;
        }

        // read free list node
        err = fnode->read(_backing);
        if(err != OK)
            return err;

        // check node
        if(fnode->size >= size){
            // Got first free entry large enough
            break;
        }

        // next node
        fnode->parent = fnode->offset;
        fnode->offset = fnode->next;
    }

    return OK;
}

ZParcel::parcelerror ZParcel::_freeNodeAlloc(ParcelFreeNode *fnode, zu64 *size){
    parcelerror err;
    zu64 nextoff;

    if((fnode->size - *size) > 16){
        // Split node
        fnode->size -= *size;
        fnode->offset += *size;

        nextoff = fnode->offset;

        err = fnode->write(_backing);
        if(err != OK)
            return err;

        // If moving the tail, update head
        if(fnode->next == ZU64_MAX){
            _freetail = fnode->offset;
            if(!_writeHeader(0))
                return ERR_WRITE;
        }

    } else {
        // Whole node
        *size = fnode->size;

        nextoff = fnode->next;
    }

    // Update parent freelist
    if(fnode->parent == 0){
        // Rewrite superblock
        _freehead = nextoff;
        if(!_writeHeader(0))
            return ERR_WRITE;
    } else {
        // Rewrite parent
        ParcelFreeNode pnode;
        pnode.offset = fnode->parent;

        err = pnode.read(_backing);
        if(err != OK)
            return err;

        pnode.next = nextoff;

        err = pnode.write(_backing);
        if(err != OK)
            return err;
    }

    return OK;
}

ZParcel::parcelerror ZParcel::_freeNodeAdd(zu64 offset, zu64 size){
    parcelerror err;

    ParcelFreeNode fnode;
    fnode.size = size;
    fnode.next = ZU64_MAX;
    fnode.offset = offset;

    err = fnode.write(_backing);
    if(err != OK)
        return err;

    ParcelFreeNode pnode;
    pnode.offset = _freetail;

    err = pnode.read(_backing);
    if(err != OK)
        return err;

    pnode.next = offset;

    err = pnode.write(_backing);
    if(err != OK)
        return err;

    return OK;
}

bool ZParcel::_writeHeader(zu64 offset){
    ParcelHeader header;
    ::memcpy(header.sig, ZPARCEL_SIG, ZPARCEL_SIG_LEN);
    header.version = _version;
    header.treehead = _treehead;
    header.freehead = _freehead;
    header.freetail = _freetail;
    header.tailptr = _tail;

    if(_backing->seek(offset) != offset)
        return false;
    if(!header.write(_backing))
        return false;
    return true;
}

} // namespace LibChaos
