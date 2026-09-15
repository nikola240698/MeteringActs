
#include "createactwidget.h"
#include "ui_CreateActWidget.h"


CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);
    // настраиваем виджет прибора и показаний
    m_meterWidget = new MeterActWidget(m_database, ui->meterContainerWidget);
    // добавляем виджет
    ui->meterContainerWidget->layout()->addWidget(m_meterWidget);

    // устанавливаем сегодняшнюю дату
    ui->actDateEdit->setDate(QDate::currentDate());
    // вызываем метод настройки показаний
    setupReadings();


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


    // сигнал нажатия кнопки сохранения
    connect(ui->createButton, &QPushButton::clicked, this,
        [this]()
        {
            saveAct();
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





// метод проверки правильности заполнения формы
bool CreateActWidget::validateForm()
{
    // 1. Проверяем тип акта
    if (!ui->actTypeComboBox->currentData().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Выберите тип акта");
        ui->actTypeComboBox->setFocus();
        return false;
    }

    // 2. Проверяем дату
    if (!ui->actDateEdit->date().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Укажите корректную дату акта.");
        ui->actDateEdit->setFocus();
        return false;
    }

    // 3. Проверяем выбранный ЛПУ
    if (!ui->areaComboBox->currentData().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Выберите участок.");
        ui->areaComboBox->setFocus();
        return false;
    }

    // 4. Проверяем подстанцию
    if (!ui->substationComboBox->currentData().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Выберите подстанцию");
        ui->substationComboBox->setFocus();
        return false;
    }

    // 5. Проверяем присоединение
    if (!ui->connectionComboBox->currentData().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Выберите присоединение.");
        ui->connectionComboBox->setFocus();
        return false;
    }

    // 6. Проверяем представителя предприятия
    if (!ui->employeeComboBox->currentData().isValid())
    {
        QMessageBox::warning(this, "Не заполнено поле", "Выберите представителя предприятия.");
        ui->employeeComboBox->setFocus();
        return false;
    }

    // 7-10. Объединили теперь так как это всё в одном классе
    if (!m_meterWidget->validate())
        return false;


    // 11. Проверка ввода причины проверки
    if (ui->resultPlainTextEdit->toPlainText().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите причину выполнения работ.");
        ui->reasonPlainTextEdit->setFocus();
        return false;
    }

    // 12. Проверяем ввод заключения
    if (ui->resultPlainTextEdit->toPlainText().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите заключение по результатам выполнения работ. ");
        ui->resultPlainTextEdit->setFocus();
        return false;
    }

    return true;
}

// главный метод сохранения акта
bool CreateActWidget::saveAct()
{
    // проверяем, что форма заполнена правильно
    if (!validateForm())
        return false;
    // пробуем начать транзакцию
    if (!m_database.transaction())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось начать транзакцию: " + m_database.lastError());
        return false;
    }
    // вставляем сам акт и получаем id
    int actId = insertAct();
    // проверяем, что всё нормально
    if (actId < 0)
    {
        m_database.rollback();
        return false;
    }
    // вставляем данные прибора и получаем его id
    int actMeterId = insertActMeter(actId);
    // проверяем успех
    if (actMeterId < 0)
    {
        m_database.rollback();
        return false;
    }
    // пробуем вставить показания по id прибора
    if (!insertReadings(actMeterId))
    {
        m_database.rollback();
        return false;
    }
    // пробуем обновить год поверки прибора учета
    if (!updateMeterVerificationYear())
    {
        m_database.rollback();
        return false;
    }
    // пробуем применить изменения в БД
    if (!m_database.commit())
    {
        QString error = m_database.lastError();

        m_database.rollback();

        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось завершить транзакцию: " + error);
        return false;
    }

    QMessageBox::information(this, "Акт сохранен", "Данные акта успешно сохранены");

    return true;
}

// метод сохранения самого акта
int CreateActWidget::insertAct()
{
    // получаем необходимые данные с формы ввода
    int actTypeId = ui->actTypeComboBox->currentData().toInt();
    int connectionId = ui->connectionComboBox->currentData().toInt();
    int employeeId = ui->employeeComboBox->currentData().toInt();
    QString actDate = ui->actDateEdit->date().toString(Qt::ISODate);
    QString reason = ui->reasonPlainTextEdit->toPlainText().trimmed();
    QString result = ui->resultPlainTextEdit->toPlainText().trimmed();
    // получаем необходимый тип данных представителей предприятия
    QSqlQuery employeeQuery(m_database.getDatabase());

    employeeQuery.prepare(
        "SELECT short_name, position "
        "FROM employees "
        "WHERE id = :employeeId;");

    employeeQuery.bindValue(":employeeId", employeeId);

    if (!employeeQuery.exec() || !employeeQuery.next())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось получить данные представителя: "
            + employeeQuery.lastError().text());
        return -1;
    }

    QString employeeName = employeeQuery.value("short_name").toString();

    QString employeePosition = employeeQuery.value("position").toString();

    // вносим данные в базу данных
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO acts ("
        "act_type_id, "
        "act_date, "
        "connection_id, "
        "employee_id, "
        "employee_name, "
        "employee_position, "
        "reason, "
        "result"
        ") "
        "VALUES ("
        ":actTypeId, "
        ":actDate, "
        ":connectionId, "
        ":employeeId, "
        ":employeeName, "
        ":employeePosition, "
        ":reason, "
        ":result"
        ");");

    query.bindValue(":actTypeId", actTypeId);
    query.bindValue(":actDate", actDate);
    query.bindValue(":connectionId", connectionId);
    query.bindValue(":employeeId", employeeId);
    query.bindValue(":employeeName", employeeName);
    query.bindValue(":employeePosition", employeePosition);
    query.bindValue(":reason", reason);
    query.bindValue(":result", result);

    if (!query.exec())
    {
        QMessageBox::critical(
            this, "Ошибка базы данных",
            "Не удалось сохранить акт: " + query.lastError().text());

        return -1;
    }
    // возвращаем id внесенного акта
    return query.lastInsertId().toInt();
}

