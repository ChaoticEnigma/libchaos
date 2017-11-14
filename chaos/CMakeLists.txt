## LibChaos Core CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.3)

PROJECT(LibChaos)

### =================== SOURCES =================== ###

SET(LibChaos_SOURCES
    ztypes.h

    abstract/yindexedaccess.h
    abstract/ypushpopaccess.h
    abstract/zaccessor.h
    abstract/zblockaccessor.h
    abstract/zbase.h
    abstract/zhashable.h
    abstract/ziterator.h
    abstract/zposition.h
    abstract/zreader.h
    abstract/zserializer.h
    abstract/zstorage.h
    abstract/zwriter.h

    base/zclock.h
    base/zclock.cpp
    base/zerror.h
    base/zerror.cpp
    base/zexception.h
    base/zexception.cpp
    base/zlog.h
    base/zlog.cpp
    base/zlogworker.h
    base/zlogworker.cpp
    base/ztime.h
    base/ztime.cpp

    data/zallocator.h
    data/zarray.h
    data/zbinary.h
    data/zbinary.cpp
    data/zdata.h
    data/zgraph.h
    data/zlist.h
    data/zmap.h
    data/zpointer.h
    data/zqueue.h
    data/zset.h
    data/zstack.h
    data/ztable.h
    data/ztable.cpp

    file/yimagebackend.h
    file/zbitmap.h
    file/zfile.h
    file/zfile.cpp
    file/zimage.h
    file/zimage.cpp
    file/zpdf.h
    file/zpdf.cpp

    file/image/zbmp.h
    file/image/zbmp.cpp
    file/image/zppm.h
    file/image/zppm.cpp

    lib/xxhash.h
    lib/xxhash.c

    math/zexpression.h
    math/zexpression.cpp
    math/zformula.h
    math/zformula.cpp
    math/znumber.h
    math/znumber.cpp
    #math/znumber-reference.cpp
    math/zmath.h
    math/zmath.cpp

    misc/zencrypt.h
    misc/zencrypt.cpp
    misc/zhash.h
    misc/zhash.cpp
    misc/zoptions.h
    misc/zoptions.cpp
    misc/zrandom.h
    misc/zrandom.cpp
    misc/zret.h
    misc/zuid.h
    misc/zuid.cpp

    net/zaddress.h
    net/zaddress.cpp
    net/zconnection.h
    net/zconnection.cpp
    net/zdatagramsocket.h
    net/zdatagramsocket.cpp
    net/zsocket.h
    net/zsocket.cpp
    net/zstreamsocket.h
    net/zstreamsocket.cpp

    string/zjson.h
    string/zjson.cpp
    string/zpath.h
    string/zpath.cpp
    string/zstring.h
    string/zstring.cpp
    string/zstring-unicode.cpp
    string/zxml.h
    string/zxml.cpp

    thread/zcondition.h
    thread/zcondition.cpp
    thread/zlock.h
    #thread/zlock.cpp
    thread/zmutex.h
    thread/zmutex.cpp
    thread/zthread.h
    thread/zthread.cpp
    thread/zworkqueue.h
)

SET(LibChaos_SETTINGS_SOURCES
    file/zsettings.h
    file/zsettings.cpp
)

SET(LibChaos_PNG_SOURCES
    file/image/zpng.h
    file/image/zpng.cpp
)
SET(LibChaos_JPEG_SOURCES
    file/image/zjpeg.h
    file/image/zjpeg.cpp
)
SET(LibChaos_WEBP_SOURCES
    file/image/zwebp.h
    file/image/zwebp.cpp
)

SET(LibChaos_DATABASE_SOURCES
    file/zdatabase.h
    file/zdatabase.cpp
)

SET(LibChaos_WIN32_SOURCES
    lib/StackWalker.h
    lib/StackWalker.cpp
)

IF(LIBCHAOS_WITH_CRYPTO OR WIN32)
    SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_SETTINGS_SOURCES})
ENDIF()

IF(LIBCHAOS_WITH_PNG)
    SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_PNG_SOURCES})
ENDIF()
IF(LIBCHAOS_WITH_JPEG)
    SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_JPEG_SOURCES})
ENDIF()
IF(LIBCHAOS_WITH_WEBP)
    SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_WEBP_SOURCES})
ENDIF()

IF(LIBCHAOS_WITH_SQLITE3)
    SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_DATABASE_SOURCES})
ENDIF()

IF(WIN32)
    #SET(LibChaos_SOURCES ${LibChaos_SOURCES} ${LibChaos_WIN32_SOURCES})
ENDIF()

CollectSources(
    ${LibChaos_SOURCES}
    ${LibChaos_PNG_SOURCES}
    ${LibChaos_JPEG_SOURCES}
    ${LibChaos_WEBP_SOURCES}
    ${LibChaos_DATABASE_SOURCES}
    ${LibChaos_WIN32_SOURCES}
)

### =================== CONFIG =================== ###

# Visual Studio project filters
SOURCE_GROUP(""         FILES ztypes.h)
SOURCE_GROUP(abstract   REGULAR_EXPRESSION "abstract/.*")
SOURCE_GROUP(base       REGULAR_EXPRESSION "base/.*")
SOURCE_GROUP(data       REGULAR_EXPRESSION "data/.*")
SOURCE_GROUP(file       REGULAR_EXPRESSION "file/.*")
SOURCE_GROUP(math       REGULAR_EXPRESSION "math/.*")
SOURCE_GROUP(misc       REGULAR_EXPRESSION "misc/.*")
SOURCE_GROUP(net        REGULAR_EXPRESSION "net/.*")
SOURCE_GROUP(string     REGULAR_EXPRESSION "string/.*")
SOURCE_GROUP(thread     REGULAR_EXPRESSION "thread/.*")

