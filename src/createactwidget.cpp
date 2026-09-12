
#include "createactwidget.h"
#include "ui_CreateActWidget.h"


CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);

    // вызываем загрузку типов актов
    loadActTypes();
    // вызываем загрузку списка участков
    loadAreas();
    // вызываем загрузку списка представителей
    loadEmployees();
    // подключаем сигнал изменения выбора участка
    connect(ui->areaComboBox, &QComboBox::currentIndexChanged, this,
        [this](int)
        {
            // получаем данные выбранного участка
            QVariant data = ui->areaComboBox->currentData();
            // очищаем поля
            ui->substationComboBox->clear();
            ui->connectionComboBox->clear();
            ui->voltageLineEdit->clear();
            ui->ctRatioLineEdit->clear();
            // проверяем, что выбрали что-то из списка
            if (!data.isValid())
            {
                // выставляем значения по умолчанию
                ui->substationComboBox->addItem("Выберите подстанцию...", QVariant());
                ui->connectionComboBox->addItem("Выберите присоединение...", QVariant());
                return;
            }
            // получаем id выбранного участка
            int areaId = data.toInt();
            // загружаем список подстанций данного участка
            loadSubstations(areaId);
        });

    // сигнал выбора подстанции из выпадающего списка
    connect(ui->substationComboBox, &QComboBox::currentIndexChanged, this,
        [this](int)
        {
            // получаем данные из выбранного варианта
            QVariant data = ui->substationComboBox->currentData();
            // очищаем поля
            ui->connectionComboBox->clear();
            ui->voltageLineEdit->clear();
            ui->ctRatioLineEdit->clear();
            // проверяем что выбрали что-то из списка
            if (!data.isValid())
            {
                // Выставляем значение по умолчанию
                ui->connectionComboBox->addItem("Выберите присоединение...", QVariant());
                return;
            }
            // получаем id подстанции
            int substationId = data.toInt();
            // загружаем список присоединений
            loadConnections(substationId);
        });

    // подключаем сигнал выбора присоединения из выпадающего списка
    connect(ui->connectionComboBox, &QComboBox::currentIndexChanged, this,
        [this](int)
        {
            // получаем данные выбранного пункта
            QVariant data = ui->connectionComboBox->currentData();
            // очищаем поля
            ui->voltageLineEdit->clear();
            ui->ctRatioLineEdit->clear();
            // проверяем, что выбрали что-то из списка
            if (!data.isValid())
            {
                return;
            }
            // получаем id присоединения
            int connectionId = data.toInt();
            // загружаем данные присоединения
            loadConnectionData(connectionId);
        });

    // загружаем данные прибора
    loadMeters();

    // подключаем сигнал выбора прибора из списка
    connect(ui->meterComboBox, &QComboBox::currentIndexChanged, this,
        [this](int)
        {
            // получаем выбранные данные
            QVariant data = ui->meterComboBox->currentData();
            // очищаем поля
            ui->meterNameLineEdit->clear();
            ui->serialNumberLineEdit->clear();
            ui->accuracyClassLineEdit->clear();
            ui->verificationYearLineEdit->clear();
            // проверяем полученные данные
            if (!data.isValid())
                return;
            // загружаем данные прибора
            loadMeterData(data.toInt());
        });

    // слот нажатия на кнопку "+Новый прибор"
    connect(ui->addMeterButton, &QPushButton::clicked, this,
        [this]()
        {
            // создаем диалоговое окно
            MeterDialog dialog(m_database, this);
            // проверяем, что оно запускается и сохраняются изменения
            if (dialog.exec() != QDialog::Accepted)
                return;
            // получаем id созданного прибора
            int newMeterId = dialog.createdMeterId();
            // перезагружаем список приборов
            loadMeters();
            // получаем индекс при поиске нашего прибора в общем списке
            int index = ui->meterComboBox->findData(newMeterId);
            // если индекс найден
            if (index >= 0)
                ui->meterComboBox->setCurrentIndex(index);
        });

}

CreateActWidget::~CreateActWidget()
{
    delete ui;
}

// метод загрузки типов актов
void CreateActWidget::loadActTypes() const
{
    // очищаем выпадающий список
    ui->actTypeComboBox->clear();
    // вставляем значение по-умолчанию
    ui->actTypeComboBox->addItem("Выберите тип акта...", QVariant());
    // создаем запрос и пробуем его исполнить
    QSqlQuery query(m_database.getDatabase());
    if (!query.exec(
        "SELECT id, name "
        "FROM act_types "
        "ORDER BY id"))
    {
        qDebug() << "LoadActTypes error: " << query.lastError().text();
        return;
    }
    // вставляем данные в выпадающий список при успешном выполнении запроса
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        ui->actTypeComboBox->addItem(name, id);
    }
}

