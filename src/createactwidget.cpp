
#include <QDebug>

#include "createactwidget.h"
#include "ui_createactwidget.h"
#include "actrepository.h"


CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);
    // настраиваем виджет первого прибора и показаний
    m_primaryMeterWidget = new MeterActWidget(m_database, ui->meterContainerWidget);
    // добавляем виджет
    ui->meterContainerWidget->layout()->addWidget(m_primaryMeterWidget);
    // подобно настраиваем второй виджет
    m_secondaryMeterWidget = new MeterActWidget(m_database, ui->secondMeterContainerWidget);
    ui->secondMeterContainerWidget->layout()->addWidget(m_secondaryMeterWidget);
    // отключаем по стандарту видимость второго прибора
    ui->secondaryMeterGroupBox->setVisible(false);

    // создаем виджет векторной диаграммы
    m_vectorDiagramWidget = new VectorDiagramWidget(ui->vectorDiagramContainerWidget);
    // добавляем поля векторной диаграммы на главный экран
    ui->vectorDiagramContainerWidget->layout()->addWidget(m_vectorDiagramWidget);
    // сразу скрываем все поля
    ui->vectorDiagramContainerWidget->setVisible(false);
    ui->replacementDurationWidget->setVisible(false);

    // устанавливаем сегодняшнюю дату
    ui->actDateEdit->setDate(QDate::currentDate());

    // скрываем поля для ввода данных про трансформаторы тока
    ui->removedCurrentTransformersGroupBox->setVisible(false);
    ui->installedCurrentTransformersGroupBox->setVisible(false);

    // Заполняем ComboBox характера работ
    ui->workScheduleComboBox->addItem("Выберите характер работ...", 0);
    ui->workScheduleComboBox->addItem("Плановая", PlannedWork);
    ui->workScheduleComboBox->addItem("Внеплановая", UnplannedWork);
    // скрываем по умолчанию данный виджет
    ui->workScheduleWidget->setVisible(false);

    // вызываем загрузку типов актов
    loadActTypes();
    // сигнал изменения типа акта для показа полей для второго прибора
    connect(ui->actTypeComboBox, &QComboBox::currentIndexChanged, this,
        [this]()
        {
            updateActTypeUi();
        });
    // Обновляем показ UI лдя правильного отображения
    updateActTypeUi();
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

    // сигнал добавления нового представителя
    connect(ui->addExternalRepresentativeButton, &QPushButton::clicked, this,
        [this]()
        {
            addExternalRepresentative();
        });

    // слот привязки чекбокса наличия векторной диаграммы
    connect(ui->hasVectorDiagramCheckBox, &QCheckBox::toggled, this,
        [this]()
        {
            updateVectorDiagramUi();
        });

    // сигналы для кнопок работы с трансформаторами тока
    connect(ui->addRemoveCtButton, &QPushButton::clicked, this,
    [this]()
    {
        addRemovedCurrentTransformer();
    });
    connect(ui->addInstalledCtButton, &QPushButton::clicked, this,
        [this]()
        {
            addInstalledCurrentTransformer();
        });

    // сигнал нажатия кнопки сохранения
    connect(ui->createButton, &QPushButton::clicked, this,
        [this]()
        {
            saveAct();
        });

    // кнопка очистки формы
    connect(ui->clearButton, &QPushButton::clicked, this, &CreateActWidget::clearForm);
}

