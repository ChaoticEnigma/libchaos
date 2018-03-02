#include "tests.h"
#include "zrandom.h"
#include "zuid.h"
#include "zlist.h"
#include "zhash.h"
#include "zoptions.h"

#define PADLEN 16
#define PAD(X) ZString(X).pad(' ', PADLEN)

namespace LibChaosTest {

void test_random(){
    ZRandom random;
    LOG(random.genzu());
}

void test_mac(){
    auto fmt = [](ZBinary mac){
        ZString macstr;
        for(zu64 i = 0 ; i < mac.size(); ++i)
            macstr += ZString::ItoS(mac[i], 16, 2) += ":";
        macstr.substr(0, macstr.size()-1);
        return macstr;
    };

    ZBinary mac = ZUID::getMACAddress();
    LOG("MAC: " << fmt(mac));

    LOG("MAC List:");
    ZList<ZBinary> maclist = ZUID::getMACAddresses();
    for(auto it = maclist.begin(); it.more(); ++it)
        LOG("  " << fmt(it.get()));
}

void uid_str(){
    ZUID uid1;

    ZString uidstr2 = "abcdef00-1234-5678-9012-fedcbaabcdef";
    ZUID uid2 = uidstr2;
    TASSERT(uid2.str() == uidstr2);

    ZString uidstr3 = "abcdef0x-1234-5678-9012-fedcbaabcdef";
    ZUID uid3 = uidstr3;
    LOG(uid3.str());
    TASSERT(uid3 == ZUID_NIL);

    ZUID uid4(ZUID::NIL);

    LOG(PAD("Default (nil):")  << uid1.str());
    LOG(PAD("String:")         << uid2.str());
    LOG(PAD("String (fail):")  << uid3.str() << " " << uidstr3);
    LOG(PAD("Nil:")            << uid4.str());
}

void uid_time(){
    ZUID uid5(ZUID::TIME);
    ZUID uid6(ZUID::RANDOM);

    LOG(PAD("Time:")           << uid5.str());
    LOG(PAD("Random:")         << uid6.str());

    ZBinary mac = ZUID::getMACAddress();
    ZString macstr;
    for(zu64 i = 0 ; i < mac.size(); ++i)
        macstr += ZString::ItoS(mac[i], 16, 2) += ":";
    macstr.substr(0, macstr.size()-1);
    LOG(ZString("MAC:").pad(' ', PADLEN) << macstr << " " << mac.size() << " " << mac);

    //ZList<ZBinary> maclist = ZUID::getMACAddresses();
}

#if defined(ZHASH_HAS_MD5) && defined(ZHASH_HAS_SHA1)
void uid_name(){
    ZUID ndns("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    ZString name = "www.znnxs.com";
    ZUID uid7(ZUID::NAME_MD5, ndns, name);
    ZUID uid8(ZUID::NAME_SHA, ndns, name);

    TASSERT(ZUID(ZUID::NAME_MD5, ndns, "www.widgets.com").str() == "e902893a-9d22-3c7e-a7b8-d6e313b71d9f");

    LOG(PAD("Name MD5:")       << uid7.str());
    LOG(PAD("Name SHA1:")      << uid8.str());
}
#endif

void options(){
    const ZArray<ZOptions::OptDef> optdef = {
        { "option_a",   'a', ZOptions::NONE },
        { "option_b",   'b', ZOptions::STRING },
        { "option_c",   'c', ZOptions::INTEGER },
        { "option_d",   'd', ZOptions::LIST },
    };

    const char *const argv[] = {
        "./dummy",
        "arg0",
        "-a",
        "arg1",
        "-b",
        "test1",
        "arg2",
        "-b",
        "test space",
        "arg3",
        "--option_c",
        "100",
        "arg4",
        "--option_d=tst1",
        "--option_d=tst 2",
        "--option_d=\"tst3\"",
        "--option_d=\"tst 4\"",
        "-dtst5",
        "-d\"tst 6\"",
        "arg5",
        "arg6",
    };
    int argc = sizeof(argv)/sizeof(char*);
    LOG("Tokens: " << argc);

    ZOptions options(optdef);
    TASSERT(options.parse(argc, argv));

    LOG("Options:");
    auto opts = options.getOpts();
    for(auto it = opts.begin(); it.more(); ++it){
        LOG("  " << it.get() << ": '" << opts[it.get()] << "'");
    }

    TASSERT(opts.size() == 4);
    TASSERT(opts.contains("option_a"));
    TASSERT(opts.contains("option_b"));
    TASSERT(opts.contains("option_c"));
    TASSERT(opts.contains("option_d"));
    TASSERT(opts["option_a"] == "");
    TASSERT(opts["option_b"] == "test space");
    TASSERT(opts["option_c"] == "100");
    ZArray<ZString> dopt = opts["option_d"].explode(',');
    TASSERT(dopt.size() == 6);
    TASSERT(dopt[0] == "tst1");
    TASSERT(dopt[1] == "tst 2");
    TASSERT(dopt[2] == "\"tst3\"");
    TASSERT(dopt[3] == "\"tst 4\"");
    TASSERT(dopt[4] == "tst5");
    TASSERT(dopt[5] == "\"tst 6\"");

    LOG("Arguments:");
    auto args = options.getArgs();
    for(auto it = args.begin(); it.more(); ++it){
        LOG("  '" << it.get() << "'");
    }

    TASSERT(args.size() == 7);
    TASSERT(args[0] == "arg0");
    TASSERT(args[1] == "arg1");
    TASSERT(args[2] == "arg2");
    TASSERT(args[3] == "arg3");
    TASSERT(args[4] == "arg4");
    TASSERT(args[5] == "arg5");
    TASSERT(args[6] == "arg6");
}

ZArray<Test> misc_tests(){
    return {
        { "random",     test_random,    true, {} },
        { "mac",        test_mac,       true, {} },
        { "uid_str",    uid_str,        true, {} },
        { "uid_time",   uid_time,       true, {} },
#if defined(ZHASH_HAS_MD5) && defined(ZHASH_HAS_SHA1)
        { "uid_name",   uid_name,       true, {} },
#endif
        { "options",    options,        true, {} },
    };
}

}
