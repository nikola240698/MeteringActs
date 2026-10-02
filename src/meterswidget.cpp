
#include <QSqlQueryModel>
#include <QSqLQuery>
#include <QsqlError>
#include <QDebug>
#include <QHeaderView>
#include <QItemSelectionModel>
#include  <QMessageBox>

#include "meterswidget.h"
#include "ui_meterswidget.h"
#include "meterdialog.h"
#include "actviewdialog.h"


MetersWidget::MetersWidget(Database &database, QWidget *parent)
    : QWidget(parent), ui(new Ui::MetersWidget), m_database(database)
{
    ui->setupUi(this);

    // Создаем модель основной таблицы
    m_metersModel = new QSqlQueryModel(this);
    ui->metersTableView->setModel(m_metersModel);

    // Создаем модель таблицы истории
    m_historyModel = new QSqlQueryModel(this);
    ui->historytableView->setModel(m_historyModel);

    // Добавляем кнопку очистки для поля поиска
    ui->searchLineEdit->setClearButtonEnabled(true);

    loadMeters();

    // Подключаем строку для поиска
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this,
        [this](const QString &text)
        {
            loadMeters(text);
        });

    // Подключаем слот при изменении выбора строки для активности кнопок
    connect(ui->metersTableView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this]()
        {
            updateButtons();

            const QModelIndex currentIndex =
                ui->metersTableView->currentIndex();

            if (!currentIndex.isValid())
            {
                clearMeterHistory();
                return;
            }

            const int row = currentIndex.row();

            const int meterId =
                m_metersModel->index(row, 0).data().toInt();

            loadMeterHistory(meterId);
        });

    // Подключаем кнопку добавления нового прибора
    connect(ui->addMeterButton, &QPushButton::clicked, this,
        [this]()
        {
            MeterDialog dialog(m_database, "", this);

            if (dialog.exec() == QDialog::Accepted)
            {
                loadMeters(ui->searchLineEdit->text());
            }
        });

    // Подключаем кнопку редактирования
    connect(ui->editMeterButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex currentIndex =
                ui->metersTableView->currentIndex();

            if (!currentIndex.isValid())
                return;

            const int row = currentIndex.row();

            const int meterId =
                m_metersModel->index(row, 0).data().toInt();

            MeterDialog dialog(m_database, meterId, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                loadMeters(ui->searchLineEdit->text());
            }
        });

    // Подключаем кнопку удаления
    connect(ui->deleteMeterButton, &QPushButton::clicked, this,
        [this]()
        {
            deleteSelectedMeter();
        });

    updateButtons();

    // Подключаем слот двойного нажатия на строку в истории акта
    connect(ui->historytableView, &QTableView::doubleClicked, this,
        [this](const QModelIndex &)
        {
            openSelectedHistoryAct();
        });
}

MetersWidget::~MetersWidget()
{
    delete ui;
}

void MetersWidget::reloadMeters()
{
    // Запоминаем выбранную строку
    // Создаем переменную для id прибор
    int selectedMeterId = -1;

    // Получаем индекс выбранной строки
    const QModelIndex currentIndex =
        ui->metersTableView->currentIndex();

    // Проверяем, что строка выбрана
    if (currentIndex.isValid())
    {
        // получаем id прибора
        selectedMeterId =
            m_metersModel->index(currentIndex.row(), 0).data().toInt();
    }

    // Перезагружаем данные модели
    loadMeters(ui->searchLineEdit->text());

    // Проверяем, что выбор был до перезагрузки
    if (selectedMeterId < 0)
        return;

    // Прочитываем каждую строку в поиске нужного id
    for (int row = 0; row < m_metersModel->rowCount(); ++row)
    {
        // Получаем id прибора на текущей строке
        const int meterId =
            m_metersModel->index(row, 0).data().toInt();

        // Сравниваем с записанным
        if (meterId != selectedMeterId)
            continue;

        // Получаем индекс найденной строки
        const QModelIndex index =
            m_metersModel->index(row, 1);

        // Ставим выбор на индекс найденной строки
        ui->metersTableView->setCurrentIndex(index);
        ui->metersTableView->selectRow(row);

        break;
    }
}

void MetersWidget::loadMeters(const QString &searchText)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
            "id, "
            "name, "
            "serial_number, "
            "accuracy_class, "
            "verification_year, "
            "manufacturer, "
            "manufacture_year "
        "FROM meters "
        "WHERE "
            "name LIKE :search "
            "OR serial_number LIKE :search "
        "ORDER BY name, serial_number;");

    query.bindValue(":search", "%" + searchText.trimmed() + "%");

    if (!query.exec())
    {
        qDebug() << "loadMeters error:" << query.lastError().text();

        return;
    }

    m_metersModel->setQuery(std::move(query));

    m_metersModel->setHeaderData(1, Qt::Horizontal, "Наименование");
    m_metersModel->setHeaderData(2, Qt::Horizontal, "Заводской №");
    m_metersModel->setHeaderData(3, Qt::Horizontal, "Класс точности");
    m_metersModel->setHeaderData(4, Qt::Horizontal, "Год поверки");
    m_metersModel->setHeaderData(5, Qt::Horizontal, "Производитель");
    m_metersModel->setHeaderData(6, Qt::Horizontal, "Год выпуска");

    ui->metersTableView->hideColumn(0);

    setupMetersTable();

    ui->metersTableView->clearSelection();
    ui->metersTableView->setCurrentIndex(QModelIndex());

    updateButtons();
    clearMeterHistory();
}



