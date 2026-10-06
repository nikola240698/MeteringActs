

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "createactwidget.h"
#include "archivewidget.h"
#include "directorieswidget.h"
#include "equipmentwidget.h"
#include "meterswidget.h"
#include "currenttransformerswidget.h"
#include "settingswidget.h"


MainWindow::MainWindow(Database &database, QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow), m_database(database)
{
    ui->setupUi(this);

    setWindowTitle("Акты приборов учета");

    // создаем виджет первого окна и добавляем его в stackWidget
    auto* createActWidget = new CreateActWidget(m_database, this);
    ui->stackedWidget->insertWidget(0, createActWidget);

    // Создаем раздел оборудования
    auto* equipmentWidget = new EquipmentWidget(m_database, this);
    ui->stackedWidget->insertWidget(1, equipmentWidget);

    // Получаем виджет приборов учета для соединения сигналов
    auto* metersWidget = equipmentWidget->metersWidget();

    // Получаем виджет ТТ
    auto* currentTransformersWidget =
        equipmentWidget->currentTransformerWidget();

    // Создаем виджет третьего окна и добавляем в stackWidget
    auto* archiveWidget = new ArchiveWidget(m_database, this);
    ui->stackedWidget->insertWidget(2, archiveWidget);

    // Создаем страницу справочников
    auto* directoriesWidget = new DirectoriesWidget(m_database, this);
    ui->stackedWidget->insertWidget(3, directoriesWidget);

    // Создаем страницу настроек
    auto* settingsWidget =
        new SettingsWidget(m_database, this);
    ui->stackedWidget->insertWidget(4, settingsWidget);

    // Получаем сигнал при изменении списка сотрудников
    connect(directoriesWidget, &DirectoriesWidget::employeesChanged,
        createActWidget, &CreateActWidget::reloadEmployees);

    // Получаем сигнал при изменении списка участков, ПС и присоединений
    connect(directoriesWidget, &DirectoriesWidget::objectChanged,
        createActWidget, &CreateActWidget::reloadObject);

    // Получаем сигнал при сохранении нового прибора
    connect(createActWidget, &CreateActWidget::metersChanged,
        metersWidget, &MetersWidget::reloadMeters);

    // Получаем сигнал при сохранении нового ТТ
    connect(createActWidget, &CreateActWidget::currentTransformersChanged,
        currentTransformersWidget,
        &CurrentTransformersWidget::reloadCurrentTransformers);

    // Получаем сигнал при добавлении нового акта в окне создания
    connect(createActWidget, &CreateActWidget::actCreated,
        archiveWidget, &ArchiveWidget::reloadActs);

    // Получаем сигнал при добавлении нового ТТ в окне создания
    connect(createActWidget, &CreateActWidget::actCreated,
        currentTransformersWidget,
        &CurrentTransformersWidget::reloadCurrentTransformers);

    // Получаем сигнал для обновления истории при создании нового акта
    connect(createActWidget, &CreateActWidget::actCreated,
        metersWidget, &MetersWidget::reloadHistory);

    // Получаем сигналы при изменении акта
    connect(archiveWidget, &ArchiveWidget::actsChanged,
        metersWidget, &MetersWidget::reloadMeters);
    connect(archiveWidget, &ArchiveWidget::actsChanged,
        currentTransformersWidget,
        &CurrentTransformersWidget::reloadCurrentTransformers);

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


