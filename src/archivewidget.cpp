
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QSignalBlocker>

#include "archivewidget.h"
#include "actviewdialog.h"
#include "actrepository.h"
#include "docxgenerator.h"
#include "ui_archivewidget.h"


ArchiveWidget::ArchiveWidget(Database &database, QWidget *parent) :
    QWidget(parent), ui(new Ui::ArchiveWidget), m_database(database)
{
    ui->setupUi(this);

    // Выставляем дату по умолчанию в QDateEdit
    const QDate today = QDate::currentDate();

    ui->dateToEdit->setDate(today);

    ui->dateFromEdit->setDate(
        QDate(today.year(), today.month(), 1));

    loadActTypes();
    loadActs();

    connect(ui->generateDocxButton, &QPushButton::clicked, this,
        &ArchiveWidget::generateSelectedAct);

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this,
        [this]()
        {
            applyFilters();
        });

    connect(ui->actTypeComboBox, &QComboBox::currentIndexChanged, this,
        [this]()
        {
            applyFilters();
        });

    // Подключаем чекбокс выбора фильтра по датам
    connect(ui->dateFilterCheckBox, &QCheckBox::toggled, this,
        [this](bool checked)
        {
            ui->dateFromEdit->setEnabled(checked);
            ui->dateToEdit->setEnabled(checked);
            applyFilters();
        });

    // Подключаем сами даты
    connect(ui->dateFromEdit, &QDateEdit::dateChanged, this,
        [this](const QDate &date)
        {
            ui->dateToEdit->setMinimumDate(date);
            applyFilters();
        });
    connect(ui->dateToEdit, &QDateEdit::dateChanged, this,
        [this](const QDate &date)
        {
            ui->dateFromEdit->setMaximumDate(date);
            applyFilters();
        });

    // Подключаем кнопку сброса
    connect(ui->resetFiltersButton, &QPushButton::clicked, this,
        [this]()
        {
            // создаем блокировку сигналов для всех элементов
            // чтобы не выполнялся их connect и изменении данных
            const QSignalBlocker searchBlocker(
                ui->searchLineEdit);
            const QSignalBlocker typeBlocker(
                ui->actTypeComboBox);
            const QSignalBlocker checkBoxBlocker(
                ui->dateFilterCheckBox);
            const QSignalBlocker dateFromBlocker(
                ui->dateFromEdit);
            const QSignalBlocker dateToBlocker(
                ui->dateToEdit);

            // сбрасываем всё
            ui->searchLineEdit->clear();
            ui->actTypeComboBox->setCurrentIndex(0);
            ui->dateFilterCheckBox->setChecked(false);
            const QDate today = QDate::currentDate();
            ui->dateFromEdit->setDate(
                QDate(today.year(), today.month(), 1));
            ui->dateToEdit->setDate(today);
            applyFilters();
        });

    // Подключаем слот кнопки "Открыть акт"
    connect(ui->openActButton, &QPushButton::clicked, this,
        &ArchiveWidget::openSelectedAct);

    // Подключаем слот двойного нажатия на строку акта
    connect(ui->actsTableView, &QTableView::doubleClicked, this,
        [this](const QModelIndex &)
        {
            openSelectedAct();
        });

}

ArchiveWidget::~ArchiveWidget()
{
    delete ui;
}

void ArchiveWidget::loadActs(
    const QString &searchText,
    int actTypeId,
    bool useDateFilter,
    const QDate &dateFrom,
    const QDate &dateTo)
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

        "WHERE ("
            "s.name LIKE :search "
            "OR c.name LIKE :search "
            "OR a.employee_name LIKE :search "
            "OR at.name LIKE :search "
        ") "

        "AND ("
            ":actTypeId = -1 "
            "OR a.act_type_id = :actTypeId"
        ") "

        "AND ("
            ":userDateFilter = 0 "
            "OR a.act_date BETWEEN :dateFrom AND :dateTo"
        ") "

        "ORDER BY a.act_date DESC, a.id DESC;"
    );

    query.bindValue(":search", "%" + searchText.trimmed() + "%");

    query.bindValue(":actTypeId", actTypeId);

    query.bindValue(
        ":userDateFilter", useDateFilter ? 1 : 0);
    query.bindValue(
        ":dateFrom", dateFrom.toString("yyyy-MM-dd"));
    query.bindValue(
        ":dateTo", dateTo.toString("yyyy-MM-dd"));

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

void ArchiveWidget::loadActTypes()
{
    ui->actTypeComboBox->clear();

    // Первый пункт означает отсутствие фильтра
    ui->actTypeComboBox->addItem("Все типы", -1);

    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, name "
        "FROM act_types "
        "ORDER BY id;"))
    {
        qWarning() << "Не удалось загрузить типы актов: "
            << query.lastError().text();

        return;
    }

    while (query.next())
    {
        const int id = query.value("id").toInt();

        const QString name = query.value("name").toString();

        ui->actTypeComboBox->addItem(name, id);
    }
}

void ArchiveWidget::generateSelectedAct()
{
    const QModelIndexList selectedRows =
        ui->actsTableView->selectionModel()->selectedRows();

    if (selectedRows.isEmpty())
    {
        QMessageBox::warning(this, "Акт не выбран",
            "Выберите акт, для которого необходимо сформировать DOCX.");
        return;
    }

    const int row = selectedRows.first().row();

    const QAbstractItemModel* model = ui->actsTableView->model();

    const int actId = model->index(row, 0).data().toInt();

    qDebug() << "Selected act ID: " << actId;

    // Загружаем полные данные акта
    ActRepository repository(m_database);

    ActData data;

    if (!repository.loadAct(actId, data))
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить данные акта: " + repository.lastError());

        return;
    }

    // Предлагаем имя файла
    QString suggestedFileName =
        QString("Акт_%1_%2.docx")
            .arg(actId)
            .arg(data.date.toString("dd-MM-yyyy"));

    // Выбираем путь сохранения
    const QString outputPath =
        QFileDialog::getSaveFileName(
            this, "Сохранить акт", suggestedFileName, "Документ Word (*.docx)");

    // Если пользователь отменил
    if (outputPath.isEmpty())
    {
        return;
    }

    // Формируем DOCX
    DocxGenerator generator;

    if (!generator.generate(data, outputPath))
    {
        QMessageBox::critical(this, "Ошибка формирования DOCX",
            generator.lastError());

        return;
    }

    QMessageBox::information(this, "Готово",
        "Документ успешно сформирован: " + outputPath);
}

void ArchiveWidget::applyFilters()
{
    const QString searchText =
        ui->searchLineEdit->text();

    const int actTypeId =
        ui->actTypeComboBox->currentData().toInt();

    const bool useDateFilter =
        ui->dateFilterCheckBox->isChecked();

    const QDate dateFrom =
        ui->dateFromEdit->date();

    const QDate dateTo =
        ui->dateToEdit->date();

    loadActs(
        searchText,
        actTypeId,
        useDateFilter,
        dateFrom,
        dateTo);



}

void ArchiveWidget::openSelectedAct()
{
    const QModelIndexList selectedRows =
        ui->actsTableView->selectionModel()->selectedRows();

    if (selectedRows.isEmpty())
    {
        QMessageBox::warning(this, "Акт не выбран", "Выберите акт для просмотра.");

        return;
    }

    const int row = selectedRows.first().row();

    const QAbstractItemModel* model = ui->actsTableView->model();

    const int actId = model->index(row, 0).data().toInt();

    ActViewDialog dialog(m_database, actId, this);

    dialog.exec();

}





















