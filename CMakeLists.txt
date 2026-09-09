cmake_minimum_required(VERSION 3.16)
project(MeteringActs VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(MY_UI_DIR ${CMAKE_SOURCE_DIR}/ui)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Sql
        REQUIRED
)

add_subdirectory(src)