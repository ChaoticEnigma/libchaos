## Utility Functions and Macros

# Build Type
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(TYPE_STR "Debug")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "Release")
    SET(TYPE_STR "Release")
ELSE()
    SET(TYPE_STR "Other")
ENDIF()

# Linkage
IF(LIBCHAOS_BUILD_SHARED)
    SET(LINK_TYPE "Shared")
ELSE()
    SET(LINK_TYPE "Static")
ENDIF()

# Libraries
IF(LIBCHAOS_WITH_CRYPTO)
    SET(CRYPTO_STR "Crypto,")
ENDIF()
IF(LIBCHAOS_WITH_PNG)
    SET(PNG_STR "PNG,")
ENDIF()
IF(LIBCHAOS_WITH_JPEG)
    SET(JPEG_STR "JPEG,")
ENDIF()
IF(LIBCHAOS_WITH_WEBP)
    SET(WEBP_STR "WebP,")
ENDIF()
IF(LIBCHAOS_WITH_SQLITE3)
    SET(SQLITE3_STR "SQlite3,")
ENDIF()

SET(LIBS_STR "Core,${CRYPTO_STR}${PNG_STR}${JPEG_STR}${WEBP_STR}${SQLITE3_STR}")
STRING(LENGTH ${LIBS_STR} LEN0)
MATH(EXPR LEN "${LEN0} - 1")
STRING(SUBSTRING ${LIBS_STR} 0 ${LEN} LIBS_STR)

# Git Describe
MACRO(GitDescribe DIR OUTPUT)
    EXECUTE_PROCESS(
        WORKING_DIRECTORY "${DIR}"
        COMMAND git describe --all --long --always --dirty=*
        OUTPUT_VARIABLE ${OUTPUT}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDMACRO()

GitDescribe("${CMAKE_CURRENT_SOURCE_DIR}" LIBCHAOS_DESCRIBE)

# Build Status Line
MESSAGE(STATUS "LibChaos: ${TYPE_STR} (${LINK_TYPE}) - ${LIBS_STR} - ${LIBCHAOS_DESCRIBE}")

# Add each argument to the global source list
FUNCTION(CollectSources)
    GET_PROPERTY(LibChaosAllSources GLOBAL PROPERTY LibChaosAllSources)
    FOREACH(SRC ${ARGV})
        SET(LibChaosAllSources ${LibChaosAllSources} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC})
    ENDFOREACH()
    SET_PROPERTY(GLOBAL PROPERTY LibChaosAllSources ${LibChaosAllSources})
ENDFUNCTION()

# Download a file at URL to FILE in build dir
FUNCTION(DownloadFile URL FILE)
    SET(PATH ${CMAKE_CURRENT_BINARY_DIR}/${FILE})
    IF(NOT EXISTS "${PATH}")
        MESSAGE(STATUS "Downloading ${FILE}")
        FILE(DOWNLOAD "${URL}" "${PATH}")
    ENDIF()
ENDFUNCTION()

# Download a tar.* archive at URL and extract to ARCHIVE in build dir
FUNCTION(DownloadArchive URL ARCHIVE)
    SET(PATH ${CMAKE_CURRENT_BINARY_DIR}/${ARCHIVE})
    IF(NOT EXISTS "${PATH}")
        MESSAGE(STATUS "Downloading ${ARCHIVE}")
        FILE(DOWNLOAD "${URL}" "${PATH}")
        GET_FILENAME_COMPONENT(NAME "${ARCHIVE}" NAME_WE)
        EXECUTE_PROCESS(
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${NAME}"
        )
        EXECUTE_PROCESS(
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${NAME}
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${PATH}"
        )
    ENDIF()
ENDFUNCTION()
