
#include "meterdialog.h"
#include "ui_MeterDialog.h"


MeterDialog::MeterDialog(Database &database, QWidget *parent)
        : QDialog(parent), ui(new Ui::MeterDialog), m_database(database)
{
    ui->setupUi(this);
    // слот нажатия кнопки Save
    connect(ui->saveButton, &QPushButton::clicked, this, &MeterDialog::saveMeter);
    // слот нажатия кнопки Cancel
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

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
    // проверяем, что они введены
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Input the name of the meter");
        return;
    }
    if (serial.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Input the serial number of the meter");
        return;
    }
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "INSERT INTO meters "
        "(name, serial_number, accuracy_class, verification_year, manufacturer, manufacture_year, note) "
        "VALUES "
        "(:name, :serial, :accuracy, :verificationYear, :manufacturer, :manufactureYear, :note);");
    // биндим значения в запрос
    query.bindValue(":name", name);
    query.bindValue(":serial", serial);
    query.bindValue(":accuracy", ui->accuracyLineEdit->text().trimmed());
    query.bindValue(":verificationYear", ui->verificationYearSpinBox->value());
    query.bindValue(":manufacturer", ui->manufacturerLineEdit->text().trimmed());
    query.bindValue(":manufactureYear", ui->manufactureYearSpinBox->value());
    query.bindValue(":note", ui->notePlainTextEdit->toPlainText().trimmed());
    // проверяем что запрос выполняется
    if (!query.exec())
    {
        QMessageBox::critical(this, "Error", "Failed to save meter: " + query.lastError().text());
        return;
    }
    // получаем id последнего созданного прибора
    m_createdMeterId = query.lastInsertId().toInt();
    // применяем изменения
    accept();
}
