CreateActWidget::CreateActWidget(Database &database, int actId, QWidget *parent) :
    CreateActWidget(database, parent)
{
    m_mode = Mode::Edit;
    m_editActId = actId;

    loadActForEditing(actId);
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
    // проверяем, что запрос выполнен
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
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Выберите тип акта");
        ui->actTypeComboBox->setFocus();
        return false;
    }

    // 2. Проверяем дату
    if (!ui->actDateEdit->date().isValid())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Укажите корректную дату акта.");
        ui->actDateEdit->setFocus();
        return false;
    }

    // 3. Проверяем выбранный ЛПУ
    if (!ui->areaComboBox->currentData().isValid())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Выберите участок.");
        ui->areaComboBox->setFocus();
        return false;
    }

    // 4. Проверяем подстанцию
    if (!ui->substationComboBox->currentData().isValid())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Выберите подстанцию");
        ui->substationComboBox->setFocus();
        return false;
    }

    // 5. Проверяем присоединение
    if (!ui->connectionComboBox->currentData().isValid())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Выберите присоединение.");
        ui->connectionComboBox->setFocus();
        return false;
    }

    // 6. Проверяем представителя предприятия
    if (!ui->employeeComboBox->currentData().isValid())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле", "Выберите представителя предприятия.");
        ui->employeeComboBox->setFocus();
        return false;
    }

    // 7-10. Проверяем введенные данные прибора.
    // Объединили теперь так как это всё в одном классе
    if (!m_primaryMeterWidget->validate())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->metersTab);
        return false;
    }

    // добавляем проверку введенных полей второго прибора учета, если он видим
    // в данном типе акта
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();
    if (actTypeId == 2)
    {
        if (!m_secondaryMeterWidget->validate())
        {
            // переключаемся на нужную вкладку
            ui->actTabWidget->setCurrentWidget(ui->metersTab);
            return false;
        }
    }

    // 11. Проверка ввода причины проверки
    if (ui->reasonPlainTextEdit->toPlainText().trimmed().isEmpty())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->conclusionTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите причину выполнения работ.");
        ui->reasonPlainTextEdit->setFocus();
        return false;
    }

    // 12. Проверяем ввод заключения
    if (ui->resultPlainTextEdit->toPlainText().trimmed().isEmpty())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->conclusionTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите заключение по результатам выполнения работ. ");
        ui->resultPlainTextEdit->setFocus();
        return false;
    }

    // 13. Проверяем правильно введенную пломбу
    if (ui->sealWidget->isVisible() &&
        ui->sealLineEdit->text().trimmed().isEmpty())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->conclusionTab);
        // выводим сообщение и выставляем фокус на поле
        QMessageBox::warning(this, "Поле не заполнено", "Укажите номер пломбы");
        ui->sealLineEdit->setFocus();
        return false;
    }

    // 14. Проверяем сторонник представителей, если есть
    for (ExternalRepresentativeWidget *representative :
        m_externalRepresentativeWidgets)
    {
        if (!representative->validate())
        {
            // переключаемся на нужную вкладку
            ui->actTabWidget->setCurrentWidget(ui->mainTab);
            return false;
        }
    }

    // 15. Проверка валидности векторной диаграммы
    // Условие наличия векторной по типам актов
    const bool supportsVectorDiagram =
        actTypeId == 1 ||
        actTypeId == 2 ||
        actTypeId == 5 ||
        actTypeId == 6 ||
        actTypeId == 7;
    // проверяем по типам актов и отмеченной галочке
    if (supportsVectorDiagram && ui->hasVectorDiagramCheckBox->isChecked())
    {
        if (!m_vectorDiagramWidget->validate())
        {
            // переключаемся на нужную вкладку
            ui->actTabWidget->setCurrentWidget(ui->measurementsTab);
            return false;
        }
    }

    // 16. Проверка ввода параметров трансформаторов тока
    // для акта установки ТТ
    if (actTypeId == 6)
    {
        if (!validateCurrentTransformers(
            m_installedCurrentTransformerWidgets, "Установленные трансформаторы тока"))
        {
            ui->actTabWidget->setCurrentWidget(ui->metersTab);
            return false;
        }
    }
    // для акта замены ТТ
    if (actTypeId == 7)
    {
        if (!validateCurrentTransformers(
            m_removedCurrentTransformerWidgets, "Снятые трансформаторы тока"))
        {
            ui->actTabWidget->setCurrentWidget(ui->metersTab);
            return false;
        }

        if (!validateCurrentTransformers(
            m_installedCurrentTransformerWidgets, "Установленные трансформаторы тока"))
        {
            ui->actTabWidget->setCurrentWidget(ui->metersTab);
            return false;
        }
        // проверяем на различие снятых и установленных ТТ
        if (!validateCurrentTransformerReplacement())
        {
            ui->actTabWidget->setCurrentWidget(ui->metersTab);
            return false;
        }
    }

    // 17. Проверяем правильно выбранный характер работ
    const bool requiresWorkSchedule =
        actTypeId == 1 ||
        actTypeId == 2 ||
        actTypeId == 7;

    if (requiresWorkSchedule &&
        ui->workScheduleComboBox->currentData().toInt() == 0)
    {
        QMessageBox::warning(this, "Ошибка",
            "Выберите характер работ.");
        ui->actTabWidget->setCurrentWidget(ui->mainTab);
        ui->workScheduleComboBox->setFocus();
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
    // пробуем записать представителей
    if (!insertExternalRepresentatives(actId))
    {
        m_database.rollback();
        return false;
    }
    // сохраняем первый прибор
    int primaryActMeterId = insertActMeter(actId, m_primaryMeterWidget, primaryMeterRole());
    if (primaryActMeterId < 0)
    {
        m_database.rollback();
        return false;
    }
    // и его показания
    if (!insertReadings(primaryActMeterId, m_primaryMeterWidget))
    {
        m_database.rollback();
        return false;
    }
    // пробуем обновить год поверки прибора учета
    if (!updateMeterVerificationYear(m_primaryMeterWidget))
    {
        m_database.rollback();
        return false;
    }
    // проверяем на предмет наличия замены и сохраняем при наличии
    int actTypeId = ui->actTypeComboBox->currentData().toInt();
    if (actTypeId == 2)
    {
        int secondaryActMeterId = insertActMeter(actId, m_secondaryMeterWidget, InstalledMeter);
        if (secondaryActMeterId < 0)
        {
            m_database.rollback();
            return false;
        }
        if (!insertReadings(secondaryActMeterId, m_secondaryMeterWidget))
        {
            m_database.rollback();
            return false;
        }
        if (!updateMeterVerificationYear(m_secondaryMeterWidget))
        {
            m_database.rollback();
            return false;
        }
    }

    // пробуем записать трансформаторы тока
    if (actTypeId == 6)
    {
        if (!insertActCurrentTransformers(
            actId,
            m_installedCurrentTransformerWidgets, InstalledCurrentTransformer))
        {
            m_database.rollback();
            return false;
        }
    }
    if (actTypeId == 7)
    {
        // снятые ТТ
        if (!insertActCurrentTransformers(
            actId,
            m_removedCurrentTransformerWidgets, RemovedCurrentTransformer))
        {
            m_database.rollback();
            return false;
        }
        // установленные ТТ
        for (CurrentTransformerActWidget* transformer
            : m_installedCurrentTransformerWidgets)
        {
            if (!insertActCurrentTransformer(
                actId, transformer, InstalledCurrentTransformer))
            {
                m_database.rollback();
                return false;
            }
        }
    }

    // пробуем записать векторную диаграмму
    if (!insertVectorDiagram(actId))
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
    // очищаем форму
    clearForm();

    //------------------------------------------------
    // Делаем проверку полученных данных из БД
    //------------------------------------------------
    ActRepository repository(m_database);
    ActData data;

    if (repository.loadAct(actId, data))
    {
        qDebug() << "===== CHECK ACT REPOSITORY =====";
        qDebug() << "ID: " << data.id;
        qDebug() << "Type ID: " << data.actTypeId;
        qDebug() << "Type: " << data.actTypeName;
        qDebug() << "Date: " << data.date;

        qDebug() << "Area: " << data.areaName;
        qDebug() << "SubSt: " << data.substationName;
        qDebug() << "Connection: " << data.connectionName;
        qDebug() << "Voltage: " << data.voltage;
        qDebug() << "CtRatio: " << data.ctRatio;

        qDebug() << "Employee: " << data.employeeName;
        qDebug() << "Position: " << data.employeePosition;

        qDebug() << "Seal: " << data.sealNumber;
        qDebug() << "Reason: " << data.reason;
        qDebug() << "Result: " << data.result;
        qDebug() << "Duration: " << data.replacementDurationMinutes;

        qDebug() << "External representatives: " <<
            data.externalRepresentatives.size();

        for (const ExternalRepresentativeData &representative :
            data.externalRepresentatives)
        {
            qDebug() << "-----------------------------";
            qDebug() << "Organization: " << representative.organization;
            qDebug() << "Name: " << representative.shortName;
            qDebug() << "Position: " << representative.position;
        }

        qDebug() << "Meters count: " << data.meters.size();

        for (const ActMeterData &meter : data.meters)
        {
            qDebug() << "------------------------------";
            qDebug() << "Meter id: " << meter.meterId;
            qDebug() << "Role: " << meter.role;
            qDebug() << "Serial: " << meter.serialNumber;
            qDebug() << "Accuracy: " << meter.accuracyClass;
            qDebug() << "Verification year: " << meter.verificationYear;

            qDebug() << "Readings: " << meter.readings.size();

            for (const ActMeterReadingData &reading : meter.readings)
            {
                qDebug()
                    << "  "
                    << reading.code
                    << "="
                    << reading.value
                    << "(type id: "
                    << reading.typeId
                    << ")";
            }
        }

        qDebug() << "Current transformers: " << data.currentTransformers.size();

        for (const ActCurrentTransformerData &transformer :
            data.currentTransformers)
        {
            qDebug() << "------------------------------";
            qDebug() << "CT id: " << transformer.currentTransformerId;
            qDebug() << "Role: " << transformer.role;
            qDebug() << "Phase: " << transformer.phase;
            qDebug() << "Name: " << transformer.name;
            qDebug() << "Serial: " << transformer.serialNumber;
            qDebug() << "Ratio: " << transformer.transformationRatio;
            qDebug() << "Accuracy: " << transformer.accuracyClass;
        }

        qDebug() << "Vector diagram exists: " << data.vectorDiagram.exists;

        if (data.vectorDiagram.exists)
        {
            qDebug() << "Ia: " << data.vectorDiagram.ia;
            qDebug() << "Angle A: " << data.vectorDiagram.angleA;
            qDebug() << "Type A: " << data.vectorDiagram.angleAType;

            qDebug() << "Ib: " << data.vectorDiagram.ib;
            qDebug() << "Angle B: " << data.vectorDiagram.angleB;
            qDebug() << "Type B: " << data.vectorDiagram.angleBType;

            qDebug() << "Ic: " << data.vectorDiagram.ic;
            qDebug() << "Angle C: " << data.vectorDiagram.angleC;
            qDebug() << "Type C: " << data.vectorDiagram.angleCType;

            qDebug() << "Uab: " << data.vectorDiagram.uab;
            qDebug() << "Ubc: " << data.vectorDiagram.ubc;
            qDebug() << "Uca: " << data.vectorDiagram.uca;
        }

        qDebug() << "====================================";
    } else
    {
        qDebug() << "Ошибка загрузки акта: " << repository.lastError();
    }
    //-------------------------------------------------
    //-------------------------------------------------

    return true;
}

