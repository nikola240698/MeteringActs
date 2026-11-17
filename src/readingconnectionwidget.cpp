#include "readingconnectionwidget.h"
#include "ui_readingconnectionwidget.h"

#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardItemModel>
#include <QListView>

ReadingConnectionWidget::ReadingConnectionWidget(
    Database &database,
    QWidget *parent
)
    : QWidget(parent),
      ui(new Ui::ReadingConnectionWidget),
      m_database(database)
{
    ui->setupUi(this);

    // Создаём существующий виджет прибора учёта
    m_meterWidget = new MeterActWidget(
        m_database,
        ui->meterCardContainer
    );

    connect(
    m_meterWidget, &MeterActWidget::meterSelected, this,
        [this]()
        {
            emit meterSelected(this);
        }
    );

    // Размещаем его внутри правой карточки
    ui->meterCardContainerLayout->addWidget(m_meterWidget);

    /*
    // Пока отключаем кнопку удаления
    ui->removeButton->setEnabled(false);
    */

    connect(ui->connectionComboBox,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int index)
        {
            Q_UNUSED(index);

            ui->voltageLineEdit->clear();
            ui->ctRatioLineEdit->clear();

            // При смене присоединения прежнее историческое название
            // больше не относится к выбранному присоединению
            m_originalConnectionName.clear();

            const QVariant connectionId =
                ui->connectionComboBox->currentData();

            if (!connectionId.isValid()) {
                emit connectionChanged(this, -1);
                return;
            }

            const int id = connectionId.toInt();

            loadConnectionData(id);

            emit connectionChanged(this, id);
        });

    // Подключаем кнопку удаления виджета
    connect(ui->removeButton, &QPushButton::clicked,
        this, [this]()
        {
            emit removeRequested(this);
        });
}

ReadingConnectionWidget::~ReadingConnectionWidget()
{
    delete ui;
}

void ReadingConnectionWidget::setSubstation(int substationId)
{
    ui->connectionComboBox->clear();
    ui->voltageLineEdit->clear();
    ui->ctRatioLineEdit->clear();

    if (substationId <= 0) {
        ui->connectionComboBox->addItem(
            "Выберите подстанцию...",
            QVariant()
        );
        return;
    }

    loadConnections(substationId);
}

// Восстанавливает сохранённые данные карточки группового акта
bool ReadingConnectionWidget::setData(const ActMeterData& data)
{
    // Ищем сохранённое присоединение в загруженном списке
    const int index =
        ui->connectionComboBox->findData(data.connectionId);

    if (index < 0)
    {
        qWarning() << "Reading connection not found:"
                   << data.connectionId;
        return false;
    }

    // Запоминаем запись прибора в редактируемом акте
    m_actMeterId = data.id;

    // Выбор присоединения автоматически загружает его текущие параметры
    ui->connectionComboBox->setCurrentIndex(index);

    // Запоминаем историческое название после выбора присоединения,
    // поскольку currentIndexChanged очищает его
    m_originalConnectionName = data.connectionName;

    // Восстанавливаем прибор и сохранённые показания
    m_meterWidget->setData(data);

    // Заменяем текущие параметры историческими значениями акта
    ui->voltageLineEdit->setText(
        data.connectionVoltage.isEmpty()
            ? QString()
            : data.connectionVoltage + " кВ"
    );

    ui->ctRatioLineEdit->setText(data.connectionCtRatio);

    return true;
}

void ReadingConnectionWidget::setNumber(int number)
{
    ui->connectionCardTitle->setText(
        QString("Присоединение № %1").arg(number)
    );
}

void ReadingConnectionWidget::setRemoveEnabled(bool enabled)
{
    ui->removeButton->setEnabled(enabled);
}

void ReadingConnectionWidget::setConnectionLocked(bool locked)
{
    // Запрещаем изменение выбранного присоединения,
    // но сохраняем возможность просмотра его параметров
    ui->connectionComboBox->setEnabled(!locked);

    // При редактировании существующего акта
    // запрещаем удаление карточки присоединения
    ui->removeButton->setVisible(!locked);
}

int ReadingConnectionWidget::selectedConnectionId() const
{
    const QVariant data = ui->connectionComboBox->currentData();

    return data.isValid() ? data.toInt() : -1;
}

void ReadingConnectionWidget::setUnavailableConnections(
    const QSet<int> &connectionIds)
{
    const int selectedId = selectedConnectionId();

    auto *view = qobject_cast<QListView *>(
        ui->connectionComboBox->view()
    );

    if (!view)
        return;

    for (int i = 0; i < ui->connectionComboBox->count(); ++i) {
        const QVariant data =
            ui->connectionComboBox->itemData(i);

        // Пункт "Выберите присоединение..." оставляем
        if (!data.isValid()) {
            view->setRowHidden(i, false);
            continue;
        }

        const int connectionId = data.toInt();

        const bool occupied =
            connectionIds.contains(connectionId) &&
            connectionId != selectedId;

        view->setRowHidden(i, occupied);
    }
}

