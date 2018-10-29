#include "tests.h"
#include "zjson.h"

namespace LibChaosTest {

void checkType(ZJSON &json, ZString pre){
    switch(json.type()){
    case ZJSON::OBJECT: {
        LOG(pre << "Object: " << json.object().size());
        for(auto i = json.object().begin(); i.more(); i.advance()){
            LOG(pre << "  " << i.get() << ":");
            checkType(json.object()[i.get()], pre + "  | ");
        }
        break;
    }
    case ZJSON::ARRAY:
        LOG(pre << "Array: " << json.array().size());
        for(zu64 i = 0; i < json.array().size(); ++i){
            LOG(pre << "  " << i << ":");
            checkType(json.array()[i], pre + "  | ");
        }
        break;
    case ZJSON::STRING:
        LOG(pre << "String: " << json.string());
        break;
    case ZJSON::NUMBER:
        LOG(pre << "Number: " << json.number());
        break;
    case ZJSON::BOOLEAN:
        LOG(pre << "Boolean: " << json.boolean());
        break;
    case ZJSON::NULLVAL:
        LOG(pre << "NULL");
        break;
    case ZJSON::UNDEF:
        LOG(pre << "Undefined");
        break;
    default:
        LOG(pre << "Other");
        break;
    }
}

void json_type(){
    ZJSON json;
    json["obj"] = "test";
    TASSERT(json.type() == ZJSON::OBJECT);
    json << "array";
    TASSERT(json.type() == ZJSON::ARRAY);
    json = "string";
    TASSERT(json.type() == ZJSON::STRING);
    json = 100;
    TASSERT(json.type() == ZJSON::NUMBER);
    json = true;
    TASSERT(json.type() == ZJSON::BOOLEAN);
}

void json_encode(){
    ZJSON json;
    json["string"] = "test1";
    json["number"] = 12345;
    json["object"]["str"] = "test2";
    json["array"] << "test\"3\"";
    json["array"] << 54321;
    ZString estr = json.encode(false);
    LOG(estr);
    TASSERT(estr == "{\"string\":\"test1\",\"number\":12345,\"object\":{\"str\":\"test2\"},\"array\":[\"test\\\"3\\\"\",54321]}");
}

void json_decode(){
    ZString str = "{ \"object\" : { \"str\" : \"strval\", \"num\" : 12345 }, \"array\" : [ \"val1\", \"val2\" ], \"array2\" : [ 0, 1, 2, 3 ], \"string\" : \"stringval\", \"number\" : 54321 }";
    LOG(str);
    ZJSON json;
    TASSERT(json.decode(str));
//    ZString estr = json.encode(true);
//    LOG(estr);
//    TASSERT(estr == str);
    //checkType(json, "");
}

void json_empty(){
    ZString str = "{}";
    ZJSON json;
    TASSERT(json.decode(str));
    TASSERT(json.encode(false) == str);
    checkType(json, "");
}

void json_empty_elem(){
    ZString str = "{ \"object\" : {}, \"array\" : [] }";
    LOG(str);
    ZJSON json;
    TASSERT(json.decode(str));
    ZString estr = json.encode(true);
    LOG(estr);
    TASSERT(estr == str);
    checkType(json, "");
}

ZArray<Test> json_tests(){
    return {
        { "json_type",          json_type,          false, {} },
        { "json_empty",         json_empty,         false, {} },
        { "json_empty_elem",    json_empty_elem,    false, {} },
        { "json_encode",        json_encode,        false, {} },
        { "json_decode",        json_decode,        false, { "json_encode" } },
    };
}

}