# Disable warnings on lib files
SET_SOURCE_FILES_PROPERTIES(lib/xxhash.c PROPERTIES COMPILE_FLAGS -w)

IF(LIBCHAOS_BUILD_SHARED)
    SET(LIBCHAOS_LINK SHARED)
ELSE()
    SET(LIBCHAOS_LINK STATIC)
ENDIF()

### =================== BUILD =================== ###

ADD_LIBRARY(chaos ${LIBCHAOS_LINK} ${LibChaos_SOURCES})

SET_PROPERTY(TARGET chaos PROPERTY C_STANDARD 11)
SET_PROPERTY(TARGET chaos PROPERTY CXX_STANDARD 14)
SET_PROPERTY(TARGET chaos PROPERTY CXX_STANDARD_REQUIRED 11)

# Includes
TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC
    .
    abstract
    base
    data
    file
    file/image
    lib
    math
    misc
    net
    string
    thread
)

# Defines
TARGET_COMPILE_DEFINITIONS(chaos PUBLIC BUILDING _LIBCHAOS_DESCRIBE="${LIBCHAOS_DESCRIBE}")

# Build Type
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC _LIBCHAOS_BUILD_DEBUG)
ELSEIF(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC _LIBCHAOS_BUILD_DEBUG)
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC _LIBCHAOS_BUILD_RELEASE)
ELSE()
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC _LIBCHAOS_BUILD_RELEASE)
ENDIF()

# MinGW / MSYS Options
IF(MINGW OR MSYS)
    TARGET_COMPILE_OPTIONS(chaos PRIVATE -mwin32 -mwindows -static-libgcc -static-libstdc++)
ENDIF()

# MSVC Options
IF(MSVC)
    SET_PROPERTY(TARGET chaos PROPERTY LINK_FLAGS "/ignore:4221")
ENDIF()

# GCC / Clang Options
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
   CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    TARGET_COMPILE_OPTIONS(chaos PRIVATE
        # Warning groups
        -Wall -Wextra -Wpedantic
        # Casting
        -Wcast-qual -Wsign-conversion
        # Formatting
        -Wformat=2 -Wstrict-overflow=5 -Wswitch-default
        # Declarations
        -Winit-self -Wredundant-decls -Wundef
        -Wmissing-include-dirs -Wdisabled-optimization

        # C++ warnings
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-promo -Woverloaded-virtual -Wctor-dtor-privacy>

        # Disabled warnings
        -Wno-unused -Wno-unused-parameter -Wno-expansion-to-defined

        # Forced errors
        -Werror=return-type -Werror=delete-incomplete

        # GCC-only
        $<$<C_COMPILER_ID:GNU>:-Wlogical-op>
        $<$<C_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:-Wnoexcept -Wstrict-null-sentinel>>
        # Clang-only
        $<$<C_COMPILER_ID:Clang>:-Wno-nested-anon-types>
    )
ENDIF()

# Crypto
IF(LIBCHAOS_WITH_CRYPTO)
    TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC ${LIBCRYPTO_INCLUDE})
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC LIBCHAOS_HAS_CRYPTO)
    TARGET_LINK_LIBRARIES(chaos ${LIBCRYPTO_LIB})
ENDIF()
# PNG
IF(LIBCHAOS_WITH_PNG)
    TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC ${ZLIB_INCLUDE} ${LIBPNG_INCLUDE})
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC LIBCHAOS_HAS_PNG PNG_STATIC)
    TARGET_LINK_LIBRARIES(chaos ${LIBPNG_LIB} ${ZLIB_LIB})
ENDIF()
# JPEG
IF(LIBCHAOS_WITH_JPEG)
    TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC ${LIBJPEG_INCLUDE})
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC LIBCHAOS_HAS_JPEG)
    TARGET_LINK_LIBRARIES(chaos ${LIBJPEG_LIB})
ENDIF()
# WEBP
IF(LIBCHAOS_WITH_WEBP)
    TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC ${LIBWEBP_INCLUDE})
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC LIBCHAOS_HAS_WEBP)
    TARGET_LINK_LIBRARIES(chaos ${LIBWEBP_LIB})
ENDIF()
# Sqlite3
IF(LIBCHAOS_WITH_SQLITE3)
    TARGET_INCLUDE_DIRECTORIES(chaos PUBLIC ${LIBSQLITE3_INCLUDE})
    TARGET_COMPILE_DEFINITIONS(chaos PUBLIC LIBCHAOS_HAS_SQLITE3)
    TARGET_LINK_LIBRARIES(chaos ${LIBSQLITE3_LIB})
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    IF(MSVC)
        TARGET_LINK_LIBRARIES(chaos Ws2_32 iphlpapi)
    ELSE()
        TARGET_LINK_LIBRARIES(chaos pthread)
        TARGET_LINK_LIBRARIES(chaos ws2_32 dbghelp kernel32 psapi iphlpapi version)
    ENDIF()
ELSE()
    IF(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
        # For zclock
        TARGET_LINK_LIBRARIES(chaos rt)
    ENDIF()

    # For pthreads
    TARGET_LINK_LIBRARIES(chaos pthread)
    IF(IBERTY_DEMANGLE)
        TARGET_LINK_LIBRARIES(chaos iberty)
    ENDIF()

    IF(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        TARGET_LINK_LIBRARIES(chaos execinfo)
    ELSE()
        TARGET_LINK_LIBRARIES(chaos dl)
    ENDIF()
ENDIF()
