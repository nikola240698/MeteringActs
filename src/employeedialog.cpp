
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "employeedialog.h"
#include "ui_employeedialog.h"
#include "database.h"


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
        "SELECT short_name, position "
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
}

bool EmployeeDialog::insertEmployee()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO employees "
        "(short_name, position) "
        "VALUES (:shortName, :position);");

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
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE employees "
        "SET short_name = :shortName, "
        "position = :position "
        "WHERE id = :id;");

    query.bindValue(":shortName",
        ui->shortNameLineEdit->text().trimmed());

    query.bindValue(":position",
        ui->positionLineEdit->text().trimmed());

    query.bindValue(":id", m_employeeId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить сотрудника:\n" +
            query.lastError().text());

        return false;
    }

    return true;
}












