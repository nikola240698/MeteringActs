
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


MetersWidget::MetersWidget(Database &database, QWidget *parent)
    : QWidget(parent), ui(new Ui::MetersWidget), m_database(database)
{
    ui->setupUi(this);

    m_metersModel = new QSqlQueryModel(this);

    ui->metersTableView->setModel(m_metersModel);

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
}

MetersWidget::~MetersWidget()
{
    delete ui;
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