// метод загрузки участков предприятия
void CreateActWidget::loadAreas() const
{
    // очищаем выпадающий список
    ui->areaComboBox->clear();
    // добавляем значение по-умолчанию
    ui->areaComboBox->addItem(
        "Выберите участок...", QVariant());
    // создаем запрос и пробуем его исполнить
    QSqlQuery query(m_database.getDatabase());
    if (!query.exec(
        "SELECT id, name "
        "FROM areas "
        "ORDER BY name"))
    {
        qDebug() << "loadAreas error: " << query.lastError().text();
        return;
    }
    // вставляем все полученные данные из запроса
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        ui->areaComboBox->addItem(name, id);
    }
}

// метод загрузки представителей предприятия
void CreateActWidget::loadEmployees() const
{
    // очищаем выпадающий список
    ui->employeeComboBox->clear();
    // добавляем значения по-умолчанию
    ui->employeeComboBox->addItem(
        "Выберите представителя...", QVariant());
    // создаем запрос и пробуем его исполнить
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
    // вставляем полученные данные из запроса в выпадающий список
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString shortName = query.value("short_name").toString();
        QString position = query.value("position").toString();
        // склеиваем строку
        QString displayText = shortName + " - " + position;
        ui->employeeComboBox->addItem(displayText, id);
    }
}

// метод загрузки подстанций
void CreateActWidget::loadSubstations(int areaId) const
{
    // очищаем выпадающий список
    ui->substationComboBox->clear();
    // вставляем значение по-умолчанию
    ui->substationComboBox->addItem("Выберите подстанцию...", QVariant());
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT id, name "
        "FROM substations "
        "WHERE area_id = :areaId "
        "ORDER BY name;");
    // биндим переменные в запрос
    query.bindValue(":areaId", areaId);
    // проверяем, что запрос выполнен
    if (!query.exec())
    {
        qDebug() << "loadSubstations error: " << query.lastError().text();
        return;
    }
    // вносим данные в выпадающий список
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        ui->substationComboBox->addItem(name, id);
    }
}

// метод загрузки присоединений
void CreateActWidget::loadConnections(int substationId) const
{
    // очищаем выпадающий список и вставляем значение по-умолчанию
    ui->connectionComboBox->clear();
    ui->connectionComboBox->addItem("Выберите присоединение...", QVariant());
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT id, name "
        "FROM connections "
        "WHERE substation_id = :substationId "
        "ORDER BY name;");
    // биндим значения в запрос
    query.bindValue(":substationId", substationId);
    // проверяем, что запрос выполенен
    if (!query.exec())
    {
        qDebug() << "loadConnection error: " << query.lastError().text();
        return;
    }
    // вставляем полученные данные в выпадающий список
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        ui->connectionComboBox->addItem(name, id);
    }

}

// метод загрузки данные присоединения
void CreateActWidget::loadConnectionData(int connectionId) const
{
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT voltage_kv, ct_ratio "
        "FROM connections "
        "WHERE id = :connectionId;");
    // биндим переменные в запрос

    query.bindValue(":connectionId", connectionId);
    // проверяем, что запрос выполняется
    if (!query.exec())
    {
        qDebug() << "loadConnectionData error: " << query.lastError().text();
        return;
    }
    // проверяем, что есть хоть одно значение
    if (!query.next())
    {
        return;
    }
    // получаем значения и вставляем их в поля
    QString voltage = query.value("voltage_kv").toString();
    QString ctRatio = query.value("ct_ratio").toString();
    ui->voltageLineEdit->setText(voltage + " кВ");
    ui->ctRatioLineEdit->setText(ctRatio);
}

void CreateActWidget::loadMeters() const
{
    // очищаем поле и добавляем значение по-умолчанию
    ui->meterComboBox->clear();
    ui->meterComboBox->addItem("Выберите прибор...", QVariant());
    // создаем запрос
    QSqlQuery query(m_database.getDatabase());
    // пробуем выполнить его
    if (!query.exec(
        "SELECT id, name, serial_number "
        "FROM meters "
        "ORDER BY name, serial_number;"))
    {
        qDebug() << "loadMeters error: " << query.lastError().text();
        return;
    }
    // получаем значения из запроса
    while (query.next())
    {
        int id = query.value("id").toInt();
        QString name = query.value("name").toString();
        QString serial = query.value("serial_number").toString();
        QString text = name + " - №" + serial;
        ui->meterComboBox->addItem(text, id);
    }
}

void CreateActWidget::loadMeterData(int meterId) const
{
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT name, serial_number, accuracy_class, verification_year "
        "FROM meters "
        "WHERE id = :meterId;");
    // биндим значения в запрос
    query.bindValue(":meterId", meterId);
    // проверяем что запрос выполняется
    if (!query.exec())
    {
        qDebug() << "loadMeterData error: " << query.lastError().text();
        return;
    }
    // проверяем, что нашлось хоть одно значение
    if (!query.next())
        return;
    // вставляем в поля полученные данные
    ui->meterNameLineEdit->setText(query.value("name").toString());
    ui->serialNumberLineEdit->setText(query.value("serial_number").toString());
    ui->accuracyClassLineEdit->setText(query.value("accuracy_class").toString());
    ui->verificationYearLineEdit->setText(query.value("verification_year").toString());

}

















