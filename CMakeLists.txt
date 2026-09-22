cmake_minimum_required(VERSION 3.16)
project(MeteringActs VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(MY_UI_DIR ${CMAKE_SOURCE_DIR}/ui)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Sql
        Xml
        REQUIRED
)

include(FetchContent)

# =========================================================
# zlib
# =========================================================

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
)

FetchContent_MakeAvailable(zlib)

# =========================================================
# QuaZip
# =========================================================

set(QUAZIP_QT_MAJOR_VERSION 6 CACHE STRING "" FORCE)
set(QUAZIP_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(QUAZIP_INSTALL OFF CACHE BOOL "" FORCE)

# QuaZip вызывает find_package(ZLIB).
# Передаём ему zlib, которую только что собрали.
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIRS
        "${zlib_SOURCE_DIR}"
        "${zlib_BINARY_DIR}"
)
set(ZLIB_INCLUDE_DIR
        "${zlib_SOURCE_DIR}"
)

set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_LIBRARY zlibstatic)

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        QuaZip
        GIT_REPOSITORY https://github.com/stachenov/quazip.git
        GIT_TAG v1.5
)

FetchContent_MakeAvailable(QuaZip)


add_subdirectory(src)