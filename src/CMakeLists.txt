set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(MeteringActs
        main.cpp
        database.cpp

        ${MY_INCLUDE_DIR}/database.h
)

target_include_directories(MeteringActs PRIVATE
        ${MY_INCLUDE_DIR}
        ${MY_UI_DIR}
)


target_compile_definitions(MeteringActs PRIVATE
        PROJECT_ROOT="${CMAKE_SOURCE_DIR}"

)

target_link_libraries(MeteringActs PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
)