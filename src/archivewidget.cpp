
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QDebug>

#include "archivewidget.h"
#include "ui_ArchiveWidget.h"


ArchiveWidget::ArchiveWidget(Database &database, QWidget *parent) :
    QWidget(parent), ui(new Ui::ArchiveWidget), m_database(database)
{
    ui->setupUi(this);

    loadActs();
}

ArchiveWidget::~ArchiveWidget()
{
    delete ui;
}

void ArchiveWidget::loadActs()
{
    auto* model = new QSqlQueryModel(this);

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
        "a.id, "
        "a.act_date, "
        "at.name AS act_type_name,"
        "s.name AS substation_name, "
        "c.name AS connection_name, "
        "c.voltage_kv, "
        "a.employee_name "
        "FROM acts a "
        "JOIN act_types at "
        "ON at.id = a.act_type_id "
        "JOIN connections c "
        "ON c.id = a.connection_id "
        "JOIN substations s "
        "ON s.id = c.substation_id "
        "ORDER BY a.act_date DESC, a.id DESC;"
    );

    if (!query.exec())
    {
        qWarning() << "Не удалось загрузить архив актов: "
            << query.lastError().text();

        delete model;
        return;
    }

    model->setQuery(std::move(query));

    model->setHeaderData(0, Qt::Horizontal, "ID");
    model->setHeaderData(1, Qt::Horizontal, "Дата");
    model->setHeaderData(2, Qt::Horizontal, "Тип акта");
    model->setHeaderData(3, Qt::Horizontal, "Подстанция");
    model->setHeaderData(4, Qt::Horizontal, "Присоединение");
    model->setHeaderData(5, Qt::Horizontal, "Напряжение, кВ");
    model->setHeaderData(6, Qt::Horizontal, "Представитель");

    ui->actsTableView->setModel(model);

    // Скрываем столбец ID
    ui->actsTableView->hideColumn(0);

    // Выбираем сразу всю строку
    ui->actsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Активный выбор только на одной строке
    ui->actsTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Запрещаем редактирование
    ui->actsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Убираем вертикальные номера строк
    ui->actsTableView->verticalHeader()->setVisible(false);

    // Последний столбец занимает свободное место
    ui->actsTableView->horizontalHeader()->setStretchLastSection(true);

    // Подгоняем остальные столбцы
    ui->actsTableView->resizeColumnsToContents();

}





















