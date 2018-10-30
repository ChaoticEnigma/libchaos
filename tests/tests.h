#ifndef TEST_H
#define TEST_H

#include "zlog.h"
#include "zmap.h"

// Assert for Tests
#define TASSERT(X) if(!(X)) throw __LINE__;

using namespace LibChaos;

namespace LibChaosTest {

typedef void (*test_func)(void);

enum test_status {
    NONE,
    PASS,
    FAIL,
};

struct Test {
    ZString name;
    test_func func;
    bool run;
    ZArray<ZString> pre_deps;
};

typedef ZArray<Test> (*reg_func)(void);

ZArray<Test> allocator_tests();
ZArray<Test> pointer_tests();

ZArray<Test> binary_tests();
ZArray<Test> array_tests();
ZArray<Test> list_tests();

ZArray<Test> string_tests();
ZArray<Test> path_tests();
ZArray<Test> json_tests();

ZArray<Test> hash_tests();
ZArray<Test> table_tests();
ZArray<Test> graph_tests();
ZArray<Test> misc_tests();
ZArray<Test> number_tests();

ZArray<Test> file_tests();
ZArray<Test> image_tests();
ZArray<Test> pdf_tests();

ZArray<Test> socket_tests();
ZArray<Test> http_tests();

ZArray<Test> thread_tests();
ZArray<Test> error_tests();

ZArray<Test> sandbox_tests();

int test_forward_iterator(ZSimplexConstIterator<ZString> *it, zu64 size);
int test_reverse_iterator(ZDuplexIterator<ZString> *it, zu64 size);
int test_duplex_iterator(ZDuplexIterator<ZString> *it, zu64 size);
int test_random_iterator(ZRandomIterator<ZString> *it, zu64 size);

// Test registration functions
static const ZArray<reg_func> regtests = {
    allocator_tests,
    pointer_tests,

    binary_tests,
    array_tests,
    list_tests,

    string_tests,
    path_tests,
    json_tests,

    hash_tests,
    table_tests,
    graph_tests,
    misc_tests,
    number_tests,

    file_tests,
    image_tests,
    pdf_tests,

    socket_tests,

    thread_tests,
    error_tests,

    sandbox_tests
};

}

#endif // TEST_H