// метод сохранения самого акта
int CreateActWidget::insertAct()
{
    // получаем необходимые данные с формы ввода
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();
    QVariant workScheduleType;
    if (actTypeId == 1 || actTypeId == 2 || actTypeId == 7)
    {
        workScheduleType = ui->workScheduleComboBox->currentData().toInt();
    }
    const int connectionId = ui->connectionComboBox->currentData().toInt();
    const int employeeId = ui->employeeComboBox->currentData().toInt();
    const QString actDate = ui->actDateEdit->date().toString(Qt::ISODate);
    const QString reason = ui->reasonPlainTextEdit->toPlainText().trimmed();
    const QString result = ui->resultPlainTextEdit->toPlainText().trimmed();
    QString sealNumber;
    if (ui->sealWidget->isVisible())
    {
        sealNumber = ui->sealLineEdit->text().trimmed();
    }
    // Получаем время замены прибора учета
    QVariant replacementDuration;

    if (actTypeId == 2 && ui->hasVectorDiagramCheckBox->isChecked())
    {
        replacementDuration = ui->replacementDurationSpinBox->value();
    }

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
        "result, "
        "seal_number, "
        "replacement_duration_minutes, "
        "work_schedule_type"
        ") "
        "VALUES ("
        ":actTypeId, "
        ":actDate, "
        ":connectionId, "
        ":employeeId, "
        ":employeeName, "
        ":employeePosition, "
        ":reason, "
        ":result, "
        ":sealNumber,"
        ":replacementDuration, "
        ":workScheduleType"
        ");");

    query.bindValue(":actTypeId", actTypeId);
    query.bindValue(":actDate", actDate);
    query.bindValue(":connectionId", connectionId);
    query.bindValue(":employeeId", employeeId);
    query.bindValue(":employeeName", employeeName);
    query.bindValue(":employeePosition", employeePosition);
    query.bindValue(":reason", reason);
    query.bindValue(":result", result);
    query.bindValue(":sealNumber", sealNumber);
    query.bindValue(":replacementDuration", replacementDuration);
    query.bindValue(":workScheduleType", workScheduleType);

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
int CreateActWidget::insertActMeter(
    const int actId, const MeterActWidget* meterWidget, const int role)
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
    query.bindValue(":serialNumber", meterWidget->serialNumber());
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
    int verificationYear = meterWidget->verificationYear();
    // обновляем значение в БД
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE meters "
        "SET verification_year = :verificationYear "
        "WHERE id = :meterId;");

    query.bindValue(":verificationYear", verificationYear);

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