bool ReadingConnectionWidget::validateMeter()
{
    return m_meterWidget->validate();
}

QString ReadingConnectionWidget::meterSerialNumber() const
{
    return m_meterWidget->serialNumber().trimmed();
}

// Возвращает вложенный виджет прибора учёта
MeterActWidget* ReadingConnectionWidget::meterWidget() const
{
    return m_meterWidget;
}

// Собирает текущее содержимое карточки группового акта
ActMeterData ReadingConnectionWidget::data() const
{
    ActMeterData result;

    // Сохраняем связь с исходной записью act_meters
    result.id = m_actMeterId;

    // Присоединение
    result.connectionId = selectedConnectionId();
    result.connectionName = m_originalConnectionName.isEmpty()
    ? ui->connectionComboBox->currentText()
    : m_originalConnectionName;
    result.connectionVoltage =
        ui->voltageLineEdit->text().trimmed();
    result.connectionVoltage.remove(" кВ");

    result.connectionCtRatio =
        ui->ctRatioLineEdit->text().trimmed();

    // Прибор учёта
    result.role = 4; // ReadingMeter
    result.meterId = m_meterWidget->meterId();
    result.name = m_meterWidget->meterName();
    result.serialNumber = m_meterWidget->serialNumber();
    result.accuracyClass = m_meterWidget->accuracyClass();
    result.verificationYear = m_meterWidget->verificationYear();

    // Показания прибора
    for (const auto& reading : m_meterWidget->readings())
    {
        ActMeterReadingData item;

        item.typeId = reading.typeId;
        item.value = reading.value;

        result.readings.append(item);
    }

    return result;
}

void ReadingConnectionWidget::loadConnections(int substationId)
{
    ui->connectionComboBox->clear();

    ui->connectionComboBox->addItem(
        "Выберите присоединение...",
        QVariant()
    );

    QSqlQuery query(m_database.getDatabase());

    query.prepare(R"(
        SELECT
            id,
            CASE connection_type
                WHEN 1 THEN
                    'яч.' || printf('%g', voltage_kv) ||
                    'кВ №' || cell_number || ' "' || name || '"'
                WHEN 2 THEN
                    'ВЛ-' || printf('%g', voltage_kv) ||
                    'кВ "' || name || '"'
                ELSE name
            END AS display_name
        FROM connections
        WHERE substation_id = :substationId
        ORDER BY
            connection_type ASC,
            voltage_kv DESC,
            cell_number ASC,
            name COLLATE NOCASE ASC
    )");

    query.bindValue(":substationId", substationId);

    if (!query.exec()) {
        qWarning() << "Failed to load reading connections:"
                   << query.lastError().text();
        return;
    }

    while (query.next()) {
        ui->connectionComboBox->addItem(
            query.value("display_name").toString(),
            query.value("id")
        );
    }
}

void ReadingConnectionWidget::loadConnectionData(int connectionId)
{

    qDebug() << "Loading connection data for ID:" << connectionId;

    QSqlQuery query(m_database.getDatabase());

    if (!query.prepare(R"(
        SELECT
            voltage_kv,
            ct_primary_current,
            ct_secondary_current
        FROM connections
        WHERE id = :connectionId
    )"))
    {
        qWarning() << "Prepare failed:"
                   << query.lastError().text();
        return;
    }

    query.bindValue(":connectionId", connectionId);

    qDebug() << "SQL:" << query.lastQuery();
    qDebug() << "Bound values:" << query.boundValues();

    if (!query.exec()) {
        qWarning() << "Failed to load connection data:"
                   << query.lastError().text();
        return;
    }

    if (!query.next())
        return;

    // Напряжение присоединения
    const double voltage = query.value("voltage_kv").toDouble();

    qDebug() << "Voltage:" << query.value("voltage_kv");
    qDebug() << "CT primary:" << query.value("ct_primary_current");
    qDebug() << "CT secondary:" << query.value("ct_secondary_current");

    ui->voltageLineEdit->setText(
        QString::number(voltage, 'g', 10) + " кВ"
    );

    // Коэффициент трансформации тока
    const int primary = query.value("ct_primary_current").toInt();
    const int secondary = query.value("ct_secondary_current").toInt();

    if (secondary == 0) {
        ui->ctRatioLineEdit->setText("Прямого включения");
    } else {
        ui->ctRatioLineEdit->setText(
            QString("%1/%2").arg(primary).arg(secondary)
        );
    }
}