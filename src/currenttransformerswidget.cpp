
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QHeaderView>
#include <QDebug>
#include <QMessageBox>

#include "currenttransformerswidget.h"
#include "ui_currenttransformerswidget.h"
#include "currenttransformerdialog.h"


CurrentTransformersWidget::CurrentTransformersWidget(
    Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CurrentTransformersWidget), m_database(database)
{
    ui->setupUi(this);

    m_currentTransformerModel = new QSqlQueryModel(this);

    m_historyModel = new QSqlQueryModel(this);

    ui->ctTableView->setModel(m_currentTransformerModel);

    ui->ctHistoryTableView->setModel(m_historyModel);


    setupHistoryTable();

    loadCurrentTransformers();
    clearHistory();
    updateButtons();

    setupCurrentTransformersTable();

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this,
        [this](const QString &text)
        {
            loadCurrentTransformers(text);
        });

    // Подключаем кнопку "Добавить"
    connect(ui->addButton, &QPushButton::clicked, this,
        [this]()
        {
            CurrentTransformerDialog dialog(m_database, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                reloadCurrentTransformers();
            }
        });

    // Подключаем кнопку "Изменить"
    connect(ui->editButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex currentIndex =
                ui->ctTableView->currentIndex();

            if (!currentIndex.isValid())
                return;

            const int row = currentIndex.row();

            const int currentTransformerId =
                m_currentTransformerModel->index(row, 0).data().toInt();

            CurrentTransformerDialog dialog(
                m_database, currentTransformerId, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                reloadCurrentTransformers();
            }
        });

    // Подключаем кнопку удаления
    connect(ui->deleteButton, &QPushButton::clicked,
        this, &CurrentTransformersWidget::deleteCurrentTransformer);

    // Подключаем сигнал при смене выбора в таблице
    connect(
        ui->ctTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this,
        [this] ()
        {
            updateButtons();

            const QModelIndex currentIndex =
                ui->ctTableView->currentIndex();

            if (!currentIndex.isValid())
            {
                clearHistory();
                return;
            }

            const int currentTransformerId =
                m_currentTransformerModel->index(currentIndex.row(), 0).data().toInt();

            loadHistory(currentTransformerId);
        });

    // Подключаем двойной клик по строке таблицы истории ТТ
    connect(ui->ctHistoryTableView, &QTableView::doubleClicked, this,
        [this](const QModelIndex &)
        {
            openSelectedHistoryAct();
        });
}

CurrentTransformersWidget::~CurrentTransformersWidget()
{
    delete ui;
}

void CurrentTransformersWidget::reloadCurrentTransformers()
{
    int currentTransformerId = -1;

    const QModelIndex currentIndex =
        ui->ctTableView->currentIndex();

    if (currentIndex.isValid())
    {
        currentTransformerId =
            m_currentTransformerModel->index(currentIndex.row(), 0).data().toInt();
    }

    loadCurrentTransformers(ui->searchLineEdit->text());

    if (currentTransformerId <= 0)
        return;

    for (int row = 0; row < m_currentTransformerModel->rowCount(); ++row)
    {
        const int id =
            m_currentTransformerModel->index(row, 0).data().toInt();

        if (id == currentTransformerId)
        {
            ui->ctTableView->selectRow(row);
            break;
        }
    }
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
        "OR serial_number LIKE :search "
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

    QHeaderView* header =
        ui->ctTableView->horizontalHeader();

    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::Stretch);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
}

