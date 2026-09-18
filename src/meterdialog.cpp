
#include "meterdialog.h"
#include "ui_MeterDialog.h"


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

MeterDialog::~MeterDialog()
{
    delete ui;
}

// метод получения id созданного прибора
int MeterDialog::createdMeterId() const
{
    return m_createdMeterId;
}

// метод сохранения прибора
void MeterDialog::saveMeter()
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
        return;
    }
    // проверяем, что они введены
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите название прибора учета");
        ui->nameLineEdit->setFocus();
        return;
    }
    if (serial.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите заводской номер прибора учета");
        ui->serialLineEdit->setFocus();
        return;
    }
    if (ui->verificationYearSpinBox->value() == 1900)
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите год поверки прибора учета.");
        ui->verificationYearSpinBox->setFocus();
        return;
    }
    if (ui->accuracyLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите класс точности прибора учета");
        ui->accuracyLineEdit->setFocus();
        return;
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
        query.bindValue(":manufacturerYear", QVariant());
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
        return;
    }
    // получаем id последнего созданного прибора
    m_createdMeterId = query.lastInsertId().toInt();
    // применяем изменения
    accept();
}

bool MeterDialog::serialNumberExists(const QString &serialNumber)
{
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT 1 "
        "FROM meters "
        "WHERE serial_number = :serialNumber "
        "LIMIT 1;");

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
















