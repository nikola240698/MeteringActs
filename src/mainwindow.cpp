

#include "mainwindow.h"
#include "ui_MainWindow.h"


MainWindow::MainWindow(Database &database, QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow), m_database(database)
{
    ui->setupUi(this);

    setupNavigation();

    setCurrentPage(0);




}



MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNavigation()
{
    auto *navigationGroup = new QButtonGroup(this);

    navigationGroup->setExclusive(true);

    navigationGroup->addButton(ui->createActButton, 0);
    navigationGroup->addButton(ui->mettersButton, 1);
    navigationGroup->addButton(ui->archiveButton, 2);
    navigationGroup->addButton(ui->directoriesButton, 3);
    navigationGroup->addButton(ui->settingsButton, 4);


    connect(navigationGroup, &QButtonGroup::idClicked, this,
        [this](int id)
            {
                ui->stackedWidget->setCurrentIndex(id);
            });


}

void MainWindow::setCurrentPage(const int index) const
{
    ui->stackedWidget->setCurrentIndex(index);
}