void MetersWidget::setupMetersTable()
{
    QHeaderView *header =
        ui->metersTableView->horizontalHeader();

    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);

}

void MetersWidget::updateButtons()
{
    const bool hasSelection =
        ui->metersTableView->selectionModel()->hasSelection();

    ui->editMeterButton->setEnabled(hasSelection);
    ui->deleteMeterButton->setEnabled(hasSelection);
}

void MetersWidget::deleteSelectedMeter()
{
    const QModelIndex currentIndex =
        ui->metersTableView->currentIndex();

    if (!currentIndex.isValid())
        return;

    const int row = currentIndex.row();

    const int meterId =
        m_metersModel->index(row, 0).data().toInt();

    const QString meterName =
        m_metersModel->index(row, 1).data().toString();

    const QString serialNumber =
        m_metersModel->index(row, 2).data().toString();

    // Проверяем, использовался ли прибор в актах
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM act_meters "
            "WHERE meter_id = :meterId);");

    checkQuery.bindValue(":meterId", meterId);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить использование прибора:\n" +
            checkQuery.lastError().text());

        return;
    }

    const bool isUsed = checkQuery.value(0).toBool();

    if (isUsed)
    {
        QMessageBox::warning(this, "Удаление не возможно",
            "Невохможно удалить прибор учёта\n\n"
            + meterName
            + "\nЗаводской № "
            + serialNumber
            + "\n\nПрибор используется в сохраненных актах.");

        return;
    }

    // Если нигде не используется - то удаляем с подтверждением
    const auto answer = QMessageBox::question(this, "Удаление прибора",
        "Удалить прибор учета?\n\n"
        + meterName
        + "\nЗаводской № "
        + serialNumber
        + "\n\nЭто действие нельзя отменить.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    // Удаляем прибор
    QSqlQuery deleteQuery(m_database.getDatabase());

    deleteQuery.prepare(
        "DELETE FROM meters "
        "WHERE id = :meterId;");

    deleteQuery.bindValue(":meterId", meterId);

    if (!deleteQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось удалить прибор учёта:\n" +
            deleteQuery.lastError().text());

        return;
    }

    // Обновляем таблицу с сохранением текущего поиска
    loadMeters(ui->searchLineEdit->text());
}

void MetersWidget::loadMeterHistory(int meterId)
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
            "CASE am.role "
                "WHEN 1 THEN 'Проверяемый' "
                "WHEN 2 THEN 'Снятый' "
                "WHEN 3 THEN 'Установленный' "
                "WHEN 4 THEN 'Снятие показаний' "
                "WHEN 5 THEN 'Существующий' "
                "ELSE 'Неизвестно' "
            "END AS meter_role "
        "FROM act_meters am "

        "JOIN acts a "
        "ON a.id = am.act_id "

        "JOIN act_types at "
        "ON at.id = a.act_type_id "

        "JOIN connections c "
        "ON c.id = a.connection_id "

        "JOIN substations s "
        "ON s.id = c.substation_id "

        "JOIN areas ar "
        "ON ar.id = s.area_id "

        "WHERE am.meter_id = :meterId "

        "ORDER BY a.act_date DESC, a.id DESC;");

    query.bindValue(":meterId", meterId);

    if (!query.exec())
    {
        qDebug() << "Не удалось загрузить историю прибора: "
                << query.lastError().text();

        clearMeterHistory();
         return;
    }

    m_historyModel->setQuery(std::move(query));

    m_historyModel->setHeaderData(1, Qt::Horizontal, "Дата");
    m_historyModel->setHeaderData(2, Qt::Horizontal, "Тип акта");
    m_historyModel->setHeaderData(3, Qt::Horizontal, "Участок");
    m_historyModel->setHeaderData(4, Qt::Horizontal, "Подстанция");
    m_historyModel->setHeaderData(5, Qt::Horizontal, "Присоединение");
    m_historyModel->setHeaderData(6, Qt::Horizontal, "Роль");

    ui->historytableView->hideColumn(0);

    setupHistoryTale();
}

void MetersWidget::clearMeterHistory()
{
    m_historyModel->clear();
}

void MetersWidget::setupHistoryTale()
{
    QHeaderView* header =
        ui->historytableView->horizontalHeader();

    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::Stretch);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
}

void MetersWidget::openSelectedHistoryAct()
{
    const QModelIndex currentIndex =
        ui->historytableView->currentIndex();

    if (!currentIndex.isValid())
        return;

    const int row = currentIndex.row();

    const int actId =
        m_historyModel->index(row, 0).data().toInt();

    if (actId < 0)
        return;

    ActViewDialog dialog(m_database, actId, this);

    dialog.exec();
}


















