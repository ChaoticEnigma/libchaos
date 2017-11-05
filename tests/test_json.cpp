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
        LOG(pre << "Array");
        for(int i = 0; i < json.array().size(); ++i){
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

void json(){
    ZString str1 = "{ \"object\" : { \"str\" : \"strval\" , \"num\" : 12345 } , \"array\" : [ \"val1\" , \"val2\" ], \"array2\" : [ 0, 1, 2, 3 ], \"string\" : \"stringval\" , \"number\" : 54321 }";
    LOG(str1);
    ZJSON json1;
    TASSERT(json1.decode(str1));
    checkType(json1, "");

    ZJSON json2;
    json2["string"] = "test1";
    json2["number"] = 12345;
    json2["object"]["str"] = "test2";
    json2["array"] << "test\"3\"";
    json2["array"] << 54321;
    LOG(json2.encode());
}

ZArray<Test> json_tests(){
    return {
        { "json", json, true, {} },
    };
}

}
