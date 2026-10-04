
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
    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (saveCurrentTransformer())
            {
                accept();
            }
        });

    // сигнал нажатия кнопки "Отмена"
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

CurrentTransformerDialog::CurrentTransformerDialog(
    Database &database, int currentTransformerId, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::CurrentTransformerDialog),
        m_database(database),
        m_editCurrentTransformerId(currentTransformerId)
{
    ui->setupUi(this);

    setWindowTitle("Редактирование трансформатора тока");

    if (!loadCurrentTransformer())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить данные трансформатора тока.");
    }

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (updateCurrentTransformer())
            {
                accept();
            }
        });

    connect(ui->cancelButton, &QPushButton::clicked,
        this, &QDialog::rejected);
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

bool CurrentTransformerDialog::serialNumberExists(
    const QString &serialNumber, int excludeId)
{
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT 1 "
        "FROM current_transformers "
        "WHERE serial_number = :serialNumber "
        "AND id != :excludeId "
        "LIMIT 1;");

    query.bindValue(":serialNumber", serialNumber);

    query.bindValue(":excludeId", excludeId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить заводской номер: "
            + query.lastError().text());

        return false;
    }

    return query.next();
}

bool CurrentTransformerDialog::loadCurrentTransformer()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
        "name, "
        "serial_number, "
        "transformation_ratio, "
        "accuracy_class, "
        "manufacturer, "
        "manufacture_year, "
        "note "
        "FROM current_transformers "
        "WHERE id = :id;");

    query.bindValue(":id", m_editCurrentTransformerId);

    if (!query.exec())
    {
        qDebug() << "Не удалось загрузить трансформатор тока: "
        << query.lastError().text();

        return false;
    }

    if (!query.next())
        return false;

    ui->nameLineEdit->setText(
        query.value("name").toString());
    ui->serialLineEdit->setText(
        query.value("serial_number").toString());
    ui->tranformationRatioLineEdit->setText(
        query.value("transformation_ratio").toString());
    ui->accuracyClassLineEdit->setText(
        query.value("accuracy_class").toString());
    ui->manufacturerLineEdit->setText(
        query.value("manufacturer").toString());

    if (!query.value("manufacture_year").isNull())
    {
        ui->manufactureYearSpinBox->setValue(
            query.value("manufacture_year").toInt());
    }

    ui->notePlainTextEdit->setPlainText(
        query.value("note").toString());

    m_currentTransformerId = m_editCurrentTransformerId;

    return true;
}

bool CurrentTransformerDialog::updateCurrentTransformer()
{
    if (!validateForm())
        return false;

    const QString name =
        ui->nameLineEdit->text().trimmed();

    const QString serialNumber =
        ui->serialLineEdit->text().trimmed();

    const QString transformationRatio =
        ui->tranformationRatioLineEdit->text().trimmed();

    const QString accuracyClass =
        ui->accuracyClassLineEdit->text().trimmed();

    const QString manufacturer =
        ui->manufacturerLineEdit->text().trimmed();

    const int manufactureYear =
        ui->manufactureYearSpinBox->value();

    const QString note =
        ui->notePlainTextEdit->toPlainText().trimmed();

    if (serialNumberExists(serialNumber, m_editCurrentTransformerId))
    {
        QMessageBox::warning(this, "Ошибка заполнения",
            "Трансформатор тока с заводским нмоером "
            + serialNumber +
            " уже существует.");

        ui->serialLineEdit->setFocus();
        ui->serialLineEdit->selectAll();

        return false;
    }

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE current_transformers SET "
        "name = :name, "
        "serial_number = :serialNumber, "
        "transformation_ratio = :transformationRatio, "
        "accuracy_class = :accuracyClass, "
        "manufacturer = :manufacturer, "
        "manufacture_year = :manufactureYear, "
        "note = :note "
        "WHERE id = :id;");

    query.bindValue(":name", name);
    query.bindValue(":serialNumber", serialNumber);
    query.bindValue(":transformationRatio", transformationRatio);
    query.bindValue(":accuracyClass", accuracyClass);
    query.bindValue(":manufacturer", manufacturer);

    if (manufactureYear == 1900)
    {
        query.bindValue(":manufactureYear", QVariant());
    }
    else
    {
        query.bindValue(":manufactureYear", manufactureYear);
    }

    query.bindValue(":note", note);

    query.bindValue(":id", m_editCurrentTransformerId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось изменить трансформаторы тока: "
            + query.lastError().text());

        return false;
    }

    m_currentTransformerId = m_editCurrentTransformerId;

    return true;
}
















