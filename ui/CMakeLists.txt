## LibChaos UI CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(LibChaosUI)

### =================== HEADERS =================== ###

SET(LibChaosUI_HEADERS
    zui.h
    align.h
    base.h
    window.h
    control.h
    button.h
    edit.h
    font.h
    table.h
    progress.h
)

### =================== SOURCES =================== ###

SET(LibChaosUI_X11_SOURCES
    align.cpp
    button.cpp
    control.cpp
    edit.cpp
    font.cpp
    progress.cpp
    table.cpp
    window.cpp
    windowproc.cpp
)

SET(LibChaosUI_WIN32_SOURCES
    win32/align_win32.cpp
    win32/button_win32.cpp
    win32/control_win32.cpp
    win32/edit_win32.cpp
    win32/font_win32.cpp
    win32/progress_win32.cpp
    win32/table_win32.cpp
    win32/window_win32.cpp
    win32/windowproc_win32.cpp
)

### =================== CONFIG =================== ###

IF(LIBCHAOS_PLATFORM_WINDOWS)
    SET(LibChaosUI_SOURCES ${LibChaosUI_WIN32_SOURCES})
ELSE()
    SET(LibChaosUI_SOURCES ${LibChaosUI_X11_SOURCES})
ENDIF()

# Export Properties
GatherSources(${LibChaosUI_HEADERS} ${LibChaosUI_X11_SOURCES} ${LibChaosUI_WIN32_SOURCES})
GatherIncludes(.)

# Include Directories
GET_PROPERTY(LibChaosInclude GLOBAL PROPERTY LibChaosInclude)
INCLUDE_DIRECTORIES(
    ${LibChaosInclude}
)

### =================== BUILD =================== ###

IF(LIBCHAOS_PLATFORM_WINDOWS)
    ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ENDIF()

ADD_LIBRARY(chaosui EXCLUDE_FROM_ALL ${LibChaosUI_HEADERS} ${LibChaosUI_SOURCES})
SET_PROPERTY(TARGET chaosui PROPERTY CXX_STANDARD 14)
SET_PROPERTY(TARGET chaosui PROPERTY CXX_STANDARD_REQUIRED 11)
SET_PROPERTY(TARGET chaosui PROPERTY COMPILE_FLAGS ${LIBCHAOS_CXX_FLAGS})
IF(LIBCHAOS_PLATFORM_WINDOWS)
    TARGET_LINK_LIBRARIES(chaosui chaos comctl32)
ELSE()
    TARGET_LINK_LIBRARIES(chaosui chaos)
ENDIF()