// метод показа/скрытия поля для второго прибора учета
void CreateActWidget::updateActTypeUi()
{
    // Если тип акта не выбран
    if (!ui->actTypeComboBox->currentData().isValid())
    {
        ui->primaryMeterGroupBox->setTitle("Прибор учета");
        ui->secondaryMeterGroupBox->setVisible(false);
        // скрываем поле пломбы пока не выберется тип акта
        ui->sealWidget->setVisible(false);
        // скрываем блоки ТТ
        ui->removedCurrentTransformersGroupBox->setVisible(false);
        ui->installedCurrentTransformersGroupBox->setVisible(false);
        // Обновляем отображение полей векторной диаграммы
        updateVectorDiagramUi();
        return;
    }
    // получаем id выбранного акта
    int actTypeId = ui->actTypeComboBox->currentData().toInt();
    // Отображаем поле пломбы везде кроме демонтажа
    ui->sealWidget->setVisible(actTypeId != 4);
    // скрываем блоки ТТ
    ui->removedCurrentTransformersGroupBox->setVisible(false);
    ui->installedCurrentTransformersGroupBox->setVisible(false);

    // Настраиваем отображение поля характера работ
    // переменная выбора типов актов в котором нужен характер работ
    const bool showWorkSchedule =
        actTypeId == 1 ||
        actTypeId == 2 ||
        actTypeId == 7;
    // устанавливаем видимость в зависимости от типа акта
    ui->workScheduleWidget->setVisible(showWorkSchedule);
    // в случае выбора акта с не нужным характером работ сбрасываем на 0
    if (!showWorkSchedule)
    {
        ui->workScheduleComboBox->setCurrentIndex(0);
    }

    // отображаем окна и подписи согласно выбранному типу акта
    switch (actTypeId)
    {
        case 1:     // проверка
        {
            ui->primaryMeterGroupBox->setTitle("Проверяемый прибор");
            ui->secondaryMeterGroupBox->setVisible(false);
            break;
        }
        case 2:     // Замена
        {
            ui->primaryMeterGroupBox->setTitle("Снимаемый прибор");
            ui->secondaryMeterGroupBox->setTitle("Устанавливаемый прибор");
            ui->secondaryMeterGroupBox->setVisible(true);
            break;
        }
        case 3:     // Снятие показаний
        {
            ui->primaryMeterGroupBox->setTitle("Прибор учета");
            ui->secondaryMeterGroupBox->setVisible(false);
            break;
        }
        case 4:     // Демонтаж
        {
            ui->primaryMeterGroupBox->setTitle("Демонтируемый прибор");
            ui->secondaryMeterGroupBox->setVisible(false);
            break;
        }
        case 5:     // Установка
        {
            ui->primaryMeterGroupBox->setTitle("Устанавливаемый прибор");
            ui->secondaryMeterGroupBox->setVisible(false);
            break;
        }
        case 6:     // Установка прибора учета и ТТ
        {
            ui->primaryMeterGroupBox->setTitle("Устанавливаемый прибор");
            ui->secondaryMeterGroupBox->setVisible(false);
            // поля для ввода трансформаторов тока
            ui->removedCurrentTransformersGroupBox->setVisible(false);
            ui->installedCurrentTransformersGroupBox->setVisible(true);
            // сразу добавляем окно для ввода данных
            if (m_installedCurrentTransformerWidgets.isEmpty())
            {
                addInstalledCurrentTransformer();
            }
            break;
        }
        case 7:     // Замена ТТ
        {
            ui->primaryMeterGroupBox->setTitle("Прибор учета");
            ui->secondaryMeterGroupBox->setVisible(false);
            // поля для ввода трансформаторов тока
            ui->removedCurrentTransformersGroupBox->setVisible(true);
            ui->installedCurrentTransformersGroupBox->setVisible(true);
            // сразу добавляем по одному полю для вода данных
            if (m_removedCurrentTransformerWidgets.isEmpty())
            {
                addRemovedCurrentTransformer();
            }
            if (m_installedCurrentTransformerWidgets.isEmpty())
            {
                addInstalledCurrentTransformer();
            }
            break;
        }
        default:
        {
            ui->primaryMeterGroupBox->setTitle("Прибор учета");
            ui->secondaryMeterGroupBox->setVisible(false);
            break;
        }
    }
    // Обновляем отображение полей векторной диаграммы
    updateVectorDiagramUi();
}

