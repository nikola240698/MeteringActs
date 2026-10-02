
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QHeaderView>
#include <QDebug>

#include "currenttransformerswidget.h"
#include "ui_currenttransformerswidget.h"


CurrentTransformersWidget::CurrentTransformersWidget(
    Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CurrentTransformersWidget), m_database(database)
{
    ui->setupUi(this);

    m_currentTransformerModel = new QSqlQueryModel(this);

    m_historyModel = new QSqlQueryModel(this);

    ui->ctTableView->setModel(m_currentTransformerModel);

    ui->ctHistoryTableView->setModel(m_historyModel);

    setupCurrentTransformersTable();
    setupHistoryTable();

    loadCurrentTransformers();

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this,
        [this](const QString &text)
        {
            loadCurrentTransformers(text);
        });
}

CurrentTransformersWidget::~CurrentTransformersWidget()
{
    delete ui;
}

void CurrentTransformersWidget::reloadCurrentTransformers()
{
    loadCurrentTransformers(ui->searchLineEdit->text());
}

void CurrentTransformersWidget::loadCurrentTransformers(const QString &searchText)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
        "id, "
        "name, "
        "serial_number, "
        "transformation_ratio, "
        "accuracy_class, "
        "manufacturer, "
        "manufacture_year "
        "FROM current_transformers "
        "WHERE name LIKE :search "
        "OR transformation_ratio LIKE :search "
        "ORDER BY name, serial_number;");

    query.bindValue(":search", "%" + searchText.trimmed() + "%");

    if (!query.exec())
    {
        qDebug() << "Не удалось загрузить трансформаторы тока: "
            << query.lastError().text();

        return;
    }

    m_currentTransformerModel->setQuery(std::move(query));

    m_currentTransformerModel->setHeaderData(
        0, Qt::Horizontal, "ID");
    m_currentTransformerModel->setHeaderData(
        1, Qt::Horizontal, "Наименование");
    m_currentTransformerModel->setHeaderData(
        2, Qt::Horizontal, "Заводской номер");
    m_currentTransformerModel->setHeaderData(
        3, Qt::Horizontal, "Ктт");
    m_currentTransformerModel->setHeaderData(
        4, Qt::Horizontal, "Класс точности");
    m_currentTransformerModel->setHeaderData(
        5, Qt::Horizontal, "Производитель");
    m_currentTransformerModel->setHeaderData(
        6, Qt::Horizontal, "Год выпуска");

    ui->ctTableView->hideColumn(0);

    ui->ctTableView->resizeColumnsToContents();

    updateButtons();
}

void CurrentTransformersWidget::setupCurrentTransformersTable()
{
    ui->ctTableView->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    ui->ctTableView->setSelectionMode(
        QAbstractItemView::SingleSelection);

    ui->ctTableView->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    ui->ctTableView->verticalHeader()->setVisible(false);

    ui->ctTableView->horizontalHeader()->setStretchLastSection(true);
}

void CurrentTransformersWidget::loadHistory(int currentTransformerId)
{
}

void CurrentTransformersWidget::clearHistory()
{
    m_historyModel->setQuery(QSqlQuery());
}

void CurrentTransformersWidget::setupHistoryTable()
{
    ui->ctTableView->setSelectionBehavior(
       QAbstractItemView::SelectRows);

    ui->ctTableView->setSelectionMode(
        QAbstractItemView::SingleSelection);

    ui->ctTableView->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    ui->ctTableView->verticalHeader()->setVisible(false);

    ui->ctTableView->horizontalHeader()->setStretchLastSection(true);
}

void CurrentTransformersWidget::updateButtons()
{
    const bool selected =
        ui->ctTableView->currentIndex().isValid();

    ui->editButton->setEnabled(selected);
    ui->deleteButton->setEnabled(selected);
}
