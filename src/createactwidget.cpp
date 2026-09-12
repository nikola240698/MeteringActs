
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

    // вводим текст-подскахку в поле
    ui->serialNumberLineEdit->setPlaceholderText("Введите серийный номер...");

    // слот нажатия Enter при вводе серийного номера
    connect(ui->serialNumberLineEdit, &QLineEdit::returnPressed, this,
        [this]()
        {
            findMeterBySerial();
        });
    // слот нажатия кнопки "Найти"
    connect(ui->findMeterButton, &QPushButton::clicked, this, &CreateActWidget::findMeterBySerial);

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

// метод поиска прибора по серийному номеру
void CreateActWidget::findMeterBySerial()
{
    // получаем серийный номер
    QString serial = ui->serialNumberLineEdit->text().trimmed();
    // если не ввели серийный номер
    if (serial.isEmpty())
        return;
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT id, name, serial_number, accuracy_class, verification_year "
        "FROM meters "
        "WHERE serial_number = :serial;");
    // биндим значение
    query.bindValue(":serial", serial);
    // проверяем, что он выполняется
    if (!query.exec())
    {
        qDebug() << "finMeterBySerial error: " << query.lastError().text();
        return;
    }
    // если найдет прибор
    if (query.next())
    {
        // получаем его id
        m_currentMeterId = query.value("id").toInt();
        // вставляем остальные значения
        ui->meterNameLineEdit->setText(query.value("name").toString());
        ui->accuracyClassLineEdit->setText(query.value("accuracy_class").toString());
        ui->verificationYearLineEdit->setText(query.value("verification_year").toString());
        return;
    }
    // если прибор не найден
    auto answer = QMessageBox::question(
        this,
        "Прибор не найден",
        "Прибор с серийным номером " + serial +
        " не найден. \n\nДобавить новый прибор?",
        QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
    {
        MeterDialog dialog(m_database, serial, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            m_currentMeterId = dialog.createdMeterId();

            loadMeterData(m_currentMeterId);
        }
    }
}

















