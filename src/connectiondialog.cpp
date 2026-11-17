
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

    // Настраиваем отображение элементов
    ui->typeComboBox->addItem("Ячейка", 1);
    ui->typeComboBox->addItem("ВЛ", 2);
    ui->typeComboBox->addItem("Другое", 3);

    ui->ctSecondaryComboBox->addItem("5", 5);
    ui->ctSecondaryComboBox->addItem("1", 1);

    ui->ctPrimarySpinBox->setValue(100);
    ui->ctSecondaryComboBox->setCurrentIndex(0);

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (save())
                accept();
        });

    // Подключаем выбор типа присоединения
    connect(ui->typeComboBox, &QComboBox::currentIndexChanged, this,
        [this](int)
        {
            const bool isCell = ui->typeComboBox->currentData().toInt() == 1;

            ui->cellNumberLabel->setVisible(isCell);
            ui->cellNumberSpinBox->setVisible(isCell);
        });

    // Подключаем чекбокс наличия ТТ
    connect(ui->directConnectionCheckBox, &QCheckBox::toggled, this,
        [this](bool checked)
        {
            ui->ctPrimarySpinBox->setEnabled(!checked);
            ui->ctSecondaryComboBox->setEnabled(!checked);
        });

    // Устанавливаем стандартные данные
    ui->typeComboBox->setCurrentIndex(0);
    ui->cellNumberLabel->setVisible(true);
    ui->cellNumberSpinBox->setVisible(true);
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

    const int connectionType =
        ui->typeComboBox->currentData().toInt();

    if (connectionType == 1 &&
        ui->cellNumberSpinBox->value() <= 0)
    {
        QMessageBox::warning(this, "Ошибка",
                             "Укажите номер ячейки.");
        ui->cellNumberSpinBox->setFocus();
        return false;
    }

    if (!ui->directConnectionCheckBox->isChecked())
    {
        if (ui->ctPrimarySpinBox->value() <= 0)
        {
            QMessageBox::warning(this, "Ошибка",
                                 "Укажите первичный ток ТТ.");
            ui->ctPrimarySpinBox->setFocus();
            return false;
        }
    }


    if (m_connectionId < 0)
        return insertConnection();

    return updateConnection();
}

bool ConnectionDialog::insertConnection()
{
    const QString name = ui->nameLineEdit->text().trimmed();
    const int connectionType = ui->typeComboBox->currentData().toInt();
    const double voltage = ui->voltageDoubleSpinBox->value();

    const QVariant cellNumber = connectionType == 1
        ? QVariant(ui->cellNumberSpinBox->value())
        : QVariant();

    const bool directConnection =
        ui->directConnectionCheckBox->isChecked();

    const QVariant ctPrimary = directConnection
        ? QVariant()
        : QVariant(ui->ctPrimarySpinBox->value());

    const int ctSecondary = directConnection
        ? 0
        : ui->ctSecondaryComboBox->currentData().toInt();

    // Проверка уникальности названия в пределах типа и ПС.
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM connections "
            "WHERE substation_id = :substationId "
            "AND connection_type = :connectionType "
            "AND connection_type IN (2, 3) "
            "AND name = :name COLLATE NOCASE"
        ");"
    );

    checkQuery.bindValue(":substationId", m_substationId);
    checkQuery.bindValue(":connectionType", connectionType);
    checkQuery.bindValue(":name", name);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить название присоединения.\n\n"
            + checkQuery.lastError().text());
        return false;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Присоединение уже существует",
            "Присоединение с таким названием и типом "
            "уже существует на этой подстанции.");
        ui->nameLineEdit->setFocus();
        return false;
    }

    // Проверка уникальности номера ячейки на всей ПС.
    if (connectionType == 1)
    {
        QSqlQuery cellQuery(m_database.getDatabase());

        cellQuery.prepare(
            "SELECT EXISTS("
            "SELECT 1 FROM connections "
            "WHERE substation_id = :substationId "
            "AND connection_type = 1 "
            "AND cell_number = :cellNumber"
            ");");

        cellQuery.bindValue(":substationId", m_substationId);
        cellQuery.bindValue(":cellNumber", cellNumber);

        if (!cellQuery.exec() || !cellQuery.next())
        {
            QMessageBox::critical(this, "Ошибка",
                "Не удалось проверить номер ячейки.\n\n"
                + cellQuery.lastError().text());
            return false;
        }

        if (cellQuery.value(0).toBool())
        {
            QMessageBox::warning(this, "Ячейка уже существует",
                QString("Ячейка №%1 уже существует на этой подстанции.")
                    .arg(cellNumber.toInt()));
            ui->cellNumberSpinBox->setFocus();
            return false;
        }
    }

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO connections ("
        "uuid, version_uuid, substation_id, "
        "connection_type, cell_number, name, voltage_kv, "
        "ct_primary_current, ct_secondary_current"
        ") VALUES ("
        ":uuid, :versionUuid, :substationId, "
        ":connectionType, :cellNumber, :name, :voltage, "
        ":ctPrimary, :ctSecondary"
        ");");

    query.bindValue(":uuid", UuidUtils::generate());
    query.bindValue(":versionUuid", UuidUtils::generate());
    query.bindValue(":substationId", m_substationId);
    query.bindValue(":connectionType", connectionType);
    query.bindValue(":cellNumber", cellNumber);
    query.bindValue(":name", name);
    query.bindValue(":voltage", voltage);
    query.bindValue(":ctPrimary", ctPrimary);
    query.bindValue(":ctSecondary", ctSecondary);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить присоединение.\n\n"
            + query.lastError().text());
        return false;
    }

    return true;
}

