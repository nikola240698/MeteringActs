

#include "mainwindow.h"
#include "ui_MainWindow.h"

#include "createactwidget.h"
#include "archivewidget.h"


MainWindow::MainWindow(Database &database, QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow), m_database(database)
{
    ui->setupUi(this);

    setWindowTitle("Акты приборов учета");

    // создаем виджет первого окна и добавляем его в stackWidget
    auto* createActWidget = new CreateActWidget(m_database, this);
    ui->stackedWidget->insertWidget(0, createActWidget);

    // Создаем виджет третьего окна и добавляем в stackWidget
    auto* archiveWidget = new ArchiveWidget(m_database, this);
    ui->stackedWidget->insertWidget(2, archiveWidget);


    // создаем группу кнопок для возможности уникального выбора
    auto *navigationGroup = new QButtonGroup(this);
    // разрешаем только уникальный выбор
    navigationGroup->setExclusive(true);
    // добавляем кнопки в группу
    navigationGroup->addButton(ui->createActButton, 0);
    navigationGroup->addButton(ui->metersButton, 1);
    navigationGroup->addButton(ui->archiveButton, 2);
    navigationGroup->addButton(ui->directoriesButton, 3);
    navigationGroup->addButton(ui->settingsButton, 4);
    // прописываем условие работы кнопок
    connect(navigationGroup, &QButtonGroup::idClicked, this,
        [this](int id)
            {
                ui->stackedWidget->setCurrentIndex(id);
            });
    // выбираем начальные условия создания окна
    ui->createActButton->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);




}



MainWindow::~MainWindow()
{
    delete ui;
}