void CurrentTransformersWidget::loadHistory(int currentTransformerId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
        "a.id, "
        "a.act_date, "
        "at.name AS act_type_name, "
        "ar.name AS area_name, "
        "s.name AS substation_name, "
        "c.name AS connection_name, "
        "CASE act_ct.role "
            "WHEN 1 THEN 'Снятый' "
            "WHEN 2 THEN 'Установленный' "
            "ELSE 'Неизвестно' "
        "END AS cr_role, "
        "act_ct.phase "
        "FROM act_current_transformers act_ct "

        "JOIN acts a "
        "ON a.id = act_ct.act_id "

        "JOIN act_types at "
        "ON at.id = a.act_type_id "

        "JOIN connections c "
        "ON c.id = a.connection_id "

        "JOIN substations s "
        "ON s.id = c.substation_id "

        "JOIN areas ar "
        "ON ar.id = s.area_id "

        "WHERE act_ct.current_transformer_id = :currentTransformerId "
        "ORDER BY a.act_date DESC, a.id DESC;");

    query.bindValue(":currentTransformerId", currentTransformerId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось загрузить историю трансформатора тока:\n"
            + query.lastError().text());

        clearHistory();
        return;
    }

    m_historyModel->setQuery(std::move(query));

    m_historyModel->setHeaderData(0, Qt::Horizontal, "ID");
    m_historyModel->setHeaderData(1, Qt::Horizontal, "Дата");
    m_historyModel->setHeaderData(2, Qt::Horizontal, "Тип акта");
    m_historyModel->setHeaderData(3, Qt::Horizontal, "Участок");
    m_historyModel->setHeaderData(4, Qt::Horizontal, "Подстанция");
    m_historyModel->setHeaderData(5, Qt::Horizontal, "Присоединение");
    m_historyModel->setHeaderData(6, Qt::Horizontal, "Роль");
    m_historyModel->setHeaderData(7, Qt::Horizontal, "Фаза");

    ui->ctHistoryTableView->hideColumn(0);

    setupHistoryColumns();
}

void CurrentTransformersWidget::clearHistory()
{
    m_historyModel->setQuery(QSqlQuery());
}

void CurrentTransformersWidget::setupHistoryTable()
{
    ui->ctHistoryTableView->setSelectionBehavior(
       QAbstractItemView::SelectRows);

    ui->ctHistoryTableView->setSelectionMode(
        QAbstractItemView::SingleSelection);

    ui->ctHistoryTableView->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    ui->ctHistoryTableView->verticalHeader()->setVisible(false);
}

void CurrentTransformersWidget::setupHistoryColumns()
{
    QHeaderView* header =
        ui->ctHistoryTableView->horizontalHeader();

    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::Stretch);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents);
}

void CurrentTransformersWidget::updateButtons()
{
    const bool selected =
        ui->ctTableView->currentIndex().isValid();

    ui->editButton->setEnabled(selected);
    ui->deleteButton->setEnabled(selected);
}

void CurrentTransformersWidget::deleteCurrentTransformer()
{
    const QModelIndex currentIndex =
        ui->ctTableView->currentIndex();

    if (!currentIndex.isValid())
        return;

    const int row = currentIndex.row();

    const int currentTransformerId =
        m_currentTransformerModel->index(row, 0).data().toInt();

    const QString name =
        m_currentTransformerModel->index(row, 1).data().toString();

    const QString serialNumber =
        m_currentTransformerModel->index(row, 2).data().toString();

    // Проверяем, использовался ил ТТ в актах
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM act_current_transformers "
        "WHERE current_transformer_id = :id);");

    checkQuery.bindValue(":id", currentTransformerId);

    if (!checkQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить использование трансформатора тока:\n"
            + checkQuery.lastError().text());

        qDebug() << currentTransformerId;

        return;
    }

    if (!checkQuery.next())
        return;

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Удаление не возможно",
            "Трансформатор тока "
            "\"" + name + "\" "
            "с заводским номером " + serialNumber +
            " используется в сохраненных актах.\n\n"
            "Удалить его нельзя.");

        return;
    }

    const auto answer =
        QMessageBox::question(this, "Удаление трансформаторов тока",
            "Удалить трансформатор тока "
            "\"" + name +  "\"\n"
            "заводской номер: " + serialNumber + "?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    QSqlQuery deleteQuery(m_database.getDatabase());

    deleteQuery.prepare(
        "DELETE FROM current_transformers "
        "WHERE id = :id;");

    deleteQuery.bindValue(":id", currentTransformerId);

    if (!deleteQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось удалить трансформатор тока:\n"
            + deleteQuery.lastError().text());

        return;
    }

    reloadCurrentTransformers();
    clearHistory();

    QMessageBox::information(this, "Трансформатор тока удалён",
        "Трансформатор тока успешно удалён.");
}

void CurrentTransformersWidget::openSelectedHistoryAct()
{
    const QModelIndex currentIndex =
        ui->ctHistoryTableView->currentIndex();

    if (!currentIndex.isValid())
        return;

    const int row = currentIndex.row();

    const int actId = m_historyModel->index(row, 0).data().toInt();

    if (actId <= 0)
        return;

    ActViewDialog dialog(m_database, actId, this);

    dialog.exec();
}













