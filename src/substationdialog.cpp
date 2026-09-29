#include "substationdialog.h"

#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

#include "substationdialog.h"

#include <QPushButton>

#include "ui_substationdialog.h"


SubstationDialog::SubstationDialog(
    Database &database, int areaId, QWidget *parent)
        : QDialog(parent), ui(new Ui::SubstationDialog), m_database(database), m_areaId(areaId)
{
    ui->setupUi(this);

    setWindowTitle("Добавление подстанции");
    ui->titleLabel->setText("Новая подстанция");

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (save())
                accept();
        });

}


SubstationDialog::SubstationDialog(
    Database &database, int areaId, int substationId, QWidget *parent)
        : SubstationDialog(database, areaId, parent)
{
    m_substationId = substationId;

    setWindowTitle("Редактирование подстанции");
    ui->titleLabel->setText("Редактирование подстанции");

    loadSubstation();
}

SubstationDialog::~SubstationDialog()
{
    delete ui;
}

bool SubstationDialog::save()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка",
            "Введите название подстанции.");

        ui->nameLineEdit->setFocus();

        return false;
    }

    if (m_substationId < 0)
        return insertSubstation();

    return updateSubstation();
}

bool SubstationDialog::insertSubstation()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO substations (area_id, name) "
        "VALUES (:areaId, :name);");

    query.bindValue(":areaId", m_areaId);
    query.bindValue(":name", name);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить подстанцию.\n\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

bool SubstationDialog::updateSubstation()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE substations "
        "SET name = :name "
        "WHERE id = :substationId "
        "AND area_id = :areaId");

    query.bindValue(":name", name);
    query.bindValue(":substationId", m_substationId);
    query.bindValue(":areaId", m_areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить подстанцию.\n\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

void SubstationDialog::loadSubstation()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT name "
        "FROM substations "
        "WHERE id = :substationId "
        "AND area_id = :areaId;");

    query.bindValue(":substationId", m_substationId);
    query.bindValue(":areaId", m_areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить данные подстанции.\n\n" +
            query.lastError().text());

        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Подстанция не найдена.");

        return;
    }

    ui->nameLineEdit->setText(
        query.value("name").toString());
}

























