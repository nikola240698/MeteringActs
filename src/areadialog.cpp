
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "areadialog.h"
#include "ui_areadialog.h"
#include "uuidutils.h"


AreaDialog::AreaDialog(Database &database, QWidget *parent)
    : QDialog(parent), ui(new Ui::AreaDialog), m_database(database)
{
    ui->setupUi(this);

    setWindowTitle("Добавление участка");
    ui->titleLabel->setText("Новый участок");

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (save())
                accept();
        });
}

AreaDialog::AreaDialog(Database &database, int areaId, QWidget *parent)
    : AreaDialog(database, parent)
{
    m_areaId = areaId;

    setWindowTitle("Редактирование участка");
    ui->titleLabel->setText("Редактирование участка");

    loadArea();
}

AreaDialog::~AreaDialog()
{
    delete ui;
}

bool AreaDialog::save()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Введите название участка");

        ui->nameLineEdit->setFocus();

        return false;
    }

    if (m_areaId < 0)
        return insertArea();

    return updateArea();
}

bool AreaDialog::insertArea()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO areas (uuid, version_uuid, name) "
        "VALUES (:uuid, :versionUuid, :name);");

    query.bindValue(":uuid", UuidUtils::generate());

    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":name", name);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить участок.\n\n" + query.lastError().text());

        return false;
    }

    return true;
}

bool AreaDialog::updateArea()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    // Проверяем, что изменения были внесены
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT name, version_uuid "
        "FROM areas "
        "WHERE id = :areaId;");

    checkQuery.bindValue(":areaId", m_areaId);

    if (!checkQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить участок:\n "
            + checkQuery.lastError().text());

        return false;
    }

    if (!checkQuery.next())
    {
        QMessageBox::warning(this, "Участок не найден",
            "Участок был удален");

        return false;
    }

    if (checkQuery.value("version_uuid").toString() !=
        m_originalVersionUuid)
    {
        QMessageBox::warning(this, "Конфликт версий",
            "Участок был изменен после открытия окна.\n"
            "Закройте окно и откройте участок заново.");

        return false;
    }

    if (checkQuery.value("name").toString() == name)
    {
        return true;
    }

    // Если изменения есть, то одновляем данные
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE areas "
        "SET name = :name, "
            "version_uuid = :versionUuid "
        "WHERE id = :areaId "
            "AND version_uuid = :originalVersionUuid "
            "AND name IS NOT :name;");

    query.bindValue(":name", name);
    query.bindValue(":versionUuid", UuidUtils::generate());
    query.bindValue(":areaId", m_areaId);
    query.bindValue(":originalVersionUuid", m_originalVersionUuid);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить участок.\n\n" + query.lastError().text());

        return false;
    }

    if (query.numRowsAffected() == 1)
    {
        return true;
    }

    if (query.numRowsAffected() == 0)
    {
        QMessageBox::warning(this, "Конфликт версий",
            "Участок был изменён или удалён другим экземпляром программы.\n\n"
            "Сохранение отменено. Закройте окно и откройте участок заново.");

        return false;
    }

    QMessageBox::critical(this, "Ошибка",
        "Неожиданное количество обновленных записей.");

    return false;
}

void AreaDialog::loadArea()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT name, version_uuid "
        "FROM areas "
        "WHERE id = :areaId;");

    query.bindValue(":areaId", m_areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить данные участка.\n\n" + query.lastError().text());

        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка", "Участок не найден");

        return;
    }

    ui->nameLineEdit->setText(query.value("name").toString());

    m_originalVersionUuid = query.value("version_uuid").toString();

    qDebug() << "Original area version UUID: " << m_originalVersionUuid;
}











