
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "uuidutils.h"


ConnectionDialog::ConnectionDialog(
    Database &database, int substationId, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::ConnectionDialog),
        m_database(database),
        m_substationId(substationId)
{
    ui->setupUi(this);

    setWindowTitle("Добавление присоединения");
    ui->titleLabel->setText("Новое присоединение");

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (save())
                accept();
        });
}

ConnectionDialog::ConnectionDialog(
    Database &database, int substationId, int connectionId, QWidget *parent)
        : ConnectionDialog(database, substationId, parent)
{
    m_connectionId = connectionId;

    setWindowTitle("Редактирование присоединения");
    ui->titleLabel->setText("Редактирование присоединения");

    loadConnection();

}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

bool ConnectionDialog::save()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка",
            "Введите название присоединения.");

        ui->nameLineEdit->setFocus();

        return false;
    }

    if (ui->voltageDoubleSpinBox->value() <= 0.0)
    {
        QMessageBox::warning(this, "Ошибка",
            "Укажите напряжение присоединения.");

        ui->voltageDoubleSpinBox->setFocus();

        return false;
    }

    if (m_connectionId < 0)
        return insertConnection();

    return updateConnection();
}

bool ConnectionDialog::insertConnection()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    // Проверяем на наличие такого же имени в БД
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM connections "
            "WHERE substation_id = :substationId "
            "AND name =:name"
        ");");

    checkQuery.bindValue(":substationId", m_substationId);
    checkQuery.bindValue(":name", name);

    // Проверяем, что вообще что-либо нашли
    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить наличие присоединения.");

        return false;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Присоединение уже существует",
            QString(
                "Присоединение \"%1\" уже существует на этой подстанции.\n\n"
                "Введите другое название.")
                .arg(name));

        ui->nameLineEdit->setFocus();
        ui->nameLineEdit->selectAll();

        return false;
    }

    const double voltage =
        ui->voltageDoubleSpinBox->value();

    const QString ctRatio =
        ui->ctRatioLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO connections "
        "(uuid, version_uuid, substation_id, name, voltage_kv, ct_ratio) "
        "VALUES "
        "(:uuid, :versionUuid, :substationId, :name, :voltage, :ctRatio);");

    query.bindValue(":uuid", UuidUtils::generate());
    query.bindValue(":versionUuid", UuidUtils::generate());
    query.bindValue(":substationId", m_substationId);
    query.bindValue(":name", name);
    query.bindValue(":voltage", voltage);

    if (ctRatio.isEmpty())
    {
        query.bindValue(":ctRatio", QVariant());
    }
    else
    {
        query.bindValue(":ctRatio", ctRatio);
    }

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить присоединение.\n\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

bool ConnectionDialog::updateConnection()
{
    const QString name =
        ui->nameLineEdit->text().trimmed();

    // Проверяем что присоединение существует и не изменился
    const double voltage = ui->voltageDoubleSpinBox->value();
    const QString ctRatio = ui->ctRatioLineEdit->text().trimmed();

    QSqlQuery currentQuery(m_database.getDatabase());

    currentQuery.prepare(
        "SELECT name, voltage_kv, ct_ratio, version_uuid "
        "FROM connections "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId;");

    currentQuery.bindValue(":connectionId", m_connectionId);
    currentQuery.bindValue(":substationId", m_substationId);

    if (!currentQuery.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить присоединение.\n\n"
            + currentQuery.lastError().text());

        return false;
    }

    if (!currentQuery.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Присоединения больше не существует.");

        return false;
    }

    if (currentQuery.value("version_uuid").toString() !=
        m_originalVersionUuid)
    {
        QMessageBox::warning(this, "Конфликт изменений",
            "Присоединение было изменено после открытия окна.\n"
            "Ваши изменения не сохранены.");

        return false;
    }

    if (currentQuery.value("name").toString() == name &&
        currentQuery.value("voltage_kv").toDouble() == voltage &&
        currentQuery.value("ct_ratio").toString() == ctRatio)
    {
        return true;
    }

    // Проверяем, что измененное имя не встречается уже в БД
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM connections "
            "WHERE substation_id = :substationId "
            "AND name = :name "
            "AND id <> :connectionId"
        ");");

    checkQuery.bindValue(":substationId", m_substationId);
    checkQuery.bindValue(":name", name);
    checkQuery.bindValue(":connectionId", m_connectionId);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить название присоединия");

        return false;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Присоединение уже существует",
            QString(
                "Присоединение \"%1\" уже существует на этой подстанции.\n\n"
                "Введите другое название.")
                .arg(name));

        ui->nameLineEdit->setFocus();
        ui->nameLineEdit->selectAll();

        return false;
    }

    // Обновляем данные в БД
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE connections "
        "SET name = :name, "
            "voltage_kv = :voltage, "
            "ct_ratio = :ctRatio, "
            "version_uuid = :versionUuid "
        "WHERE id = :connectionId "
            "AND substation_id = :substationId "
            "AND version_uuid = :originalVersionUuid "
            "AND ("
                "name IS NOT :name "
                "OR voltage_kv IS NOT :voltage "
                "OR ct_ratio IS NOT :ctRatio);");

    query.bindValue(":versionUuid", UuidUtils::generate());
    query.bindValue(":name", name);
    query.bindValue(":voltage", voltage);

    if (ctRatio.isEmpty())
        query.bindValue(":ctRatio", QVariant());
    else
        query.bindValue(":ctRatio", ctRatio);

    query.bindValue(":connectionId", m_connectionId);
    query.bindValue(":substationId", m_substationId);
    query.bindValue(":originalVersionUuid", m_originalVersionUuid);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить присоединение.\n\n" +
            query.lastError().text());

        return false;
    }

    // Проверяем успешность
    if (query.numRowsAffected() != 1)
    {
        QMessageBox::warning(this, "Конфликт изменений",
            "Не удалось сохранить присоединение.\n\n"
            "Возможно, оно было изменено или удалено "
            "другим пользователем.");

        return false;
    }

    return true;
}

void ConnectionDialog::loadConnection()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT name, voltage_kv, ct_ratio, version_uuid "
        "FROM connections "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId");

    query.bindValue(":connectionId", m_connectionId);

    query.bindValue(":substationId", m_substationId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить присоединение.\n\n" +
            query.lastError().text());

        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Присоединение не найдено.");

        return;
    }

    ui->nameLineEdit->setText(
        query.value("name").toString());

    if (!query.value("voltage_kv").isNull())
    {
        ui->voltageDoubleSpinBox->setValue(
            query.value("voltage_kv").toDouble());
    }

    ui->ctRatioLineEdit->setText(
        query.value("ct_ratio").toString());

    // Получаем текущую UUID
    m_originalVersionUuid = query.value("version_uuid").toString();

    qDebug() << "Original connection version UUID: " << m_originalVersionUuid;
}
















