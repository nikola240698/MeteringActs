set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(MeteringActs
        main.cpp
        database.cpp
        mainwindow.cpp
        createactwidget.cpp
        meterdialog.cpp
        meteractwidget.cpp

        ${MY_INCLUDE_DIR}/database.h
        ${MY_INCLUDE_DIR}/mainwindow.h
        ${MY_INCLUDE_DIR}/createactwidget.h
        ${MY_INCLUDE_DIR}/meterdialog.h
        ${MY_INCLUDE_DIR}/meteractwidget.h

        ${MY_UI_DIR}/mainwindow.ui
        ${MY_UI_DIR}/createactwidget.ui
        ${MY_UI_DIR}/meterdialog.ui
        ${MY_UI_DIR}/meteractwidget.ui
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