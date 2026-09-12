
#include "createactwidget.h"
#include "ui_CreateActWidget.h"


CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);

    loadActTypes();
    loadAreas();
    loadEmplyees();

}

CreateActWidget::~CreateActWidget()
{
    delete ui;
}

void CreateActWidget::loadActTypes()
{
    ui->actTypeComboBox->clear();

    ui->actTypeComboBox->addItem("Выберите тип акта...", QVariant());

    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, name "
        "FROM act_types "
        "ORDER BY id"))
    {
        qDebug() << "LoadActTypes error: " << query.lastError().text();
        return;
    }

    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();

        ui->actTypeComboBox->addItem(name, id);
    }
}

void CreateActWidget::loadAreas()
{
    ui->areaComboBox->clear();

    ui->areaComboBox->addItem(
        "Выберите участок...", QVariant());

    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, name "
        "FROM areas "
        "ORDER BY name"))
    {
        qDebug() << "loadAreas error: " << query.lastError().text();
        return;
    }

    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();

        ui->areaComboBox->addItem(name, id);
    }
}

void CreateActWidget::loadEmplyees()
{
    ui->employeeComboBox->clear();

    ui->employeeComboBox->addItem(
        "Выберите представителя...", QVariant());

    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, short_name, position "
        "FROM employees "
        "WHERE is_active = 1 "
        "ORDER BY short_name"))
    {
        qDebug() << "loadEmployees error: " << query.lastError().text();
        return;
    }

    while (query.next())
    {
        int id = query.value("id").toInt();

        QString shortName = query.value("short_name").toString();

        QString position = query.value("position").toString();

        QString displayText = shortName + " - " + position;

        ui->employeeComboBox->addItem(displayText, id);
    }
}

