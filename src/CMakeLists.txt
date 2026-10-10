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
        externalrepresentativewidget.cpp
        vectordiagramwidget.cpp
        currenttransformeractwidget.cpp
        currenttransformerdialog.cpp
        actrepository.cpp
        docxgenerator.cpp
        actcalculator.cpp
        archivewidget.cpp
        actviewdialog.cpp
        editactdialog.cpp
        directorieswidget.cpp
        areadialog.cpp
        substationdialog.cpp
        connectiondialog.cpp
        employeedialog.cpp
        meterswidget.cpp
        currenttransformerswidget.cpp
        equipmentwidget.cpp
        databasebackupmanager.cpp
        settingswidget.cpp
        currenttransformerphasewidget.cpp
        currenttransformerphasepanel.cpp

        ${MY_INCLUDE_DIR}/database.h
        ${MY_INCLUDE_DIR}/mainwindow.h
        ${MY_INCLUDE_DIR}/createactwidget.h
        ${MY_INCLUDE_DIR}/meterdialog.h
        ${MY_INCLUDE_DIR}/meteractwidget.h
        ${MY_INCLUDE_DIR}/externalrepresentativewidget.h
        ${MY_INCLUDE_DIR}/vectordiagramwidget.h
        ${MY_INCLUDE_DIR}/currenttransformeractwidget.h
        ${MY_INCLUDE_DIR}/currenttransformerdialog.h
        ${MY_INCLUDE_DIR}/actrepository.h
        ${MY_INCLUDE_DIR}/actdata.h
        ${MY_INCLUDE_DIR}/docxgenerator.h
        ${MY_INCLUDE_DIR}/actcalculator.h
        ${MY_INCLUDE_DIR}/archivewidget.h
        ${MY_INCLUDE_DIR}/actviewdialog.h
        ${MY_INCLUDE_DIR}/editactdialog.h
        ${MY_INCLUDE_DIR}/directorieswidget.h
        ${MY_INCLUDE_DIR}/areadialog.h
        ${MY_INCLUDE_DIR}/substationdialog.h
        ${MY_INCLUDE_DIR}/connectiondialog.h
        ${MY_INCLUDE_DIR}/employeedialog.h
        ${MY_INCLUDE_DIR}/meterswidget.h
        ${MY_INCLUDE_DIR}/currenttransformerswidget.h
        ${MY_INCLUDE_DIR}/equipmentwidget.h
        ${MY_INCLUDE_DIR}/databasebackupmanager.h
        ${MY_INCLUDE_DIR}/settingswidget.h
        ${MY_INCLUDE_DIR}/currenttransformerphasewidget.h
        ${MY_INCLUDE_DIR}/currenttransformerphasepanel.h

        ${MY_UI_DIR}/mainwindow.ui
        ${MY_UI_DIR}/createactwidget.ui
        ${MY_UI_DIR}/meterdialog.ui
        ${MY_UI_DIR}/meteractwidget.ui
        ${MY_UI_DIR}/externalrepresentativewidget.ui
        ${MY_UI_DIR}/vectordiagramwidget.ui
        ${MY_UI_DIR}/currenttransformeractwidget.ui
        ${MY_UI_DIR}/currenttransformerdialog.ui
        ${MY_UI_DIR}/archivewidget.ui
        ${MY_UI_DIR}/actviewdialog.ui
        ${MY_UI_DIR}/editactdialog.ui
        ${MY_UI_DIR}/directorieswidget.ui
        ${MY_UI_DIR}/areadialog.ui
        ${MY_UI_DIR}/substationdialog.ui
        ${MY_UI_DIR}/connectiondialog.ui
        ${MY_UI_DIR}/employeedialog.ui
        ${MY_UI_DIR}/meterswidget.ui
        ${MY_UI_DIR}/currenttransformerswidget.ui
        ${MY_UI_DIR}/equipmentwidget.ui
        ${MY_UI_DIR}/settingswidget.ui
        ${MY_UI_DIR}/currenttransformerphasewidget.ui


        ${CMAKE_SOURCE_DIR}/resources/resources.qrc
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
        Qt6::Xml
        QuaZip::QuaZip
)