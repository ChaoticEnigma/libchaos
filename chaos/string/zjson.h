/*******************************************************************************
**                                  LibChaos                                  **
**                                  zjson.h                                   **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#ifndef ZJSON_H
#define ZJSON_H

#include "zstring.h"
#include "zmap.h"

namespace LibChaos {

/*! JSON (JavaScript Object Notation) container, decoder and encoder.
 *  \ingroup String
 */
class ZJSON {
public:
    enum jsontype {
        UNDEF = 0,
        OBJECT,
        ARRAY,
        STRING,
        NUMBER,
        BOOLEAN,
        NULLVAL,
    };

    struct JsonError {
        zu64 pos;
        ZString desc;
    };

public:
    //! Type constructor.
    ZJSON(jsontype type = UNDEF);

    //! String (ZString) constructor.
    ZJSON(ZString str) : ZJSON(STRING){ string() = str; }
    //! String (c-str) constructor.
    ZJSON(const char *str) : ZJSON(ZString(str)){}
    //! Number (double) constructor.
    ZJSON(double num) : ZJSON(NUMBER){ number() = num; }
    //! Number (int) constructor.
    ZJSON(int num) : ZJSON((double)num){}
    //! Boolean constructor.
    ZJSON(bool bl) : ZJSON(BOOLEAN){ boolean() = bl; }

    //! Copy constructor.
    ZJSON(const ZJSON &other);

    //! Destructor.
    ~ZJSON();

    //! Assignment operator.
    ZJSON &operator=(const ZJSON &other);

    //! Encode JSON string.
    ZString encode(bool readable = false);

    //! Decode JSON string.
    bool decode(const ZString &str);

    bool isValid();

    //! Shortcut for object subscript operator.
    ZJSON &operator[](ZString key){
        initType(OBJECT);
        return object()[key];
    }

    //! Shortcut for array subscript operator.
    ZJSON &operator[](zu64 i){
        initType(ARRAY);
        return array()[i];
    }

    //! Shortcut for array add operator.
    ZJSON &operator<<(ZJSON value){
        initType(ARRAY);
        array().push(value);
        return *this;
    }

    //! Get type of this JSON object.
    jsontype type() const { return _type; }

    // Data accessors
    ZMap<ZString, ZJSON> &object();
    ZArray<ZJSON> &array();
    ZString &string();
    double &number();
    bool &boolean();

private:
    void initType(jsontype type);
    ZString jsonEscape(ZString str);
    bool jsonDecode(const ZString &str, zsize *position, JsonError *err);

private:
    //! JSON type.
    jsontype _type;
    ZAllocator<ZMap<ZString,ZJSON>> _oalloc;
    ZAllocator<ZArray<ZJSON>> _aalloc;
    ZAllocator<ZString> _salloc;

    //! Decoded JSON data.
#if 1
//#if LIBCHAOS_COMPILER == LIBCHAOS_COMPILER_MSVC
    struct JSONValue {
#else
    union JSONValue {
#endif
        ZMap<ZString, ZJSON> *object;
        ZArray<ZJSON> *array;
        ZString *string;
        double number;
        bool boolean;
    } _data;
};

}

#endif // ZJSON_H
