/*******************************************************************************
**                                  LibChaos                                  **
**                                  zjson.cpp                                 **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zjson.h"
#include "zlog.h"

#include <string>
#include <assert.h>

//#define ZJSON_DEBUG

namespace LibChaos {

ZJSON::ZJSON(jsontype type) : _type(UNDEF){
    initType(type);
}

ZJSON::ZJSON(const ZJSON &other) : _type(UNDEF){
    operator=(other);
}

ZJSON::~ZJSON(){
    // BUG: ZJSON memory leak
    //initType(UNDEF);
}

ZJSON &ZJSON::operator=(const ZJSON &other){
    initType(other._type);
    switch(other._type){
        case OBJECT:
            _data.object = other._data.object;
            break;
        case ARRAY:
            _data.array = other._data.array;
            break;
        case STRING:
            _data.string = other._data.string;
            break;
        case NUMBER:
            _data.number = other._data.number;
            break;
        case BOOLEAN:
            _data.boolean = other._data.boolean;
            break;
        default:
            break;
    }
    return *this;
}

ZString ZJSON::encode(bool readable){
    ZString tmp;
    switch(_type){
        case OBJECT:
            if(_data.object.size()){
                tmp = "{";
                if(readable) tmp += " ";
                bool first = true;
                for(auto i = _data.object.begin(); i.more(); i.advance()){
                    if(!first){
                        tmp += (readable ? ", " : ",");
                    }
                    first = false;
                    tmp += (ZString("\"") + i.get() + "\"");
                    if(readable) tmp += " ";
                    tmp += ":";
                    if(readable) tmp += " ";
                    tmp += _data.object[i.get()].encode(readable);
                }
                if(readable) tmp += " ";
                tmp += "}";
            } else {
                return "{}";
            }
            break;
        case ARRAY:
            if(_data.array.size()){
                tmp = "[";
                if(readable) tmp += " ";
                bool first = true;
                for(zu64 i = 0; i < _data.array.size(); ++i){
                    if(!first){
                        tmp += (readable ? ", " : ",");
                    }
                    first = false;
                    tmp += _data.array[i].encode(readable);
                }
                if(readable) tmp += " ";
                tmp += "]";
            } else {
                return "[]";
            }
            break;
        case STRING:
            return ZString("\"") + jsonEscape(_data.string) + "\"";
            break;
        case NUMBER:
            return _data.number;
            break;
        case BOOLEAN:
            return (_data.boolean ? "true" : "false");
            break;
        case NULLVAL:
            return "null";
            break;
        default:
            break;
    }
    return tmp;
}

bool isWhitespace(char ch){
    return (ch == ' ' || ch == '\n' || ch == '\t');
}

bool isDigit(char ch){
    return (ch >= '0' && ch <= '9');
}

bool ZJSON::isValid(){
    return (_type != UNDEF);
}

bool ZJSON::decode(const ZString &str){
    zu64 position = 0;
    JsonError err;
    if(jsonDecode(str, &position, &err))
        return true;
    ZString pstr;
    if(err.pos < 10){
        pstr = ZString::substr(str, err.pos - err.pos, err.pos);
    } else {
        pstr = "..." + ZString::substr(str, err.pos - 10, 10);
    }
    ZString sstr;
    if(str.size() < 11 || err.pos > str.size()-11){
        sstr = ZString::substr(str, err.pos+1, 10);
    } else {
        sstr = ZString::substr(str, err.pos+1, 10) + "...";
    }
    ELOG("ZJSON error @ " << err.pos << " {" << pstr << ">" << str[err.pos] << "<" << sstr << "} => " << err.desc);
    return false;
}

ZMap<ZString, ZJSON> &ZJSON::object(){
    if(_type != OBJECT)
        throw ZException("ZJSON object is not Object");
    return _data.object;
}

ZArray<ZJSON> &ZJSON::array(){
    if(_type != ARRAY)
        throw ZException("ZJSON object is not Array");
    return _data.array;
}

ZString &ZJSON::string(){
    if(_type != STRING)
        throw ZException("ZJSON object is not String");
    return _data.string;
}

double &ZJSON::number(){
    if(_type != NUMBER)
        throw ZException("ZJSON object is not Number");
    return _data.number;
}

bool &ZJSON::boolean(){
    if(_type != BOOLEAN)
        throw ZException("ZJSON object is not Boolean");
    return _data.boolean;
}

void ZJSON::initType(ZJSON::jsontype type){
    // Don't destroy objects if types are the same.
    if(type == _type)
        return;

    // Deconstruct existing value
    switch(_type){
        case OBJECT:
            _data.object.~ZMap();
            break;
        case ARRAY:
            _data.array.~ZArray();
            break;
        case STRING:
            _data.string.~ZString();
            break;
        default:
            break;
    }

    _type = type;

    // Construct new value
    switch(_type){
        case OBJECT:
            new (&_data.object) ZMap<ZString, ZJSON>;
            break;
        case ARRAY:
            new (&_data.array) ZArray<ZJSON>;
            break;
        case STRING:
            new (&_data.string) ZString;
            break;
        case NUMBER:
            _data.number = 0.0f;
            break;
        case BOOLEAN:
            _data.boolean = false;
            break;
        default:
            break;
    }
}

ZString ZJSON::jsonEscape(ZString str){
    str.replace("\\", "\\\\");
    str.replace("\"", "\\\"");
    str.replace("\b", "\\b");
    str.replace("\f", "\\f");
    str.replace("\n", "\\n");
    str.replace("\r", "\\r");
    str.replace("\t", "\\t");
    return str;
}

bool ZJSON::jsonDecode(const ZString &str, zu64 *position, JsonError *err){
//    if(!validJSON(s))
//        return false;

    // Check if JSON is special value
    ZString tstr = ZString::substr(str, *position);
    tstr.strip(' ');
    if(tstr == "true"){
        initType(BOOLEAN);
        _data.boolean = true;
        return true;
    } else if(tstr == "false"){
        initType(BOOLEAN);
        _data.boolean = false;
        return true;
    } else if(tstr == "null"){
        initType(NULLVAL);
        return true;
    }
    // Otherwise, parse

    // Location indicates what is expected at current character
    enum location {
        start = 0,  // Beginning of JSON string
        bkey,       // Beginning of key
        key,        // Inside key
        akey,       // At end of key
        bval,       // Beginning of value
        aval,       // At end of value
        strv,       // Inside String Value
        num,        // Inside Number Value
    } loc = start;

    ZString kbuff;
    ZString vbuff;

#ifdef ZJSON_DEBUG
    ZMap<location, ZString> descs = {
        { start,    "start" },
        { bkey,     "before key" },
        { key,      "in key" },
        { akey,     "after key" },
        { bval,     "before val" },
        { aval,     "after val" },
        { strv,     "in str" },
        { num,      "in num" },
    };
    LOG("start " << *position);
#endif

    // Start decoding
    bool status = false;
    for(zu64 i = *position; i < str.size(); ++i){
        char c = str[i];
#ifdef ZJSON_DEBUG
        LOG("c " << i << ": '" << c << "' " << descs[loc]);
#endif
        // skip whitespace except inside strings
        if(loc != strv && isWhitespace(c))
            continue;

        switch(loc){
            // Before JSON
            case start:
                if(c == '{'){
                    initType(OBJECT);
                    loc = bkey;
                } else if(c == '['){
                    initType(ARRAY);
                    loc = bval;
                } else if(c == '"'){
                    initType(STRING);
                    loc = strv;
                } else if(isDigit(c)){
                    initType(NUMBER);
                    loc = num;
                    vbuff += c;
                } else {
                    err->pos = i;
                    err->desc = ZString("unexpected character '") + c + "'";
                    return false;
                }
                break;

            // Before Key
            case bkey:
                if(c == '}'){
                    *position = i;
                    status = true;
                    break;
                } else if(c == '"'){
                    loc = key;
                } else {
                    err->pos = i;
                    err->desc = "expected \" or whitespace";
                    return false;
                }
                break;

            // In Key
            case key:
                if(c == '"' && str[i-1] != '\\'){
                    loc = akey;
                } else {
                    kbuff += c;
                }
                break;

            // After Key
            case akey:
                if(c == ':'){
                    loc = bval;
                } else {
                    err->pos = i;
                    err->desc = "expected : or whitespace";
                    return false;
                }
                break;

            // Before Value
            case bval: {
                if((_type == ARRAY && c == ']') || (_type == OBJECT && c == '}')){
                    *position = i;
                    status = true;
                    break;
                } else {
                    ZJSON json;
                    if(!json.jsonDecode(str, &i, err)){
                        return false;
                    }
                    //--i;
                    if(_type == OBJECT){
                        //DLOG("add to object: " << json.encode());
                        _data.object.add(kbuff, json);
                        kbuff.clear();
                    } else if(_type == ARRAY){
                        //DLOG("add to array: " << json.encode());
                        _data.array.push(json);
                    } else {
                        assert(false);
                        err->pos = i;
                        err->desc = "fatal internal error";
                        return false;
                    }
                    loc = aval;
                }
                break;
            }

            // String
            case strv:
                if(c == '"' && str[i-1] != '\\'){
                    _data.string = vbuff;
                    ++i;
                    *position = i-1;
                    status = true;
                    break;
                } else {
                    vbuff += c;
                }
                break;

            // Number
            case num:
                if(c == ',' || c == '}' || c == ']'){
                    _data.number = std::stod(vbuff.str());
                    *position = i-1;
                    status = true;
                    break;
                } else {
                    vbuff += c;
                }
                break;

            // After Value
            case aval:
                if(_type == ARRAY){
                    if(c == ','){
                        loc = bval;
                    } else if(c == ']'){
                        *position = i;
                        status = true;
                        break;
                    } else {
                        err->pos = i;
                        err->desc = "expected array continuation, end, or whitespace";
                        return false;
                    }
                } else {
                    if(c == ','){
                        loc = bkey;
                    } else if(c == '}'){
                        *position = i;
                        status = true;
                        break;
                    } else {
                        err->pos = i;
                        err->desc = "expected object continuation, end, or whitespace";
                        return false;
                    }
                }
                break;

            default:
                // Not Good.
                assert(false);
                err->pos = i;
                err->desc = "fatal internal error";
                return false;
                break;
        }

        if(status)
            break;
    }

#ifdef ZJSON_DEBUG
    if(status){
        LOG("end " << *position);
    }
#endif
    return status;
}

}
