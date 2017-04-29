/*******************************************************************************
**                                  LibChaos                                  **
**                                  zparcel.h                                 **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#ifndef ZPARCEL_H
#define ZPARCEL_H

#include "zuid.h"
#include "zstring.h"
#include "zbinary.h"
#include "zpath.h"
#include "zfile.h"
#include "zmap.h"

namespace LibChaos {

/*! Interface for storing and fetching objects from the LibChaos parcel file format.
 *  Each object is stored, fetched or updated through a unique UUID.
 */
class ZParcel {
public:
    class ParcelObjectAccessor;

    enum parceltype {
        UNKNOWN = 0,
        VERSION1,       //!< Type 1 parcel. No pages, payload in tree node.
        MAX_PARCELTYPE,
    };

    enum parcelstate {
        OPEN,
        CLOSED,
        LOCKED,
    };

    enum {
        NULLOBJ = 0,
        BOOLOBJ,        //!< Boolean object. 1-bit.
        UINTOBJ,        //!< Unsigned integer object. 64-bit.
        SINTOBJ,        //!< Signed integer object. 64-bit.
        FLOATOBJ,       //!< Floating point number object. Double precision.
        ZUIDOBJ,        //!< UUID object.
        BLOBOBJ,        //!< Binary blob object.
        STRINGOBJ,      //!< String object.
        FILEOBJ,        //!< File object. Includes embedded filename and file content.

        /*! User-defined object types can be created by subclassing ZParcel and defining new types starting with MAX_OBJTYPE.
         *  Example:
         *  \code
         *  enum {
         *      CUSTOM1 = MAX_OBJTYPE,
         *      CUSTOM2,
         *  };
         *  \endcode
         */
        MAX_OBJTYPE,
        UNKNOWNOBJ = 255,
    };

    typedef zu8 objtype;

    enum parcelerror {
        OK = 0,         //!< No error.
        ERR_OPEN,       //!< Error opening file.
        ERR_SEEK,       //!< Error seeking file.
        ERR_READ,       //!< Error reading file.
        ERR_WRITE,      //!< Error writing file.
        ERR_EXISTS,     //!< Object exists.
        ERR_NOEXIST,    //!< Object does not exist.
        ERR_BADCRC,     //!< CRC mismatch.
        ERR_TRUNC,      //!< Payload is truncated by end of file.
        ERR_TREE,       //!< Bad tree structure.
        ERR_FREELIST,   //!< Bad freelist structure.
        ERR_NOFREE,     //!< No free nodes.
        ERR_SIG,        //!< Bad file signature.
        ERR_VERSION,    //!< Bad file header version.
        ERR_MAX_DEPTH,  //!< Exceeded maximum tree depth.
        ERR_MAGIC,      //!< Bad object magic number.
    };

private:
    struct ObjectInfo {
        zu64 tree;      // Tree node offset
        zu64 parent;    // Parent tree node offset
        zu64 lnode;     // Left child tree node offset
        zu64 rnode;     // Right child tree node offset

        objtype type;   // Payload type
        union {
            zbyte payload[16];
            struct {
                zu64 offset;    // Payload offset
                zu64 size;      // Payload size
            } data;
        };

        ZPointer<ParcelObjectAccessor> accessor;
    };

public:
    ZParcel();
    ~ZParcel();

    /*! Create new parcel file and open it.
     *  This will overwrite an existing file.
     *  \exception ZException Failed to create file.
     */
    parcelerror create(ZPath file);
    parcelerror create(ZBlockAccessor *file);

    /*! Open existing parcel.
     *  \exception ZException Failed to open file.
     */
    parcelerror open(ZPath file);
    parcelerror open(ZBlockAccessor *file);

    //! Close file handles.
    void close();

    //! Check if \a id exists in the parcel.
    bool exists(ZUID id);
    //! Get type of parcel object.
    objtype getType(ZUID id);

    /*! Store null in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeNull(ZUID id);
    /*! Store bool in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeBool(ZUID id, bool bl);
    /*! Store unsigned int in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeUint(ZUID id, zu64 num);
    /*! Store signed int in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeSint(ZUID id, zs64 num);
    /*! Store float in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeFloat(ZUID id, double num);
    /*! Store ZUID in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeZUID(ZUID id, ZUID uid);
    /*! Store string in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeString(ZUID id, ZString str);
    /*! Store blob in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeBlob(ZUID id, ZBinary blob);
    /*! Store file reference in parcel.
     *  \exception ZException Parcel not open.
     */
    parcelerror storeFile(ZUID id, ZPath path);

