## LibChaos CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(LibChaos)

# Build Modes
OPTION(EXTRA_WARNINGS           "Produce Extra Compiler Warnings"           OFF )
OPTION(BUILD_DOCS               "Documentation should be configured"        ON  )

# General Options
OPTION(LIBCHAOS_SHARED          "Dependencies link with shared library"     ON )
OPTION(LIBCHAOS_LINK_STATIC     "Link with static libraries"                OFF )
OPTION(LIBCHAOS_LINK_SHARED     "Link with shared libraries"                OFF )
#OPTION(IBERTY_DEMANGLE          "Use libiberty to demangle symbols"         OFF )

# Library Components
OPTION(LIBCHAOS_UI              "Build LibChaos UI library"                 OFF )

# Third-Party Library Support
OPTION(DISABLE_CRYPTO           "Disable Crypto support"                    OFF )
OPTION(DISABLE_ZPNG             "Disable PNG support"                       OFF )
OPTION(DISABLE_ZJPEG            "Disable JPEG support"                      OFF )
OPTION(DISABLE_ZWEBP            "Disable WebP support"                      OFF )
OPTION(DISABLE_ZDATABASE        "Disable SQLite3 support"                   OFF )

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    OPTION(REQUIRE_CRYPTO           "Require Crypto support"                OFF )
ELSE()
    OPTION(REQUIRE_CRYPTO           "Require Crypto support"                ON  )
ENDIF()
OPTION(REQUIRE_ZJPEG            "Require JPEG support"                      OFF )
OPTION(REQUIRE_ZPNG             "Require PNG support"                       OFF )
OPTION(REQUIRE_ZWEBP            "Require WebP support"                      OFF )
OPTION(REQUIRE_ZDATABASE        "Require SQLite3 support"                   OFF )

# Find libraries
INCLUDE(cmake/FindLibs.cmake)

INCLUDE(cmake/Utils.cmake)

# CTest
INCLUDE(CTest)
ENABLE_TESTING()

# Readme Files
SET(LibChaos_OTHER_FILES
    README.md
    .travis.yml
    doc/README.md
    doc/COPYRIGHT
    doc/LICENSE
    doc/Projects.txt
    doc/LibChaos_Todo_Ideas.txt
    doc/Doxyfile.in
    doc/Doxyfile-help.in
)

# Add targets to all
IF(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    SET(LIBCHAOS_ALL EXCLUDE_FROM_ALL)
    SET(TEST_ALL EXCLUDE_FROM_ALL)
ENDIF()

# LibChaos
ADD_SUBDIRECTORY(chaos)

# LibChaos UI
IF(LIBCHAOS_UI)
    ADD_SUBDIRECTORY(ui)
ENDIF()

# LibChaos Test
ADD_SUBDIRECTORY(tests)

# Export configuration variables
GET_DIRECTORY_PROPERTY(HAS_PARENT PARENT_DIRECTORY)
IF(HAS_PARENT)
    SET(LibChaos_With_Crypto    ${LIBCHAOS_WITH_CRYPTO}     PARENT_SCOPE)
    SET(LibChaos_With_PNG       ${LIBCHAOS_WITH_PNG}        PARENT_SCOPE)
    SET(LibChaos_With_JPEG      ${LIBCHAOS_WITH_JPEG}       PARENT_SCOPE)
    SET(LibChaos_With_WebP      ${LIBCHAOS_WITH_WEBP}       PARENT_SCOPE)
    SET(LibChaos_With_SQLite3   ${LIBCHAOS_WITH_SQLITE3}    PARENT_SCOPE)
ENDIF()

# Doxygen
INCLUDE(cmake/Doxygen.cmake)

# Add All Sources to Dummy Target for IDE File Listing
GET_PROPERTY(LibChaosAllSources GLOBAL PROPERTY LibChaosAllSources)
ADD_CUSTOM_TARGET(libchaos_dummy SOURCES
    ${LibChaosAllSources}
    ${LibChaos_OTHER_FILES}
)