bool ConnectionDialog::updateConnection()
{
    const QString name = ui->nameLineEdit->text().trimmed();
    const int connectionType = ui->typeComboBox->currentData().toInt();
    const double voltage = ui->voltageDoubleSpinBox->value();

    const QVariant cellNumber = connectionType == 1
        ? QVariant(ui->cellNumberSpinBox->value())
        : QVariant();

    const bool directConnection =
        ui->directConnectionCheckBox->isChecked();

    const QVariant ctPrimary = directConnection
        ? QVariant()
        : QVariant(ui->ctPrimarySpinBox->value());

    const int ctSecondary = directConnection
        ? 0
        : ui->ctSecondaryComboBox->currentData().toInt();

    // Получаем актуальные данные и проверяем версию.
    QSqlQuery currentQuery(m_database.getDatabase());

    currentQuery.prepare(
        "SELECT name, connection_type, cell_number, "
        "voltage_kv, ct_primary_current, "
        "ct_secondary_current, version_uuid "
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
            "Присоединение больше не существует.");
        return false;
    }

    if (currentQuery.value("version_uuid").toString()
        != m_originalVersionUuid)
    {
        QMessageBox::warning(this, "Конфликт изменений",
            "Присоединение было изменено после открытия окна.\n"
            "Ваши изменения не сохранены.");
        return false;
    }

    // Проверяем, изменились ли данные.

    const auto equalNullableInt =
    [](const QVariant &a, const QVariant &b)
    {
        if (a.isNull() || b.isNull())
            return a.isNull() && b.isNull();

        return a.toInt() == b.toInt();
    };

    const bool unchanged =
        currentQuery.value("name").toString() == name &&
        currentQuery.value("connection_type").toInt() == connectionType &&
        equalNullableInt(currentQuery.value("cell_number"), cellNumber) &&
        currentQuery.value("voltage_kv").toDouble() == voltage &&
        equalNullableInt(currentQuery.value("ct_primary_current"), ctPrimary) &&
        currentQuery.value("ct_secondary_current").toInt() == ctSecondary;

    if (unchanged)
        return true;

    // Проверка дублирования названия в пределах типа и ПС.
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM connections "
            "WHERE substation_id = :substationId "
            "AND connection_type = :connectionType "
            "AND connection_type IN (2, 3) "
            "AND name = :name COLLATE NOCASE "
            "AND id <> :connectionId"
        ");"
    );

    checkQuery.bindValue(":substationId", m_substationId);
    checkQuery.bindValue(":connectionType", connectionType);
    checkQuery.bindValue(":name", name);
    checkQuery.bindValue(":connectionId", m_connectionId);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить название присоединения.\n\n"
            + checkQuery.lastError().text());
        return false;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Присоединение уже существует",
            "Присоединение с таким названием и типом "
            "уже существует на этой подстанции.");
        ui->nameLineEdit->setFocus();
        return false;
    }

    // Проверка уникальности номера ячейки.
    if (connectionType == 1)
    {
        QSqlQuery cellQuery(m_database.getDatabase());

        cellQuery.prepare(
            "SELECT EXISTS("
            "SELECT 1 FROM connections "
            "WHERE substation_id = :substationId "
            "AND connection_type = 1 "
            "AND cell_number = :cellNumber "
            "AND id <> :connectionId"
            ");");

        cellQuery.bindValue(":substationId", m_substationId);
        cellQuery.bindValue(":cellNumber", cellNumber);
        cellQuery.bindValue(":connectionId", m_connectionId);

        if (!cellQuery.exec() || !cellQuery.next())
        {
            QMessageBox::critical(this, "Ошибка",
                "Не удалось проверить номер ячейки.\n\n"
                + cellQuery.lastError().text());
            return false;
        }

        if (cellQuery.value(0).toBool())
        {
            QMessageBox::warning(this, "Ячейка уже существует",
                QString("Ячейка №%1 уже существует на этой подстанции.")
                    .arg(cellNumber.toInt()));
            ui->cellNumberSpinBox->setFocus();
            return false;
        }
    }

    // Обновление с защитой от конфликта версий.
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE connections SET "
        "name = :name, "
        "connection_type = :connectionType, "
        "cell_number = :cellNumber, "
        "voltage_kv = :voltage, "
        "ct_primary_current = :ctPrimary, "
        "ct_secondary_current = :ctSecondary, "
        "version_uuid = :versionUuid "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId "
        "AND version_uuid = :originalVersionUuid;");

    query.bindValue(":name", name);
    query.bindValue(":connectionType", connectionType);
    query.bindValue(":cellNumber", cellNumber);
    query.bindValue(":voltage", voltage);
    query.bindValue(":ctPrimary", ctPrimary);
    query.bindValue(":ctSecondary", ctSecondary);
    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":connectionId", m_connectionId);
    query.bindValue(":substationId", m_substationId);
    query.bindValue(":originalVersionUuid", m_originalVersionUuid);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить присоединение.\n\n"
            + query.lastError().text());
        return false;
    }

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
        "SELECT name, connection_type, cell_number, "
        "voltage_kv, ct_primary_current, "
        "ct_secondary_current, version_uuid "
        "FROM connections "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId;");

    query.bindValue(":connectionId", m_connectionId);
    query.bindValue(":substationId", m_substationId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить присоединение.\n\n"
            + query.lastError().text());
        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка",
            "Присоединение не найдено.");
        return;
    }

    // Название
    ui->nameLineEdit->setText(
        query.value("name").toString());

    // Тип присоединения
    const int connectionType =
        query.value("connection_type").toInt();

    const int typeIndex =
        ui->typeComboBox->findData(connectionType);

    if (typeIndex >= 0)
        ui->typeComboBox->setCurrentIndex(typeIndex);

    // Номер ячейки
    if (!query.value("cell_number").isNull())
    {
        ui->cellNumberSpinBox->setValue(
            query.value("cell_number").toInt());
    }

    // Напряжение
    if (!query.value("voltage_kv").isNull())
    {
        ui->voltageDoubleSpinBox->setValue(
            query.value("voltage_kv").toDouble());
    }

    // Параметры трансформаторов тока
    const int ctSecondary =
        query.value("ct_secondary_current").toInt();

    const bool directConnection = (ctSecondary == 0);

    ui->directConnectionCheckBox->setChecked(directConnection);

    if (!directConnection)
    {
        ui->ctPrimarySpinBox->setValue(
            query.value("ct_primary_current").toInt());

        const int secondaryIndex =
            ui->ctSecondaryComboBox->findData(ctSecondary);

        if (secondaryIndex >= 0)
            ui->ctSecondaryComboBox->setCurrentIndex(secondaryIndex);
    }

    // Версия записи для защиты от конфликтов
    m_originalVersionUuid =
        query.value("version_uuid").toString();

    qDebug() << "Original connection version UUID: " << m_originalVersionUuid;
}
















