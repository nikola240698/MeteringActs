
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "employeedialog.h"
#include "ui_employeedialog.h"
#include "database.h"
#include "uuidutils.h"


EmployeeDialog::EmployeeDialog(Database &database, QWidget *parent)
    : QDialog(parent), ui(new Ui::EmployeeDialog), m_database(database)
{
    ui->setupUi(this);

    m_editMode = false;
    m_employeeId = -1;

    ui->titleLabel->setText("Новый сотрудник");

    setupConnections();
}

EmployeeDialog::EmployeeDialog(Database &database, int employeeId, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EmployeeDialog),
    m_database(database),
    m_employeeId(employeeId),
    m_editMode(true)
{
    ui->setupUi(this);

    ui->titleLabel->setText("редактирвоание сотрудника");

    setupConnections();
    loadEmployee();
}

EmployeeDialog::~EmployeeDialog()
{
    delete ui;
}

void EmployeeDialog::saveEmployee()
{
    const QString shorName =
        ui->shortNameLineEdit->text().trimmed();

    const QString position =
        ui->positionLineEdit->text().trimmed();

    if (shorName.isEmpty())
    {
        QMessageBox::warning(this, "Проверка данных",
            "Введите ФИО сотрудника.");

        ui->shortNameLineEdit->setFocus();
        return;
    }

    if (position.isEmpty())
    {
        QMessageBox::warning(this, "Проверка данных",
            "Введите должность сотрудника.");

        ui->positionLineEdit->setFocus();
        return;
    }

    bool success = false;

    if (m_editMode)
        success = updateEmployee();
    else
        success = insertEmployee();

    if (success)
        accept();
}

// Метод подключения кнопок
void EmployeeDialog::setupConnections()
{
    connect(ui->cancelButton, &QPushButton::clicked, this,
        &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        &EmployeeDialog::saveEmployee);
}

void EmployeeDialog::loadEmployee()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT short_name, position, version_uuid "
        "FROM employees "
        "WHERE id = :id;");

    query.bindValue(":id", m_employeeId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить сотрудника:\n" +
            query.lastError().text());

        reject();
        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Сотрудник не найден.");

        reject();
        return;
    }

    ui->shortNameLineEdit->setText(
        query.value("short_name").toString());

    ui->positionLineEdit->setText(
        query.value("position").toString());

    m_originalVersionUuid = query.value("version_uuid").toString();

    qDebug() << "Original employee version UUID: " << m_originalVersionUuid;
}

bool EmployeeDialog::insertEmployee()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO employees "
        "(uuid, version_uuid, short_name, position) "
        "VALUES (:uuid, :versionUuid, :shortName, :position);");

    query.bindValue(":uuid", UuidUtils::generate());

    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":shortName",
        ui->shortNameLineEdit->text().trimmed());

    query.bindValue(":position",
        ui->positionLineEdit->text().trimmed());

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить сотрудника:\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

bool EmployeeDialog::updateEmployee()
{
    // Проверяем что изменения были внесены
    const QString shortName =
        ui->shortNameLineEdit->text().trimmed();

    const QString position =
        ui->positionLineEdit->text().trimmed();

    QSqlQuery currentQuery(m_database.getDatabase());

    currentQuery.prepare(
        "SELECT short_name, position, version_uuid "
        "FROM employees "
        "WHERE id = :id;");

    currentQuery.bindValue(":id", m_employeeId);

    if (!currentQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить сотрудника.\n\n"
            + currentQuery.lastError().text());

        return false;
    }

    if (!currentQuery.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Сотрудник больше не существует.");

        return false;
    }

    if (currentQuery.value("version_uuid").toString() !=
        m_originalVersionUuid)
    {
        QMessageBox::warning(this, "Конфликт изменений",
            "Данные сотрудника были изменены после открытия окна.\n"
            "Ваши изменения не сохранены.");

        return false;
    }

    // Если данные не изменились, обновлять запись не нужно
    if (currentQuery.value("short_name").toString() == shortName &&
        currentQuery.value("position").toString() == position)
    {
        return true;
    }

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE employees "
        "SET short_name = :shortName, "
            "position = :position, "
            "version_uuid = :versionUuid "
        "WHERE id = :id "
            "AND version_uuid = :originalVersionUuid "
            "AND ("
                "short_name IS NOT :shortName "
                "OR position IS NOT :position);");

    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":shortName",
        ui->shortNameLineEdit->text().trimmed());

    query.bindValue(":position",
        ui->positionLineEdit->text().trimmed());

    query.bindValue(":id", m_employeeId);

    query.bindValue(":originalVersionUuid", m_originalVersionUuid);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить сотрудника:\n" +
            query.lastError().text());

        return false;
    }

    // Проверяем успешность
    if (query.numRowsAffected() != 1)
    {
        QMessageBox::warning(this, "Конфликт изменений",
            "Не удалось сохранить изменение сотрудника.\n\n"
            "Возможно, запись была изменена или удалена "
            "другим пользователем.");

        return false;
    }

    return true;
}












