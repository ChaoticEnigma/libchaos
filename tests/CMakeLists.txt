# LibChaos Test CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(LibChaos_Test)

### =================== SOURCES =================== ###

SET(LibChaos_Test_SOURCES
    tests.h
    main.cpp
    sandbox.cpp

    test_pointer.cpp
    test_allocator.cpp
    test_binary.cpp

    test_array.cpp
    test_list.cpp
    test_iterator.cpp

    test_table.cpp
    test_graph.cpp

    test_string.cpp
    test_path.cpp
    test_json.cpp
    test_number.cpp

    test_thread.cpp

    test_misc.cpp
    test_hash.cpp

    test_file.cpp
    test_image.cpp
    test_pdf.cpp

    test_socket.cpp
    test_http.cpp

    test_error.cpp
)

#CollectSources(${LibChaos_Test_SOURCES})

### =================== CONFIG =================== ###

# Download test images
IF(LIBCHAOS_TEST AND LIBCHAOS_WITH_PNG)
    DownloadArchive("https://imagetestsuite.googlecode.com/files/imagetestsuite-png-1.01.tar.gz" testpng.tar.gz)
    DownloadArchive("http://www.schaik.com/pngsuite/PngSuite-2013jan13.tgz" pngsuite.tar.gz)
ENDIF()
IF(LIBCHAOS_TEST AND LIBCHAOS_WITH_JPEG)
    DownloadFile("http://www.gstatic.com/webp/gallery/4.jpg" tree.jpg)
    DownloadArchive("https://imagetestsuite.googlecode.com/files/imagetestsuite-jpg-1.00.tar.gz" testjpeg.tar.gz)
ENDIF()
IF(LIBCHAOS_TEST AND LIBCHAOS_WITH_WEBP)
    DownloadFile("https://www.gstatic.com/webp/gallery3/3_webp_ll.webp" dice.webp)
ENDIF()

### =================== BUILD =================== ###

ADD_EXECUTABLE(testchaos-static ${TEST_ALL} ${LibChaos_Test_SOURCES})
SET_PROPERTY(TARGET testchaos-static PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(testchaos-static chaos-static)

ADD_EXECUTABLE(testchaos-shared ${TEST_ALL} ${LibChaos_Test_SOURCES})
SET_PROPERTY(TARGET testchaos-shared PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(testchaos-shared chaos-shared)

ADD_CUSTOM_TARGET(testchaos DEPENDS testchaos-shared testchaos-static)

### =================== TESTS =================== ###

MACRO(TestChaos_Tests TARGET)
    ADD_TEST(NAME "${TARGET} Allocator" CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> allocator)
    ADD_TEST(NAME "${TARGET} Pointer"   CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> pointer)
    ADD_TEST(NAME "${TARGET} Storage"   CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> storage)
    ADD_TEST(NAME "${TARGET} Binary"    CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> binary)

    ADD_TEST(NAME "${TARGET} Array"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> array)
    ADD_TEST(NAME "${TARGET} Assoc"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> assoc)
    ADD_TEST(NAME "${TARGET} Stack"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> stack)
    ADD_TEST(NAME "${TARGET} Map"       CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> map)
    ADD_TEST(NAME "${TARGET} List"      CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> list)
    ADD_TEST(NAME "${TARGET} Queue"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> queue)

    ADD_TEST(NAME "${TARGET} String"    CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> string)
    ADD_TEST(NAME "${TARGET} Path"      CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> path)
    ADD_TEST(NAME "${TARGET} JSON"      CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> json)
    ADD_TEST(NAME "${TARGET} Number"    CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> number)

    ADD_TEST(NAME "${TARGET} Hash"      CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> hash)
    ADD_TEST(NAME "${TARGET} UID"       CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> uid)

    ADD_TEST(NAME "${TARGET} Thread"    CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> thread)
    ADD_TEST(NAME "${TARGET} Mutex"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> mutex)

    ADD_TEST(NAME "${TARGET} File"      CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> file)
    ADD_TEST(NAME "${TARGET} Image"     CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> image)
    ADD_TEST(NAME "${TARGET} PDF"       CONFIGURATIONS ${TARGET} COMMAND $<TARGET_FILE:${TARGET}> pdf)
ENDMACRO()

TestChaos_Tests(testchaos-static)
TestChaos_Tests(testchaos-shared)
