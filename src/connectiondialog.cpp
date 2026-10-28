
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



    const double voltage =
        ui->voltageDoubleSpinBox->value();

    const QString ctRatio =
        ui->ctRatioLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE connections "
        "SET name = :name, "
            "voltage_kv = :voltage, "
            "ct_ratio = :ctRatio, "
            "version_uuid = :versionUuid "
        "WHERE id = :connectionId "
            "AND substation_id = :substationId "
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

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить присоединение.\n\n" +
            query.lastError().text());

        return false;
    }

    return true;
}

void ConnectionDialog::loadConnection()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT name, voltage_kv, ct_ratio "
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
}
















