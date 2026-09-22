
#include <QMessageBox>

#include "../include/currenttransformerdialog.h"
#include "ui_currenttransformerdialog.h"


CurrentTransformerDialog::CurrentTransformerDialog(
    Database &database, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::CurrentTransformerDialog),
        m_database(database)
{
    ui->setupUi(this);

    setWindowTitle("Добавление трансформатора тока");

    // сигнал нажатия кнопки "Сохранить"
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
        [this]()
        {
            if (saveCurrentTransformer())
            {
                accept();
            }
        });

    // сигнал нажатия кнопки "Отмена"
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CurrentTransformerDialog::~CurrentTransformerDialog()
{
    delete ui;
}

void CurrentTransformerDialog::setSerialNumber(const QString &serialNumber)
{
    // вставляем заводской номер
    ui->serialLineEdit->setText(serialNumber);
    // ставим фокус на первую строку
    ui->nameLineEdit->setFocus();
}

int CurrentTransformerDialog::currentTransformerId() const
{
    return m_currentTransformerId;
}

bool CurrentTransformerDialog::validateForm()
{
    if (ui->nameLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите наименование трансформатора тока.");

        ui->nameLineEdit->setFocus();
        return false;
    }

    if (ui->serialLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите заводской номер трансформатора тока.");

        ui->serialLineEdit->setFocus();
        return false;
    }

    if (ui->tranformationRatioLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите коэффициент трансформации.");
        return false;
    }

    if (ui->accuracyClassLineEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите класс точности трансформатора тока.");
        return false;
    }
    return true;
}

bool CurrentTransformerDialog::saveCurrentTransformer()
{
    if (!validateForm())
        return false;

    const QString name = ui->nameLineEdit->text().trimmed();
    const QString serialNumber = ui->serialLineEdit->text().trimmed();
    if (serialNumberExists(serialNumber))
    {
        QMessageBox::warning(this, "Ошибка заполнения",
            "Трансформатор тока с заводским номером "
            + serialNumber + " уже существует.");
        ui->serialLineEdit->setFocus();
        ui->serialLineEdit->selectAll();
        return false;
    }
    const QString transformationRatio = ui->tranformationRatioLineEdit->text().trimmed();
    const QString accuracyClass = ui->accuracyClassLineEdit->text().trimmed();
    const QString manufacturer = ui->manufacturerLineEdit->text().trimmed();
    const int manufactureYear = ui->manufactureYearSpinBox->value();
    const QString note = ui->notePlainTextEdit->toPlainText().trimmed();

    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "INSERT INTO current_transformers ("
        "name, "
        "serial_number, "
        "transformation_ratio, "
        "accuracy_class, "
        "manufacturer, "
        "manufacture_year, "
        "note"
        ")"
        "VALUES ("
        ":name, "
        ":serialNumber, "
        ":transformationRatio, "
        ":accuracyClass, "
        ":manufacturer, "
        ":manufactureYear, "
        ":note"
        ");");

    query.bindValue(":name", name);
    query.bindValue(":serialNumber", serialNumber);
    query.bindValue(":transformationRatio", transformationRatio);
    query.bindValue(":accuracyClass", accuracyClass);
    query.bindValue(":manufacturer", manufacturer);
    if (manufactureYear == 1900)
    {
        query.bindValue(":manufactureYear", QVariant());
    } else
    {
        query.bindValue(":manufactureYear", manufactureYear);
    }
    query.bindValue(":note", note);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось добавить трансформатор тока: "
            + query.lastError().text());
        return false;
    }

    m_currentTransformerId = query.lastInsertId().toInt();
    return true;
}

bool CurrentTransformerDialog::serialNumberExists(const QString &serialNumber)
{
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT 1 "
        "FROM current_transformers "
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
