    /*! Fetch bool from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    bool fetchBool(ZUID id);
    /*! Fetch unsigned int from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    zu64 fetchUint(ZUID id);
    /*! Fetch signed int from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    zs64 fetchSint(ZUID id);
    /*! Fetch float from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    double fetchFloat(ZUID id);
    /*! Fetch ZUID from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    ZUID fetchZUID(ZUID id);
    /*! Fetch string from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    ZString fetchString(ZUID id);
    /*! Fetch blob from parcel.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    ZBinary fetchBlob(ZUID id);
    /*! Fetch file object from parcel.
     *  If \a offset is not null, the offset of the file payload is written at \a offset.
     *  If \a size is not null, the size of the file payload is written at \a size.
     *  \return The name of the file.
     *  \exception ZException Parcel not open.
     *  \exception ZException Object does not exist.
     *  \exception ZException Object has wrong type.
     */
    ZString fetchFile(ZUID id, zu64 *offset = nullptr, zu64 *size = nullptr);

    /*! Remove an object from the parcel.
     *  \exception ZException Parcel not open.
      */
    parcelerror removeObject(ZUID id);

    void listObjects();

    void _listStep(zu64 next, zu16 depth);

    //! Get the ZFile handle for the parcel.
    ZFile getHandle() { return _file; }

    //! Get string name of object type.
    static ZString typeName(objtype type);
    //! Get string for error.
    static ZString errorStr(parcelerror err);

public:
    class ParcelObjectAccessor : public ZBlockAccessor {
    public:
        ParcelObjectAccessor(ZBlockAccessor *file, zu64 offset, zu64 size) :
            _file(file), _base(offset), _pos(0), _size(size){

        }

        // ZReader interface
        zu64 available() const {
            return (_base + _size) - _pos;
        }
        zu64 read(zbyte *dest, zu64 size);

        // ZWriter interface
        zu64 write(const zbyte *src, zu64 size);

        // ZPosition interface
        zu64 tell() const {
            return _pos;
        }
        zu64 seek(zu64 pos){
            _pos = MIN(pos, _base + _size);
            return _pos;
        }
        bool atEnd() const {
            return (tell() == _base + _size);
        }

    private:
        ZBlockAccessor *const _file;
        const zu64 _base;
        zu64 _pos;
        const zu64 _size;
    };

private:
    class ParcelHeader {
    public:
        ParcelHeader(ZParcel *parcel) : file(parcel->_backing){}

        parcelerror read();
        parcelerror write();

        static zu64 getNodeSize(){ return 7 + 1 + 8 + 8 + 8 + 8; }

    public:
        //    zbyte sig[ZPARCEL_SIG_LEN];
        zu8 version;
        zu64 treehead;
        zu64 freehead;
        zu64 freetail;
        zu64 tailptr;

    private:
        ZBlockAccessor *file;
    };

private:
    class ParcelTreeNode {
    public:
        ParcelTreeNode(ZParcel *parcel, zu64 addr) : file(parcel->_backing), offset(addr){}

        parcelerror read();
        parcelerror write();

        static zu64 getNodeSize(){ return 4 + ZUID_SIZE + 8 + 8 + 2 + 2 + 16; }

    public:
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


    private:
        ZBlockAccessor *file;
        zu64 offset;
    };

private:
    class ParcelFreeNode {
    public:
        ParcelFreeNode(ZParcel *parcel, zu64 addr) : file(parcel->_backing), offset(addr){}

        parcelerror read();
        parcelerror write();

        static zu64 getNodeSize(){ return 4 + 8 + 8; }

    public:
        zu64 next;
        zu64 size;

    private:
        ZBlockAccessor *file;
        zu64 offset;
    };

protected:
    //! Compute the size of an object node payload.
    zu64 _objectSize(objtype type, zu64 size);
    /*! Store a new object with \a id and \a type.
     *  The contents of \a data are written into the payload of the new object.
     *  If \a trailsize > 0, indicates the number of bytes that should be reserved in the payload,
     *  beyond the size of \a data.
     */
    parcelerror _storeObject(ZUID id, objtype type, const ZBinary &data, zu64 reserve = 0);
    //! Get object info struct.
    parcelerror _getObjectInfo(ZUID id, ObjectInfo *info);

    parcelerror _nodeAlloc(zu64 size, zu64 *offset, zu64 *nsize);
    parcelerror _nodeFree(zu64 offset, zu64 size);

private:
    //! Write a copy of the file header at \a offset.
    bool _writeHeader(zu64 offset);

private:
    parcelstate _state;

    ZFile _file;
    ZBlockAccessor *_backing;

    parceltype _version;
    zu64 _treehead;
    zu64 _freehead;
    zu64 _freetail;
    zu64 _tail;
    ZMap<ZUID, ObjectInfo> _cache;
};

} // namespace LibChaos

#endif // ZPARCEL_H