// метод определения роли первого прибора
int CreateActWidget::primaryMeterRole() const
{
    // определяем тип акта
    // возвращаем номер роли согласно выбранному акту
    switch (int actTypeId = ui->actTypeComboBox->currentData().toInt())
    {
        case 1:     // Проверка
            return CheckedMeter;
        case 2:     // Замена
            return RemovedMeter;
        case 3:     // Снятие показаний
            return ReadingMeter;
        case 4:     // Демонтаж
            return RemovedMeter;
        case 5:     // Установка
            return InstalledMeter;
        case 6:     // Установка прибора и ТТ
            return InstalledMeter;
        case 7:     // Замена ТТ
            return ExistingMeter;
        default:
            return -1;
    }
}

// метод добавления полей стороннего представителя
void CreateActWidget::addExternalRepresentative()
{
    // создаем новый виджет представителя
    auto *representative = new ExternalRepresentativeWidget(ui->externalRepresentativesContainer);

    // добавляем его в контейнер
    ui->externalRepresentativesContainer->layout()->addWidget(representative);

    // запоминаем указатель
    m_externalRepresentativeWidgets.append(representative);
    // Сигнал на удаление полей
    connect(representative, &ExternalRepresentativeWidget::removeRequested, this,
        [this, representative]()
        {
            removeExternalRepresentative(representative);
        });
}

// метод удаления созданного поля добавления представителя
void CreateActWidget::removeExternalRepresentative(ExternalRepresentativeWidget *representative)
{
    // проверяем отсутствие полей
    if (!representative)
        return;

    // удаляем указатель из списка
    m_externalRepresentativeWidgets.removeOne(representative);

    // убираем виджет из поля
    ui->externalRepresentativesContainer->layout()->removeWidget(representative);

    // удаляем сами поля
    representative->deleteLater();

    // Заставляем layout пересчитать размеры
    ui->externalRepresentativesContainer->layout()->invalidate();

}

// метод сохранения сторонних представителей в базу данных
bool CreateActWidget::insertExternalRepresentatives(int actId)
{
    // проверяем на заполненность данными
    if (m_externalRepresentativeWidgets.isEmpty())
        return true;

    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "INSERT INTO external_representatives ("
        "act_id, "
        "organization, "
        "short_name, "
        "position"
        ")"
        "VALUES ("
        ":actId, "
        ":organization, "
        ":shortName, "
        ":position"
        ");");

    // пробегаемся по каждому созданному представителю
    for (ExternalRepresentativeWidget* representative :
        m_externalRepresentativeWidgets)
    {
        // биндим значения в запрос
        query.bindValue(":actId", actId);
        query.bindValue(":organization", representative->organization());
        query.bindValue(":shortName", representative->shortName());
        query.bindValue(":position", representative->position());
        // пытаемся выполнить запрос
        if (!query.exec())
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось сохранить стороннего представителя: "
                + query.lastError().text());

            return false;
        }
    }
    return true;
}

