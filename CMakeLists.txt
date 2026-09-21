cmake_minimum_required(VERSION 3.25)

project(Recrayon
    VERSION 1.0.0
    DESCRIPTION "Draw on top of your screen while you keep working"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---- Options -----------------------------------------------------------------
option(RECRAYON_BUILD_TESTS "Build the unit tests" ON)
option(RECRAYON_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(RECRAYON_ENABLE_RECORDING "Screen recording (requires Qt Multimedia)" ON)
option(RECRAYON_DEPLOY_ON_BUILD
    "Windows: copy the Qt runtime next to the executable after each link so it runs by double-click"
    ${WIN32})

# ---- Language ----------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Dependencies ------------------------------------------------------------
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup(REQUIRES 6.5)

# Recording needs Qt Multimedia 6.8+ (QVideoFrameInput for composed recordings).
if(RECRAYON_ENABLE_RECORDING)
    find_package(Qt6 6.8 QUIET COMPONENTS Multimedia)
    if(NOT Qt6Multimedia_FOUND)
        message(WARNING "Qt Multimedia 6.8+ not found: screen recording will be disabled")
    endif()
endif()

# Translations: lupdate / lrelease (Qt Linguist tools).
find_package(Qt6 QUIET COMPONENTS LinguistTools)
if(NOT Qt6LinguistTools_FOUND)
    message(WARNING "Qt Linguist tools not found: the application will only be in English")
endif()

include(CompilerWarnings)
include(Tooling)

# ---- Targets -----------------------------------------------------------------
add_subdirectory(src)

if(RECRAYON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
