
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHeaderView>

#include "directorieswidget.h"
#include "ui_directorieswidget.h"


DirectoriesWidget::DirectoriesWidget(Database &database, QWidget *parent)
    : QWidget(parent), ui(new Ui::DirectoriesWidget), m_database(database)
{
    ui->setupUi(this);

    // Скрываем по умолчания кнопку "Назад"
    ui->backButton->setVisible(false);

    // создаем модели
    m_leftModel = new QSqlQueryModel(this);
    m_rightModel = new QSqlQueryModel(this);
    // вставляем их в виджеты
    ui->leftTableView->setModel(m_leftModel);
    ui->rightTableView->setModel(m_rightModel);

    loadAreas();

    // Слот одиночного нажатия на строку в таблице участка
    connect(ui->leftTableView, &QTableView::clicked, this,
        [this](const QModelIndex &index)
        {
           if (!index.isValid())
           {
               return;
           }

            const int id = m_leftModel->index(index.row(), 0).data().toInt();

            if (m_level == DirectoryLevel::Areas)
            {
                loadSubstations(id);
            }
            else if (m_level == DirectoryLevel::Substations)
            {
                loadConnections(id);
            }
        });

    clearSubstations();

    // Подключаем двойной клик на строке левой таблицы
    connect(ui->leftTableView, &QTableView::doubleClicked, this,
        [this](const QModelIndex &index)
        {
            if (m_level != DirectoryLevel::Areas || !index.isValid())
            {
                return;
            }

            const int areaId = m_leftModel->index(index.row(), 0)
                .data().toInt();

            showSubstationsLevel(areaId);
        });

    // Подключаем слот нажатия кнопки "Назад"
    connect(ui->backButton, &QPushButton::clicked, this,
        [this]()
        {
            showAreasLevel();
        });
}

DirectoriesWidget::~DirectoriesWidget()
{
    delete ui;
}

void DirectoriesWidget::loadAreas()
{
    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, name "
        "FROM areas "
        "ORDER BY name;"))
    {
        qDebug() << "loadAreas error: " << query.lastError().text();

        return;
    }
    // Вставляем результат запроса в модель
    m_leftModel->setQuery(std::move(query));
    // Выставляем название столбца
    m_leftModel->setHeaderData(1, Qt::Horizontal, "Название участка");
    // Скрываем столбец с ID
    ui->leftTableView->hideColumn(0);
    // Растягиваем последний столбец согласно содержимому
    ui->leftTableView->horizontalHeader()->setStretchLastSection(true);


}

void DirectoriesWidget::loadSubstations(int areaId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name "
        "FROM substations "
        "WHERE area_id = :areaId "
        "ORDER BY name;");

    query.bindValue(":areaId", areaId);

    if (!query.exec())
    {
        qDebug() << "loadSubstations error: " << query.lastError().text();

        return;
    }
    // Вставляем результат запроса
    m_rightModel->setQuery(std::move(query));
    // Меняем название столбца
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");
    // Выводим данные в модель
    ui->rightTableView->setModel(m_rightModel);
    // Прячем столбец с ID
    ui->rightTableView->hideColumn(0);
    // Растягиваем последний столбец согласно ширине содержимого
    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);
}

void DirectoriesWidget::clearSubstations()
{
    // Сразу выполняем и передаем запрос в модель
    // таким запросом "очищаем" таблицу
    m_rightModel->setQuery(
        "SELECT id, name "
        "FROM substations "
        "WHERE 0;",
        m_database.getDatabase());
    // меняем заголовок столбца
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");
    // скрываем столбец id
    ui->rightTableView->hideColumn(0);
    // Растягиваем последний столбец согласно его содержимого
    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);

}

void DirectoriesWidget::showAreasLevel()
{
    m_level = DirectoryLevel::Areas;
    m_currentAreId = -1;

    ui->backButton->setVisible(false);

    ui->leftTitleLabel->setText("Участки");
    ui->rightTitleLabel->setText("Подстанции");

    loadAreas();
    clearSubstations();
}

void DirectoriesWidget::showSubstationsLevel(int areaId)
{
    m_level = DirectoryLevel::Substations;
    m_currentAreId = areaId;

    ui->backButton->setVisible(true);

    ui->leftTitleLabel->setText("Подстанции");
    ui->rightTitleLabel->setText("Присоединения");

    // Загружаем ПС теперь в левую таблицу
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name "
        "FROM substations "
        "WHERE area_id = :areaId "
        "ORDER BY name;");

    query.bindValue(":areaId", areaId);

    if (!query.exec())
    {
        qDebug() << "showSubstationsLevel error: " << query.lastError().text();

        return;
    }

    m_leftModel->setQuery(std::move(query));

    m_leftModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");

    ui->leftTableView->hideColumn(0);

    clearConnections();
}



void DirectoriesWidget::loadConnections(int substationId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name, voltage_kv, ct_ratio "
        "FROM connections "
        "WHERE substation_id = :substationId "
        "ORDER BY name;");

    query.bindValue(":substationId", substationId);

    if (!query.exec())
    {
        qDebug() << "loadConnections error: " << query.lastError().text();

        return;
    }

    m_rightModel->setQuery(std::move(query));

    setupConnectionsTable();
}

void DirectoriesWidget::clearConnections()
{
    m_rightModel->setQuery(
        "SELECT id, name, voltage_kv, ct_ratio "
        "FROM connections "
        "WHERE 0;",
        m_database.getDatabase());

   setupConnectionsTable();
}

void DirectoriesWidget::setupConnectionsTable()
{
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Присоединение");
    m_rightModel->setHeaderData(2, Qt::Horizontal, "Напряжение, кВ");
    m_rightModel->setHeaderData(3, Qt::Horizontal, "Ктт");

    ui->rightTableView->hideColumn(0);

    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);
}
