// метод управления блоком векторной диаграммы
void CreateActWidget::updateVectorDiagramUi()
{
    // Проверяем на наличие действительного значения
    if (!ui->actTypeComboBox->currentData().isValid())
    {
        // устанавливаем видимость
        ui->vectorDiagramGroupBox->setVisible(false);
        return;
    }
    // получаем id типа акта
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();
    // булева переменная состояние необходимости отображения поля
    const bool supportsVectorDiagram =
            actTypeId == 1 ||   // Проверка
            actTypeId == 2 ||   // Замена
            actTypeId == 5 ||   // Установка
            actTypeId == 6 ||   // Установка прибора и ТТ
            actTypeId == 7;     // Замена ТТ
    // устанавливаем видимость согласно переменной
    ui->vectorDiagramGroupBox->setVisible(supportsVectorDiagram);
    // меняем видимость в случае изменения переменной
    if (!supportsVectorDiagram)
    {
        ui->vectorDiagramContainerWidget->setVisible(false);
        ui->replacementDurationWidget->setVisible(false);
        return;
    }
    // булева переменная выбора наличия векторной диаграммы
    const bool hasVectorDiagram = ui->hasVectorDiagramCheckBox->isChecked();
    // устанавливаем видимость
    ui->vectorDiagramContainerWidget->setVisible(hasVectorDiagram);
    // переменная показа времени замены прибора
    const bool showReplacementDuration = actTypeId == 2 && hasVectorDiagram;
    // меняем видимость
    ui->replacementDurationWidget->setVisible(showReplacementDuration);


}

// Метод сохранения векторной диаграммы
bool CreateActWidget::insertVectorDiagram(int actId)
{
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();

    const bool supportsVectorDiagram =
        actTypeId == 1 ||
        actTypeId == 2 ||
        actTypeId == 5 ||
        actTypeId == 6 ||
        actTypeId == 7;

    // Отключаем запись диаграммы для выбранных типов актов
    if (!supportsVectorDiagram)
        return true;

    // Если диаграмма не была снята
    if (!ui->hasVectorDiagramCheckBox->isChecked())
        return true;

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO vector_diagrams "
        "("
        "act_id, "
        "ia, angle_a, angle_a_type, "
        "ib, angle_b, angle_b_type, "
        "ic, angle_c, angle_c_type, "
        "uab, ubc, uca"
        ") "
        "VALUES ("
        ":actId,"
        ":ia, :angleA, :angleAType, "
        ":ib, :angleB, :angleBType, "
        ":ic, :angleC, :angleCType, "
        ":uab, :ubc, :uca);");

    query.bindValue(":actId", actId);

    query.bindValue(":ia", m_vectorDiagramWidget->currentA());
    query.bindValue(":angleA", m_vectorDiagramWidget->angleA());
    query.bindValue(":angleAType", m_vectorDiagramWidget->angleAType());

    query.bindValue(":ib", m_vectorDiagramWidget->currentB());
    query.bindValue(":angleB", m_vectorDiagramWidget->angleB());
    query.bindValue(":angleBType", m_vectorDiagramWidget->angleBType());

    query.bindValue(":ic", m_vectorDiagramWidget->currentC());
    query.bindValue(":angleC", m_vectorDiagramWidget->angleC());
    query.bindValue(":angleCType", m_vectorDiagramWidget->angleCType());

    query.bindValue(":uab", m_vectorDiagramWidget->uab());
    query.bindValue(":ubc", m_vectorDiagramWidget->ubc());
    query.bindValue(":uca", m_vectorDiagramWidget->uca());

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось записать векторную диаграмму: "
            + query.lastError().text());
        return false;
    }

    return true;
}

void CreateActWidget::addRemovedCurrentTransformer()
{
    auto* transformer = new CurrentTransformerActWidget(
        m_database, ui->removedCtContainerWidget);

    ui->removedCtContainerWidget->layout()->addWidget(transformer);

    m_removedCurrentTransformerWidgets.append(transformer);

    connect(transformer, &CurrentTransformerActWidget::removeRequested, this,
        [this, transformer]()
        {
            removeRemovedCurrentTransformer(transformer);
        });
}

void CreateActWidget::addInstalledCurrentTransformer()
{
    auto* transformer = new CurrentTransformerActWidget(
        m_database, ui->installedCtContainerWidget);

    ui->installedCtContainerWidget->layout()->addWidget(transformer);

    m_installedCurrentTransformerWidgets.append(transformer);

    connect(transformer, &CurrentTransformerActWidget::removeRequested, this,
        [this, transformer]()
        {
            removeInstalledCurrentTransformer(transformer);
        });
}

