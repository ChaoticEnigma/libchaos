#include "tests.h"
#include "zhash.h"
#include "zmap.h"
#include "zset.h"

namespace LibChaosTest {

void hash(){
    int data1 = -34563;
    zu64 hash1 = ZHash<int>(data1).hash();
    LOG(data1 << " " << hash1);

    auto hf = [](ZString data){
        {
            zu64 hasha = ZHash<ZString, ZHashBase::SIMPLE64>(data).hash();
            zu64 hashb = ZHash<ZString, ZHashBase::SIMPLE64>(data).hash();
            LOG("Simple: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb);
        }

        {
            zu64 hasha = ZHash<ZString, ZHashBase::XXHASH64>(data).hash();
            zu64 hashb = ZHash<ZString, ZHashBase::XXHASH64>(data).hash();
            LOG("XXHash: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb);
        }

        {
            zu64 hasha = ZHash<ZString, ZHashBase::FNV64>(data).hash();
            zu64 hashb = ZHash<ZString, ZHashBase::FNV64>(data).hash();
            LOG("FNVHash: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb)
        }

        {
            zu32 hasha = ZHash<ZString, ZHashBase::CRC16>(data).hash();
            zu32 hashb = ZHash<ZString, ZHashBase::CRC16>(data).hash();
            LOG("CRC-16: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb)
        }

        {
            zu32 hasha = ZHash<ZString, ZHashBase::CRC32>(data).hash();
            zu32 hashb = ZHash<ZString, ZHashBase::CRC32>(data).hash();
            LOG("CRC-32: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb)
        }

#ifdef ZHASH_HAS_MD5
        {
            ZBinary hasha = ZHash<ZString, ZHashBase::MD5>(data).hash();
            ZBinary hashb = ZHash<ZString, ZHashBase::MD5>(data).hash();
            LOG("MD5: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb)
        }
#endif

#ifdef ZHASH_HAS_SHA1
        {
            ZBinary hasha = ZHash<ZString, ZHashBase::SHA1>(data).hash();
            ZBinary hashb = ZHash<ZString, ZHashBase::SHA1>(data).hash();
            LOG("SHA-1: " << data << " " << hasha << " " << hashb);
            TASSERT(hasha == hashb)
        }
#endif
    };

    ZString data2 = "LibChaos";
    ZString data3 = "Archon";
    ZString data4 = "Zephyr";

    hf(data2);
    hf(data3);
    hf(data4);
}

const ZBinary data1 = { 0x43, 0x66, 0x88, 0x09, 0x9c, 0x84, 0x93, 0xf0, 0x13, 0xc3, 0xd3, 0x56, 0x84, 0x9e, 0xfb, 0xf7 };
const ZBinary data2 = { 0x70, 0xf4, 0x26, 0xe8, 0xe3, 0xd6, 0xcb, 0x2f, 0xb7, 0x8c, 0xfd, 0x9a, 0x6b, 0x04, 0xf7, 0x6a };

void hash_crc16(){
    zu16 hash1 = 0xbcdc;
    {
        zu16 hasha = ZHash<ZBinary, ZHashBase::CRC16>(data1).hash();
        TASSERT(hasha == hash1); // correct
    }
    zu16 hash2 = 0x413a;
    {
        zu16 hasha = ZHash<ZBinary, ZHashBase::CRC16>(data2).hash();
        TASSERT(hasha == hash2); // correct
    }
}

void hash_crc32(){
    zu32 hash1 = 0xc3ee16eb;
    {
        zu32 hasha = ZHash<ZBinary, ZHashBase::CRC32>(data1).hash();
        TASSERT(hasha == hash1); // correct
        zu32 hashb = ZHash<ZBinary, ZHashBase::CRC32>(data1).hash();
        TASSERT(hashb == hash1); // repeated
    }
    zu32 hash2 = 0x893bae1a;
    {
        zu32 hasha = ZHash<ZBinary, ZHashBase::CRC32>(data2).hash();
        TASSERT(hasha == hash2); // correct
        zu32 hashb = ZHash<ZBinary, ZHashBase::CRC32>(data2).hash();
        TASSERT(hashb == hash2); // repeated
    }
}

#ifdef ZHASH_HAS_MD5
void hash_md5(){
    ZBinary hash1 = { 0xd8, 0x27, 0x81, 0xda, 0x14, 0x42, 0x7b, 0xd2, 0x4b, 0xed, 0x2c, 0x3b, 0x8b, 0x8c, 0x9a, 0x93 };
    {
        ZBinary hasha = ZHash<ZBinary, ZHashBase::MD5>(data1).hash();
        TASSERT(hasha == hash1); // correct
        ZBinary hashb = ZHash<ZBinary, ZHashBase::MD5>(data1).hash();
        TASSERT(hashb == hash1); // repeated
    }
    ZBinary hash2 = { 0x65, 0xf8, 0xb6, 0xe7, 0x69, 0xdb, 0xa0, 0x5f, 0xd8, 0xe3, 0x44, 0x33, 0xdb, 0x15, 0x67, 0x16 };
    {
        ZBinary hasha = ZHash<ZBinary, ZHashBase::MD5>(data2).hash();
        TASSERT(hasha == hash2); // correct
        ZBinary hashb = ZHash<ZBinary, ZHashBase::MD5>(data2).hash();
        TASSERT(hashb == hash2); // repeated
    }
}
#endif

#ifdef ZHASH_HAS_SHA1
void hash_sha1(){
    ZBinary hash1 = { 0xcc, 0x70, 0x4a, 0x8b, 0x35, 0x3f, 0x3e, 0xe3, 0x94, 0xbb, 0x73, 0xdb, 0x70, 0x9d, 0x48, 0xbe, 0x2c, 0x4b, 0x09, 0xdd };
    {
        ZBinary hasha = ZHash<ZBinary, ZHashBase::SHA1>(data1).hash();
        TASSERT(hasha == hash1); // correct
        ZBinary hashb = ZHash<ZBinary, ZHashBase::SHA1>(data1).hash();
        TASSERT(hashb == hash1); // repeated
    }
    ZBinary hash2 = { 0xe6, 0xb4, 0x9d, 0x85, 0x8b, 0x92, 0xe5, 0xa5, 0x11, 0x30, 0x3c, 0x44, 0xfb, 0xb4, 0xd6, 0x72, 0xe0, 0x83, 0xdb, 0xe6 };
    {
        ZBinary hasha = ZHash<ZBinary, ZHashBase::SHA1>(data2).hash();
        TASSERT(hasha == hash2); // correct
        ZBinary hashb = ZHash<ZBinary, ZHashBase::SHA1>(data2).hash();
        TASSERT(hashb == hash2); // repeated
    }
}
#endif

void map(){
    ZMap<ZString, zu64> map1;
    map1.add("test1", 11);
    map1.add("test2", 22);
    map1.add("test3", 33);
    map1.add("test4", 44);
    map1.add("test5", 55);
    map1.add("test6", 66);
    map1.add("test7", 77);
    map1.add("test8", 88);
    map1.add("test9", 99);
    map1.add("test:", 111);
    map1.add("test;", 222);
    map1.add("test<", 333);
    map1.add("test=", 444);
    map1.add("test>", 555);
    map1.add("test?", 666);

    LOG(map1.get("test4"));
    LOG(map1.get("test3"));
    LOG(map1.get("test2"));
    LOG(map1.get("test1"));

    map1.add("test3", 3333);

    LOG(map1.get("test3"));

    LOG(map1.size() << " " << map1.realSize());
    for(zu64 i = 0; i < map1.realSize(); ++i){
        if(map1.position(i).flags & ZMAP_ENTRY_VALID)
            LOG(i << " " << map1.position(i).key << " " << map1.position(i).value << " " << map1.position(i).hash);
    }

    map1["test21"] = 1001;
    map1["test22"] = 2002;

    LOG(map1.get("test21"));
    LOG(map1.get("test22"));

    map1.remove("test8");
    map1.remove("test:");

    LOG(map1.size() << " " << map1.realSize());
    for(zu64 i = 0; i < map1.realSize(); ++i){
        if(map1.position(i).flags & ZMAP_ENTRY_VALID)
            LOG(i << " " << map1.position(i).key << " " << map1.position(i).value << " " << map1.position(i).hash);
    }

    map1["test8"] = 888;

    LOG(map1.size() << " " << map1.realSize());
    for(zu64 i = 0; i < map1.realSize(); ++i){
        if(map1.position(i).flags & ZMAP_ENTRY_VALID)
            LOG(i << " " << map1.position(i).key << " " << map1.position(i).value << " " << map1.position(i).hash);
    }

    map1["test8"] = 999;

    LOG(map1.size() << " " << map1.realSize());
    for(zu64 i = 0; i < map1.realSize(); ++i){
        if(map1.position(i).flags & ZMAP_ENTRY_VALID)
            LOG(i << " " << map1.position(i).key << " " << map1.position(i).value << " " << map1.position(i).hash);
    }

    LOG("Forward Iterator: " << map1.size());
    auto i1f = map1.begin();
    test_forward_iterator(&i1f, map1.size());

    ZMap<ZString, zu64> map2 = map1;
    LOG("Forward Iterator: " << map2.size());
    auto i2f = map2.begin();
    test_forward_iterator(&i2f, map2.size());

    LOG("Keys:");
    ZArray<ZString> keys = map1.keys();
    for(auto it = keys.begin(); it.more(); ++it){
        LOG(*it);
    }
    TASSERT(keys.size() == map1.size());
}

void set(){
    ZString str1 = "one";
    ZString str2 = "two";
    ZString str3 = "three";
    ZString str4 = "four";
    ZString str5 = "five";

    ZSet<ZString> set1;
    set1.add(str1);
    set1.add(str2);
    set1.add(str3);
    set1.add(str4);
    set1.add(str5);
    set1.remove(str3);
    TASSERT(set1.contains(str1));
    TASSERT(set1.contains(str2));
    TASSERT(!set1.contains(str3));
    TASSERT(set1.contains(str4));
    TASSERT(set1.contains(str5));

    LOG("Forward Iterator: " << set1.size());
    auto i1f = set1.begin();
    test_forward_iterator(&i1f, set1.size());

    ZSet<ZString> set2 = set1;
    LOG("Forward Iterator: " << set2.size());
    auto i2f = set1.begin();
    test_forward_iterator(&i2f, set2.size());
}

ZArray<Test> hash_tests(){
    return {
        { "hash",       hash,       true, {} },
        { "hash-crc16", hash_crc16, true, { "hash" } },
        { "hash-crc32", hash_crc32, true, { "hash" } },
#ifdef ZHASH_HAS_MD5
        { "hash-md5",   hash_md5,   true, { "hash" } },
#endif
#ifdef ZHASH_HAS_SHA1
        { "hash-sha1",  hash_sha1,  true, { "hash" } },
#endif
        { "map",        map,        true, { "hash" } },
        { "set",        set,        true, { "hash" } },
    };
}

}
