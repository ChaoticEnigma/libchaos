#f # LibChaos Test CMakeLists.txt
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

ADD_EXECUTABLE(testchaos-shared ${TEST_ALL} ${LibChaos_Test_SOURCES})
SET_PROPERTY(TARGET testchaos-shared PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(testchaos-shared chaos-shared)

ADD_EXECUTABLE(testchaos-static ${TEST_ALL} ${LibChaos_Test_SOURCES})
SET_PROPERTY(TARGET testchaos-static PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(testchaos-static chaos-static)

### =================== TESTS =================== ###

#ADD_TEST(NAME "Allocator" CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> allocator)
#ADD_TEST(NAME "Pointer"   CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> pointer)
#ADD_TEST(NAME "Storage"   CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> storage)
#ADD_TEST(NAME "Binary"    CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> binary)
#
#ADD_TEST(NAME "Array"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> array)
#ADD_TEST(NAME "Assoc"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> assoc)
#ADD_TEST(NAME "Stack"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> stack)
#ADD_TEST(NAME "Map"       CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> map)
#ADD_TEST(NAME "List"      CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> list)
#ADD_TEST(NAME "Queue"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> queue)
#
#ADD_TEST(NAME "String"    CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> string)
#ADD_TEST(NAME "Path"      CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> path)
#ADD_TEST(NAME "JSON"      CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> json)
#ADD_TEST(NAME "Number"    CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> number)
#
#ADD_TEST(NAME "Hash"      CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> hash)
#ADD_TEST(NAME "UID"       CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> uid)
#
#ADD_TEST(NAME "Thread"    CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> thread)
#ADD_TEST(NAME "Mutex"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> mutex)
#
#ADD_TEST(NAME "File"      CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> file)
#ADD_TEST(NAME "Image"     CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> image)
#ADD_TEST(NAME "PDF"       CONFIGURATIONS testchaos-shared COMMAND $<TARGET_FILE:testchaos-shared> pdf)