void CreateActWidget::removeRemovedCurrentTransformer(CurrentTransformerActWidget *transformer)
{
    if (!transformer)
        return;

    m_removedCurrentTransformerWidgets.removeOne(transformer);

    ui->removedCtContainerWidget->layout()->removeWidget(transformer);

    transformer->deleteLater();
}

void CreateActWidget::removeInstalledCurrentTransformer(CurrentTransformerActWidget *transformer)
{
    if (!transformer)
        return;

    m_installedCurrentTransformerWidgets.removeOne(transformer);

    ui->installedCtContainerWidget->layout()->removeWidget(transformer);

    transformer->deleteLater();
}

// универсальный метод проверки заполненности формы ТТ
bool CreateActWidget::validateCurrentTransformers(const QList<CurrentTransformerActWidget *> &transformers,
    const QString &groupName)
{
    // проверяем, что добавлены поля ввода ТТ
    if (transformers.isEmpty())
    {
        QMessageBox::warning(this, "Не добавлены трансформаторы тока",
            "Добавьте хотя бы один ТТ в группу \"" + groupName + "\".");
        return false;
    }
    // создаем набор фаз
    QSet<QString> usedPhases;
    // пробегаемся по каждому блоку трансформатора
    for (CurrentTransformerActWidget *transformer : transformers)
    {
        // проверяем, что он существует
        if (!transformer)
            continue;
        // проверяем найден ли ТТ и введена ли фаза
        if (!transformer->validate())
        {
            return false;
        }
        // получаем название фазы
        const QString phase = transformer->phase();

        // проверяем на повторение фазы
        if (usedPhases.contains(phase))
        {
            QMessageBox::warning(this, "Повторение фазы",
                "В группе \"" + groupName + "\" фаза " + phase
                + " указана более одного раза.");

            return false;
        }
        // добавляем найденную фазу в набор
        usedPhases.insert(phase);
    }
    return true;
}

bool CreateActWidget::validateCurrentTransformerReplacement()
{
    QSet<int> removedTransformerIds;

    // собираем id всех снимаемых ТТ
    for (CurrentTransformerActWidget* transformer :
        m_removedCurrentTransformerWidgets)
    {
        if (!transformer)
            continue;

        removedTransformerIds.insert(transformer->currentTransformerId());
    }

    // Проверяем устанавливаемые ТТ
    for (const CurrentTransformerActWidget *transformer :
        m_installedCurrentTransformerWidgets)
    {
        if (!transformer)
            continue;

        const int transformerId = transformer->currentTransformerId();
        if (removedTransformerIds.contains(transformerId))
        {
            QMessageBox::warning(this, "Ошибка выбора трансформатора тока",
                "Трансформатор тока с заводским номером "
                + transformer->serialNumber()
                + " одновременно указан как снятый и установленный.");

            return false;
        }
    }

    return true;
}

bool CreateActWidget::insertActCurrentTransformer(
    int actId,
    CurrentTransformerActWidget *transformer,
    int role)
{
    // проверяем что существует поля для ввода
    if (!transformer)
        return false;

    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "INSERT INTO act_current_transformers ("
        "act_id, "
        "current_transformer_id, "
        "role, "
        "phase, "
        "name, "
        "serial_number, "
        "transformation_ratio, "
        "accuracy_class"
        ") "
        "VALUES ("
        ":actId, "
        ":currentTransformerId, "
        ":role, "
        ":phase, "
        ":name, "
        ":serialNumber, "
        ":transformationRatio, "
        ":accuracyClass"
        ");");

    query.bindValue(":actId", actId);

    query.bindValue(":currentTransformerId", transformer->currentTransformerId());
    query.bindValue(":role", role);
    query.bindValue(":phase", transformer->phase());
    query.bindValue(":name", transformer->name());
    query.bindValue(":serialNumber", transformer->serialNumber());
    query.bindValue(":transformationRatio", transformer->transformerRatio());
    query.bindValue(":accuracyClass", transformer->accuracyClass());

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось добавить акт трансформатора тока: "
            + query.lastError().text());

        return false;
    }

    return true;
}

bool CreateActWidget::insertActCurrentTransformers(
    int actId,
    const QList<CurrentTransformerActWidget *> &transformers,
    int role)
{
    for (CurrentTransformerActWidget* transformer : transformers)
    {
        if (!insertActCurrentTransformer(
            actId, transformer, role))
        {
            return false;
        }
    }
    return true;
}

