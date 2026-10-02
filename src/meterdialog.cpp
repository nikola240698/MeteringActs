
#include "meterdialog.h"
#include "ui_meterdialog.h"


MeterDialog::MeterDialog(Database &database, const QString &serial, QWidget *parent)
        : QDialog(parent), ui(new Ui::MeterDialog), m_database(database)
{
    ui->setupUi(this);
    // слот нажатия кнопки Save
    connect(ui->saveButton, &QPushButton::clicked, this, &MeterDialog::saveMeter);
    // слот нажатия кнопки Cancel
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    ui->serialLineEdit->setText(serial);

}

MeterDialog::MeterDialog(Database &database, int meterId, QWidget *parent)
    : QDialog(parent), ui(new Ui::MeterDialog), m_database(database), m_meterId(meterId)
{
    ui->setupUi(this);

    setWindowTitle("Редактирование прибора учёта");

    connect(ui->saveButton, &QPushButton::clicked,
        this, &MeterDialog::saveMeter);

    connect(ui->cancelButton, &QPushButton::clicked,
        this, &QDialog::reject);

    loadMeter();
}

MeterDialog::~MeterDialog()
{
    delete ui;
}

// метод получения id созданного прибора
int MeterDialog::createdMeterId() const
{
    return m_createdMeterId;
}

void MeterDialog::loadMeter()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
            "name, "
            "serial_number, "
            "accuracy_class, "
            "verification_year, "
            "manufacturer, "
            "manufacture_year, "
            "note "
        "FROM meters "
        "WHERE id = :meterId;");

    query.bindValue(":meterId", m_meterId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось загрузить данные прибора учёта:\n"
            + query.lastError().text());

        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Прибор учёта не найден.");

        return;
    }

    ui->nameLineEdit->setText(
        query.value("name").toString());
    ui->serialLineEdit->setText(
        query.value("serial_number").toString());
    ui->accuracyLineEdit->setText(
        query.value("accuracy_class").toString());
    if (!query.value("verification_year").isNull())
    {
        ui->verificationYearSpinBox->setValue(
            query.value("verification_year").toInt());
    }

    ui->manufacturerLineEdit->setText(
        query.value("manufacturer").toString());

    if (!query.value("manufacture_year").isNull())
    {
        ui->manufactureYearSpinBox->setValue(
            query.value("manufacture_year").toInt());
    }

    ui->notePlainTextEdit->setPlainText(
        query.value("note").toString());



}

// метод сохранения прибора
void MeterDialog::saveMeter()
{


    if (m_meterId < 0)
    {
        if (!insertMeter())
            return;
    }
    else
    {
        if (!updateMeter())
            return;
    }

    // применяем изменения
    accept();
}

bool MeterDialog::insertMeter()
{
    // получаем название и серийный номер прибора
    QString name = ui->nameLineEdit->text().trimmed();
    QString serial = ui->serialLineEdit->text().trimmed();
    if (serialNumberExists(serial))
    {
        QMessageBox::warning(this, "Ошибка заполнения",
            "Прибор учета с заводским номером "
            + serial + " уже существует.");
        ui->serialLineEdit->setFocus();
        ui->serialLineEdit->selectAll();
        return false;
    }
    // проверяем, что они введены
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите название прибора учета");
        ui->nameLineEdit->setFocus();
        return false;
    }
    if (serial.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите заводской номер прибора учета");
        ui->serialLineEdit->setFocus();
        return false;
    }
    if (ui->verificationYearSpinBox->value() == 1900)
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите год поверки прибора учета.");
        ui->verificationYearSpinBox->setFocus();
        return false;
    }
    if (ui->accuracyLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите класс точности прибора учета");
        ui->accuracyLineEdit->setFocus();
        return false;
    }

    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "INSERT INTO meters "
        "("
        "name, "
        "serial_number, "
        "accuracy_class, "
        "verification_year, "
        "manufacturer, "
        "manufacture_year, "
        "note"
        ") "
        "VALUES "
        "("
        ":name, "
        ":serial, "
        ":accuracy, "
        ":verificationYear, "
        ":manufacturer, "
        ":manufactureYear, "
        ":note"
        ");");
    // биндим значения в запрос
    query.bindValue(":name", name);
    query.bindValue(":serial", serial);
    query.bindValue(":accuracy", ui->accuracyLineEdit->text().trimmed());
    query.bindValue(":verificationYear", ui->verificationYearSpinBox->value());
    query.bindValue(":manufacturer", ui->manufacturerLineEdit->text().trimmed());
    if (ui->manufactureYearSpinBox->value() == 1900)
    {
        query.bindValue(":manufactureYear", QVariant());
    } else
    {
        query.bindValue(":manufactureYear", ui->manufactureYearSpinBox->value());
    }
    query.bindValue(":note", ui->notePlainTextEdit->toPlainText().trimmed());
    // проверяем что запрос выполняется
    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось записать прибор учета в базу данных: "
            + query.lastError().text());
        return false;
    }
    // получаем id последнего созданного прибора
    m_createdMeterId = query.lastInsertId().toInt();

    return true;
}

bool MeterDialog::updateMeter()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE meters SET "
            "name = :name, "
            "serial_number = :serial, "
            "accuracy_class = :accuracy, "
            "verification_year = :verificationYear, "
            "manufacturer = :manufacturer, "
            "manufacture_year = :manufactureYear, "
            "note = :note "
        "WHERE id = :meterId;");

    query.bindValue(":name", ui->nameLineEdit->text().trimmed());
    query.bindValue(":serial", ui->serialLineEdit->text().trimmed());
    query.bindValue(":accuracy", ui->accuracyLineEdit->text().trimmed());
    query.bindValue(":verificationYear", ui->verificationYearSpinBox->value());
    query.bindValue(":manufacturer", ui->manufacturerLineEdit->text().trimmed());
    if (ui->manufactureYearSpinBox->value() == 1900)
    {
        query.bindValue(":manufactureYear", QVariant());
    }
    else
    {
        query.bindValue(
            ":manufactureYear", ui->manufactureYearSpinBox->value());
    }

    query.bindValue(":note", ui->notePlainTextEdit->toPlainText().trimmed());

    query.bindValue(":meterId", m_meterId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось изменить прибор учёта:\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

bool MeterDialog::serialNumberExists(const QString &serialNumber)
{
    QSqlQuery query(m_database.getDatabase());

    if (m_meterId < 0)
    {
        query.prepare(
            "SELECT 1 "
            "FROM meters "
            "WHERE serial_number = :serialNumber "
            "LIMIT 1;");
    }
    else
    {
        query.prepare(
            "SELECT 1 "
            "FROM meters "
            "WHERE serial_number = :serialNumber "
            "AND id <> :meterId "
            "LIMIT 1;");

        query.bindValue(":meterId", m_meterId);
    }

    query.bindValue(":serialNumber", serialNumber);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить заводской номер: "
            + query.lastError().text());

        return false;
    }

    return query.next();
}
