// сохраняем прибор с привязкой к акту
int CreateActWidget::insertActMeter(int actId, MeterActWidget* meterWidget, int role)
{
    // вставляем полученные данные в нашу таблицу связи акта и прибора
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO act_meters ("
        "act_id, "
        "meter_id, "
        "role, "
        "meter_name, "
        "serial_number, "
        "accuracy_class, "
        "verification_year"
        ") "
        "VALUES ("
        ":actId, "
        ":meterId, "
        ":role, "
        ":meterName, "
        ":serialNumber, "
        ":accuracyClass, "
        ":verificationYear"
        ");");

    query.bindValue(":actId", actId);
    query.bindValue(":meterId", meterWidget->meterId());
    query.bindValue(":role", role);
    query.bindValue(":meterName", meterWidget->meterName());
    query.bindValue(":serialNumber", meterWidget->serilaNumber());
    query.bindValue(":accuracyClass", meterWidget->accuracyClass());
    query.bindValue(":verificationYear", meterWidget->verificationYear());

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось сохранить данные прибора в акте: " + query.lastError().text());

        return -1;
    }
    // возвращаем id введенного прибора
    return query.lastInsertId().toInt();
}

// метод вставки показаний с привязкой к прибору
bool CreateActWidget::insertReadings(int actMeterId, MeterActWidget* meterWidget)
{
    // получаем показания из метода
    const auto readings = meterWidget->readings();
    // вставляем их в нашу БД
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO meter_readings ("
        "act_meter_id, "
        "reading_type_id, "
        "value"
        ")"
        "VALUES ("
        ":actMeterId, "
        ":readingTypeId, "
        ":value"
        ");");

    for (const auto &reading : readings)
    {
        query.bindValue(":actMeterId", actMeterId);

        query.bindValue(":readingTypeId", reading.typeId);

        query.bindValue(":value", reading.value);

        if (!query.exec())
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                " Не удалось сохранить показания прибора: " + query.lastError().text());
            return false;
        }
    }

    return true;
}

// метод обновления актуального года поверки прибора
bool CreateActWidget::updateMeterVerificationYear(MeterActWidget* meterWidget)
{
    // получаем введенный год
    int verificationYear = ui->verificationYearLineEdit->text().toInt();
    // обновляем значение в БД
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE meters "
        "SET verification_year = :verificationYear "
        "WHERE id = :meterId;");

    query.bindValue("verificationYear", verificationYear);

    query.bindValue(":meterId", meterWidget->meterId());

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось обновить год поверки прибора: "
            + query.lastError().text());

        return false;
    }

    return true;
}




