void CreateActWidget::clearForm()
{
    // спрашиваем подтверждение очистки
    const auto answer = QMessageBox::question(this, "Очистка формы",
        "Очистить все введённые данные?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer == QMessageBox::No)
        return;

    // Очищаем все вкладки
    clearMainTab();
    clearEquipmentTab();
    clearMeasurementTab();
    clearConclusionTab();



    // обновляем вид графического окна
    updateActTypeUi();
    // возвращаемся на первую страницу
    ui->actTabWidget->setCurrentWidget(ui->mainTab);

}

// очищаем виджеты сторонних представителей
void CreateActWidget::clearExternalRepresentatives()
{
    for (ExternalRepresentativeWidget* representative :
        m_externalRepresentativeWidgets)
    {
        if (!representative)
            continue;
        ui->externalRepresentativesContainer->layout()->removeWidget(representative);

        representative->deleteLater();
    }

    m_externalRepresentativeWidgets.clear();
}

// очищаем все созданные трансформаторы тока
void CreateActWidget::clearCurrentTransformers()
{
    for (CurrentTransformerActWidget* transformer :
        m_removedCurrentTransformerWidgets)
    {
        if (!transformer)
            continue;

        ui->removedCtContainerWidget->layout()->removeWidget(transformer);
        transformer->deleteLater();
    }

    m_removedCurrentTransformerWidgets.clear();

    for (CurrentTransformerActWidget* transformer :
        m_installedCurrentTransformerWidgets)
    {
        if (!transformer)
            continue;

        ui->installedCtContainerWidget->layout()->removeWidget(transformer);
        transformer->deleteLater();
    }

    m_installedCurrentTransformerWidgets.clear();
}

// очищаем вкладку "Основное"
void CreateActWidget::clearMainTab()
{
    // основные данные
    // они же за счет работы сигналов и сбросят зависимые поля
    ui->actTypeComboBox->setCurrentIndex(0);
    ui->areaComboBox->setCurrentIndex(0);

    // характер работ
    ui->workScheduleComboBox->setCurrentIndex(0);
    ui->workScheduleWidget->setVisible(false);

    // устанавливаем текущую дату
    ui->actDateEdit->setDate(QDate::currentDate());

    // представитель предприятия
    ui->employeeComboBox->setCurrentIndex(0);

    // сторонние представители
    clearExternalRepresentatives();

}

// очищаем вкладку "Оборудование"
void CreateActWidget::clearEquipmentTab()
{
    // Приборы учета
    if (m_primaryMeterWidget)
        m_primaryMeterWidget->clear();

    if (m_secondaryMeterWidget)
        m_secondaryMeterWidget->clear();

    // Трансформаторы тока
    clearCurrentTransformers();


}

// Очищаем вкладку "Измерения"
void CreateActWidget::clearMeasurementTab()
{
    // Векторная диаграмма
    if (m_vectorDiagramWidget)
        m_vectorDiagramWidget->clear();
    ui->hasVectorDiagramCheckBox->setChecked(false);
    // длительность замены
    ui->replacementDurationSpinBox->setValue(0);
}

// Очищаем вкладку "Заключение"
void CreateActWidget::clearConclusionTab()
{
    // Пломба
    ui->sealLineEdit->clear();

    // Причина и заключение
    ui->reasonPlainTextEdit->clear();
    ui->resultPlainTextEdit->clear();

}

void CreateActWidget::loadActForEditing(int actId)
{
    ActRepository repository(m_database);

    ActData data;

    if (!repository.loadAct(actId, data))
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить акт для редактирования: "
            + repository.lastError());

        return;
    }

    qDebug() << "Editing act: " << data.id;

    // Загружаем тип акта
    const int actTypeIndex =
        ui->actTypeComboBox->findData(data.actTypeId);

    if (actTypeIndex >= 0)
    {
        ui->actTypeComboBox->setCurrentIndex(actTypeIndex);
    }

    // Загружаем дату
    ui->actDateEdit->setDate(data.date);

    // Загружаем участок
    const int areaIndex =
        ui->areaComboBox->findData(data.areaId);

    if (areaIndex >= 0)
    {
        ui->areaComboBox->setCurrentIndex(areaIndex);
    }

    // Загружаем подстанцию
    const int substationIndex =
        ui->substationComboBox->findData(data.substationId);

    if (substationIndex >= 0)
    {
        ui->substationComboBox->setCurrentIndex(substationIndex);
    }

    // Загружаем присоединение
    const int connectionIndex =
        ui->connectionComboBox->findData(data.connectionId);

    if (connectionIndex >= 0)
    {
        ui->connectionComboBox->setCurrentIndex(connectionIndex);
    }

    // Загружаем представителя
    const int employeeIndex =
        ui->employeeComboBox->findData(data.employeeId);

    if (employeeIndex >= 0)
    {
        ui->employeeComboBox->setCurrentIndex(employeeIndex);
    }

    // Загружаем характер работ для актов, где они используются
    if (data.workScheduleType > 0)
    {
        const int scheduleIndex =
            ui->workScheduleComboBox->findData(data.workScheduleType);

        if (scheduleIndex >= 0)
        {
            ui->workScheduleComboBox->setCurrentIndex(scheduleIndex);
        }
    }




}




















