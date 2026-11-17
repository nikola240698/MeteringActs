
#include <QDebug>
#include <QScrollArea>
#include <QHash>
#include <cmath>

#include "createactwidget.h"
#include "ui_createactwidget.h"
#include "actrepository.h"
#include "currenttransformerphasewidget.h"
#include "currenttransformerphasepanel.h"
#include "actrepository.h"
#include "docxservice.h"
#include "uuidutils.h"



CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);

    // Скрываем подсказку о выборе присоединений
    ui->connectionInfoLabel->hide();

    // настраиваем виджет первого прибора и показаний
    m_primaryMeterWidget = new MeterActWidget(m_database, ui->meterContainerWidget);
    // добавляем виджет
    ui->meterContainerWidget->layout()->addWidget(m_primaryMeterWidget);
    // подобно настраиваем второй виджет
    m_secondaryMeterWidget = new MeterActWidget(m_database, ui->secondMeterContainerWidget);
    ui->secondMeterContainerWidget->layout()->addWidget(m_secondaryMeterWidget);
    // отключаем по стандарту видимость второго прибора
    ui->secondaryMeterCard->setVisible(false);

    // скрываем виджет выбора нескольких приборов для снятия показаний
    ui->readingMetersContainer->hide();

    // Создаем виджеты-блоки для добавления ТТ
    m_removedCtPhasePanel = new CurrentTransformerPhasePanel(
        m_database, ui->removedCtPhaseContainer);
    ui->removedCtPhaseContainer->layout()->addWidget(
        m_removedCtPhasePanel);

    m_installedCtPhasePanel = new CurrentTransformerPhasePanel(
        m_database, ui->installedCtPhaseContainer);
    ui->installedCtPhaseContainer->layout()->addWidget(
        m_installedCtPhasePanel);

    // создаем виджет векторной диаграммы
    m_vectorDiagramWidget = new VectorDiagramWidget(ui->vectorDiagramContainerWidget);
    // добавляем поля векторной диаграммы на главный экран
    ui->vectorDiagramContainerWidget->layout()->addWidget(m_vectorDiagramWidget);
    // сразу скрываем все поля
    ui->vectorDiagramContainerWidget->setVisible(false);


    // устанавливаем сегодняшнюю дату
    ui->actDateEdit->setDate(QDate::currentDate());

    // скрываем поля для ввода данных про трансформаторы тока
    ui->removedCtCard->setVisible(false);
    ui->installedCtCard->setVisible(false);

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

            for (ReadingConnectionWidget *widget : m_readingConnectionWidgets)
            {
                widget->setSubstation(
                    data.isValid() ? data.toInt() : -1
                );
            }

            updateAvailableConnections();

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



    // сигнал нажатия кнопки сохранения
    connect(ui->createButton, &QPushButton::clicked, this,
        [this]()
        {
            if (m_mode == Mode::Create)
            {
                saveAct();
            }
            else
            {
                updateAct();
            }
        });

    // Подключаем кнопку создания акта и DOCX
    connect(ui->createAbdDocxButton, &QPushButton::clicked, this,
        [this]()
        {
            if (m_mode == Mode::Create)
            {
                saveAct(true);
            }
        });

    // кнопка очистки формы
    connect(ui->clearButton, &QPushButton::clicked, this, &CreateActWidget::clearForm);

    // Подключаем сигналы о создании акта
    connect(m_primaryMeterWidget, &MeterActWidget::meterCreated,
        this, &CreateActWidget::metersChanged);

    connect(m_secondaryMeterWidget, &MeterActWidget::meterCreated,
        this, &CreateActWidget::metersChanged);

    // Подключаем слоты подачи сигнала при создании ТТ
    connect(m_removedCtPhasePanel, &CurrentTransformerPhasePanel::currentTransformerCreated,
        this, &CreateActWidget::currentTransformersChanged);
    connect(m_installedCtPhasePanel, &CurrentTransformerPhasePanel::currentTransformerCreated,
        this, &CreateActWidget::currentTransformersChanged);

    /// Создаём первую пару карточек для снятия показаний
    auto *readingWidget = new ReadingConnectionWidget(
        m_database,
        ui->readingMetersContainer
    );

    ui->readingMetersLayout->addWidget(readingWidget);
    m_readingConnectionWidgets.append(readingWidget);

    connect(readingWidget, &ReadingConnectionWidget::meterSelected, this,
        [this](ReadingConnectionWidget *)
        {
            validateUniqueMeters();
        }
    );

    // Подключаем удаление карточки
    connect(readingWidget, &ReadingConnectionWidget::removeRequested,
            this, &CreateActWidget::removeReadingWidget);

    connect(readingWidget, &ReadingConnectionWidget::connectionChanged,
        this, [this](ReadingConnectionWidget *, int)
        {
            updateAvailableConnections();
        });

    // Обновляем номера и кнопки удаления
    updateReadingWidgets();

    // Передаём текущую подстанцию, если она уже выбрана
    const QVariant substationId = ui->substationComboBox->currentData();

    readingWidget->setSubstation(
        substationId.isValid() ? substationId.toInt() : -1
    );

    ui->readingMetersContainer->hide();

    // Обработчик кнопки добавления прибора учета
    // Добавление прибора учёта для группового снятия показаний
    connect(ui->addReadingMeterButton, &QPushButton::clicked,
            this, [this]()
            {
                auto *widget = new ReadingConnectionWidget(
                    m_database,
                    ui->readingMetersContainer
                );

                connect(widget, &ReadingConnectionWidget::meterSelected, this,
                    [this](ReadingConnectionWidget *)
                    {
                        validateUniqueMeters();
                    }
                );

                // Добавляем новую пару карточек
                ui->readingMetersLayout->addWidget(widget);
                m_readingConnectionWidgets.append(widget);

                // Подключаем удаление новой карточки
                connect(widget, &ReadingConnectionWidget::removeRequested,
                        this, &CreateActWidget::removeReadingWidget);

                connect(widget, &ReadingConnectionWidget::connectionChanged,
                        this, [this](ReadingConnectionWidget *, int)
                        {
                            updateAvailableConnections();
                        });

                // Обновляем номера и кнопки удаления
                updateReadingWidgets();

                // Загружаем присоединения выбранной подстанции
                const QVariant substationId =
                    ui->substationComboBox->currentData();

                widget->setSubstation(
                    substationId.isValid() ? substationId.toInt() : -1
                );

                updateAvailableConnections();
            });
}

CreateActWidget::CreateActWidget(Database &database, int actId, QWidget *parent) :
    CreateActWidget(database, parent)
{
    m_mode = Mode::Edit;
    m_editActId = actId;

    // Повторно загружаем сотрудников с учётом представителя,
    // сохранённого в редактируемом акте.
    loadEmployees();

    ui->createButton->setText("Сохранить изменения");
    // Прячем кнопку сохранения и формирования DOCX
    ui->createAbdDocxButton->hide();

    loadActForEditing(actId);

    // Минимальная ширина формы редактирования
    setMinimumWidth(1100);
}

CreateActWidget::~CreateActWidget()
{
    delete ui;
}

void CreateActWidget::reloadEmployees()
{
    const QVariant currentEmployeeId =
        ui->employeeComboBox->currentData();

    loadEmployees();

    if (currentEmployeeId.isValid())
    {
        const int index =
            ui->employeeComboBox->findData(currentEmployeeId);

        if (index >= 0)
            ui->employeeComboBox->setCurrentIndex(index);
    }
}

void CreateActWidget::reloadObject()
{
    const QVariant currentAreaId =
        ui->areaComboBox->currentData();

    const QVariant currentSubstationId =
        ui->substationComboBox->currentData();

    const QVariant currentConnectionId =
        ui->connectionComboBox->currentData();

    // Сначала заново загружаем участки
    loadAreas();

    // Если участок раньше не был выбран - больше восстанавливать нечего
    if (!currentAreaId.isValid())
        return;

    const int areaIndex =
        ui->areaComboBox->findData(currentAreaId);

    // Если вдруг участок удалили
    if (areaIndex < 0)
        return;

    // Восстанавливаем участок
    ui->areaComboBox->setCurrentIndex(areaIndex);

    if (!currentSubstationId.isValid())
        return;

    const int substationIndex =
        ui->substationComboBox->findData(currentSubstationId);

    // Если ПС удалили
    if (substationIndex < 0)
        return;

    // Восстанавливаем подстанцию
    ui->substationComboBox->setCurrentIndex(substationIndex);


    if (!currentConnectionId.isValid())
        return;

    const int connectionIndex =
        ui->connectionComboBox->findData(currentConnectionId);

    // Если присоединение удалено
    if (connectionIndex <0)
        return;

    // Восстанавливаем присоединение
    ui->connectionComboBox->setCurrentIndex(connectionIndex);
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

    query.prepare(
        "SELECT id, short_name, position, is_active "
        "FROM employees "
        "WHERE is_active = 1 "
        "OR id = :employeeId "
        "ORDER BY short_name"
        );

    // При редактировании разрешаем загрузить сотрудника,
    // который уже указан в акте, даже если он уволен.
    int originalEmployeeId = -1;

    if (m_mode == Mode::Edit && m_editActId >= 0)
    {
        QSqlQuery actQuery(m_database.getDatabase());

        actQuery.prepare(
            "SELECT employee_id "
            "FROM acts "
            "WHERE id = :actId;");

        actQuery.bindValue(":actId", m_editActId);

        if (!actQuery.exec() || !actQuery.next())
        {
            qDebug() << "loadEmployees act error:"
                     << actQuery.lastError().text();
            return;
        }

        originalEmployeeId = actQuery.value("employee_id").toInt();
    }

    query.bindValue(":employeeId", originalEmployeeId);
    query.bindValue(":isEditMode", m_mode == Mode::Edit ? 1 : 0);


    if (!query.exec())
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

        // Отмечаем уволенного сотрудника в списке.
        const bool isActive = query.value("is_active").toBool();

        if (!isActive)
        {
            displayText += " (уволен)";
        }

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
        "SELECT "
        "id, "
        "CASE connection_type "
            "WHEN 1 THEN "
                "'яч.' || printf('%g', voltage_kv) || "
                "'кВ №' || cell_number || ' \"' || name || '\"' "
            "WHEN 2 THEN "
                "'ВЛ-' || printf('%g', voltage_kv) || "
                "'кВ \"' || name || '\"' "
            "ELSE name "
        "END AS display_name "
        "FROM connections "
        "WHERE substation_id = :substationId "
        "ORDER BY connection_type ASC, "
        "voltage_kv DESC, cell_number ASC, "
        "name COLLATE NOCASE ASC;"
        );

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

        QString name = query.value("display_name").toString();

        ui->connectionComboBox->addItem(name, id);
    }
}

// метод загрузки данные присоединения
void CreateActWidget::loadConnectionData(int connectionId) const
{
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT voltage_kv, "
        "ct_primary_current, ct_secondary_current "
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
    const QString voltage = query.value("voltage_kv").toString();

    const int secondary =
        query.value("ct_secondary_current").toInt();

    QString ctRatio;

    if (secondary == 0)
    {
        ctRatio = "Прямого включения";
    }
    else
    {
        const int primary =
            query.value("ct_primary_current").toInt();

        ctRatio = QString("%1/%2")
                      .arg(primary)
                      .arg(secondary);
    }

    ui->voltageLineEdit->setText(voltage + " кВ");
    ui->ctRatioLineEdit->setText(ctRatio);
}

// метод проверки правильности заполнения формы
bool CreateActWidget::validateForm()
{

    // Определяем тип акта для выбора необходимых проверок
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();
    const bool isGroupReading = (actTypeId == 3);

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
    // Для группового акта присоединения выбираются в отдельных карточках
    if (!isGroupReading && !ui->connectionComboBox->currentData().isValid())
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
    // Основной прибор проверяем только для обычных актов
    if (!isGroupReading && !m_primaryMeterWidget->validate())
    {
        // переключаемся на нужную вкладку
        ui->actTabWidget->setCurrentWidget(ui->metersTab);
        return false;
    }

    // добавляем проверку введенных полей второго прибора учета, если он видим
    // в данном типе акта
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
    if (actTypeId != 4 &&
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
        if (!m_installedCtPhasePanel->validate())
            return false;
    }
    // для акта замены ТТ
    if (actTypeId == 7)
    {
        if (!m_removedCtPhasePanel->validate())
            return false;

        if (!m_installedCtPhasePanel->validate())
            return false;

        if (!validateCurrentTransformerReplacement())
            return false;
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
bool CreateActWidget::saveAct(bool generateDocument)
{

    const int actTypeId = ui->actTypeComboBox->currentData().toInt();

    if (actTypeId == 3)
    {
        if (!validateUniqueMeters())
            return false;

        if (!validateReadingConnections())
            return false;
    }

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

    if (actTypeId == 3)
    {
        // Сохраняем все карточки группового снятия показаний
        if (!insertReadingConnections(actId))
        {
            m_database.rollback();
            return false;
        }
    }
    else
    {
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
    }

    // пробуем записать трансформаторы тока
    if (actTypeId == 6)
    {
        const QList<ActCurrentTransformerData> installedTransformers =
            m_installedCtPhasePanel->data(InstalledCurrentTransformer);

        if (!insertActCurrentTransformers(
            actId,installedTransformers))
        {
            m_database.rollback();
            return false;
        }
    }
    if (actTypeId == 7)
    {
        // снятые ТТ
        const QList<ActCurrentTransformerData> removedTransformers =
            m_removedCtPhasePanel->data(RemovedCurrentTransformer);

        if (!insertActCurrentTransformers(
            actId,
            removedTransformers))
        {
            m_database.rollback();
            return false;
        }
        // установленные ТТ
        const QList<ActCurrentTransformerData> installedTransformers =
           m_installedCtPhasePanel->data(InstalledCurrentTransformer);

        if (!insertActCurrentTransformers(
            actId, installedTransformers))
        {
            m_database.rollback();
            return false;
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

    // Сообщаем остальным окнам, что появился новый акт
    emit actCreated();

    if (generateDocument)
    {
        ActRepository repository(m_database);
        ActData data;

        if (!repository.loadAct(actId, data))
        {
            QMessageBox::warning(this, "Акт сохранён",
                "Акт успешно сохранён в базе данных, "
                "но не удалось загрузить его данные для формирования DOCX.\n\n"
                "Причина: " + repository.lastError());
        }
        else
        {
            DocxService::generate(data, this);
        }
    }
    else
    {
        QMessageBox::information(this, "Акт сохранен",
            "Данные акта успешно сохранены");
    }

    // очищаем форму только после завершения всех действий
    clearForm();

    return true;
}

bool CreateActWidget::updateAct()
{

    if (m_editActId < 0)
    {
        QMessageBox::critical(this, "Ошибка",
            "Не указан акт для редактирования. ");

        return false;
    }

    if (!validateForm())
        return false;

    // Проверяем, не изменился ли акт после открытия
    if (!checkActVersion())
        return false;

    // Получаем данные акта для сохранения
    ActRepository repository(m_database);
    ActData original;

    if (!repository.loadAct(m_editActId, original))
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось загрузить акт для сравнения:\n" +
            repository.lastError());

        return false;
    }

    // Получаем данные текущего акта
    const ActData current = collectActData();

    // Для группового акта проверяем приборы и состав присоединений.
    const bool isGroupReading =
        ui->actTypeComboBox->currentData().toInt() == 3;

    if (isGroupReading)
    {
        if (!validateUniqueMeters() ||
            !validateReadingConnections() ||
            !validateOriginalReadingConnections(original))
        {
            return false;
        }
    }

    // Проверяем, изменился ли акт по сравнению с записей в БД
    const bool changed = hasActChanges(original, current);

    qDebug() << "Act changed: " << changed;

    // В случае отсутствия изменений пропускаем обновление БД
    if (!changed)
    {
        qDebug() << "Act has no changes. Skipping database update.";

        // Отправляем сигнал о завершении обновления
        emit actUpdated();
        return true;
    }


    if (!m_database.transaction())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось начать транзакцию: " + m_database.lastError());

        return false;
    }

    // Обновляем основную запись акта
    if (!updateActMainData())
    {
        m_database.rollback();
        return false;
    }

    if (isGroupReading)
    {
        // Обновляем приборы и показания без удаления записей.
        if (!updateReadingConnections(original))
        {
            m_database.rollback();
            return false;
        }

    }
    else
    {
        // Удаляем старые дочерние данные
        if (!deleteActDetails())
        {
            m_database.rollback();
            return false;
        }



        // Основной прибор
        const int primaryActMeterId =
            insertActMeter(m_editActId, m_primaryMeterWidget, primaryMeterRole());

        if (primaryActMeterId < 0)
        {
            m_database.rollback();
            return false;
        }

        if (!insertReadings(primaryActMeterId, m_primaryMeterWidget))
        {
            m_database.rollback();
            return false;
        }

        if (!updateMeterVerificationYear(m_primaryMeterWidget))
        {
            m_database.rollback();
            return false;
        }

        const int actTypeId =
            ui->actTypeComboBox->currentData().toInt();

        // Второй прибор при замене
        if (actTypeId == 2)
        {
            const int secondaryActMeterId =
                insertActMeter(m_editActId, m_secondaryMeterWidget, InstalledMeter);

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
        }

        // ТТ - установка
        if (actTypeId == 6)
        {
            const QList<ActCurrentTransformerData> installedTransformers =
                m_installedCtPhasePanel->data(InstalledCurrentTransformer);

            if (!insertActCurrentTransformers(
                m_editActId, installedTransformers))
            {
                m_database.rollback();
                return false;
            }
        }

        // ТТ - замена
        if (actTypeId == 7)
        {
            const QList <ActCurrentTransformerData> removedTransformers =
                m_removedCtPhasePanel->data(RemovedCurrentTransformer);

            if (!insertActCurrentTransformers(
                m_editActId, removedTransformers))
            {
                m_database.rollback();
                return false;
            }

            const QList<ActCurrentTransformerData> installedTransformers =
                m_installedCtPhasePanel->data(InstalledCurrentTransformer);

            if (!insertActCurrentTransformers(
                m_editActId, installedTransformers))
            {
                m_database.rollback();
                return false;
            }
        }
    }

    // Обновляем сторонних представителей независимо от типа акта.
    if (!updateExternalRepresentatives(original))
    {
        m_database.rollback();
        return false;
    }

    // Векторная диграмма
    if (!insertVectorDiagram(m_editActId))
    {
        m_database.rollback();
        return false;
    }

    if (!m_database.commit())
    {
        const QString error = m_database.lastError();

        m_database.rollback();

        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось завершить транзакцию: " + error);

        return false;
    }

    // Транзакция успешно завершена
    QMessageBox::information(this, "Акт обновлён",
        "Изменения успешно сохранены.");

    // Уведомляем остальные окна только после успешного сохранения
    emit actUpdated();

    return true;
}



bool CreateActWidget::updateActMainData()
{
    const int actTypeId = ui->actTypeComboBox->currentData().toInt();

    // Для группового акта сохраняем исходное общее присоединение.
    // Для остальных типов используем выбор из основной формы.
    QVariant connectionId;

    if (actTypeId == 3)
    {
        QSqlQuery connectionQuery(m_database.getDatabase());

        connectionQuery.prepare(
            "SELECT connection_id "
            "FROM acts "
            "WHERE id = :actId;");

        connectionQuery.bindValue(":actId", m_editActId);

        if (!connectionQuery.exec() || !connectionQuery.next())
        {
            QMessageBox::critical(
                this,
                "Ошибка базы данных",
                "Не удалось получить присоединение акта: "
                + connectionQuery.lastError().text());

            return false;
        }

        connectionId = connectionQuery.value("connection_id");
    }
    else
    {
        connectionId = ui->connectionComboBox->currentData();
    }

    const int substationId = ui->substationComboBox->currentData().toInt();

    const int employeeId = ui->employeeComboBox->currentData().toInt();

    const QString actDate = ui->actDateEdit->date().toString(Qt::ISODate);

    const QString reason = ui->reasonPlainTextEdit->toPlainText().trimmed();

    const QString result = ui->resultPlainTextEdit->toPlainText().trimmed();

    QVariant workScheduleType;

    if (actTypeId == 1 ||
        actTypeId == 2 ||
        actTypeId == 7)
    {
        workScheduleType = ui->workScheduleComboBox->currentData().toInt();
    }

    QVariant sealNumber;

    if (actTypeId != 4)
    {
        sealNumber = ui->sealLineEdit->text().trimmed();
    }

    QVariant replacementDuration;

    if (actTypeId == 2 && ui->hasVectorDiagramCheckBox->isChecked())
    {
        replacementDuration = ui->replacementDurationSpinBox->value();
    }

    // Получаем исходные данные представителя из самого акта.
    // Они сохраняют ФИО и должность на момент оформления документа.
    QSqlQuery originalEmployeeQuery(m_database.getDatabase());

    originalEmployeeQuery.prepare(
        "SELECT employee_id, employee_name, employee_position "
        "FROM acts "
        "WHERE id = :actId;");

    originalEmployeeQuery.bindValue(":actId", m_editActId);

    if (!originalEmployeeQuery.exec() || !originalEmployeeQuery.next())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось получить исходные данные представителя:\n"
            + originalEmployeeQuery.lastError().text());

        return false;
    }

    const int originalEmployeeId =
        originalEmployeeQuery.value("employee_id").toInt();

    const QString originalEmployeeName =
        originalEmployeeQuery.value("employee_name").toString();

    const QString originalEmployeePosition =
        originalEmployeeQuery.value("employee_position").toString();

    // Получаем актуальные данные представителя
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

        return false;
    }

    // Проверяем, изменился ли представитель предприятия.
    const bool employeeChanged = (employeeId != originalEmployeeId);

    // Если представитель не изменился, сохраняем исторические данные.
    // При выборе другого сотрудника используем его актуальные сведения.
    const QString employeeName = employeeChanged
        ? employeeQuery.value("short_name").toString()
        : originalEmployeeName;

    const QString employeePosition = employeeChanged
        ? employeeQuery.value("position").toString()
        : originalEmployeePosition;

    // Перед сохранением акта проверим, изменилось ли присоединение
    // которое выбрано сейчас, и которе мы выбрали
    QSqlQuery oldConnectionQuery(m_database.getDatabase());

    oldConnectionQuery.prepare(
        "SELECT connection_id "
        "FROM acts "
        "WHERE id = :actId;");

    oldConnectionQuery.bindValue(":actId", m_editActId);

    if (!oldConnectionQuery.exec() || !oldConnectionQuery.next())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось получить текущее присоединение акта: "
            + oldConnectionQuery.lastError().text());

        return false;
    }

    const QVariant oldConnectionId =
     oldConnectionQuery.value("connection_id");

    // Для группового акта состав присоединений не изменяется.
    // Для обычных актов сохраняем существующую проверку.
    const bool connectionChanged =
        actTypeId != 3 && oldConnectionId != connectionId;

    // Обновляем наш акт
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE acts SET "
        "version_uuid = :versionUuid, "
        "act_type_id = :actTypeId, "
        "act_date = :actDate, "
        "substation_id = :substationId, "
        "connection_id = :connectionId, "
        "employee_id = :employeeId, "
        "employee_name = :employeeName, "
        "employee_position = :employeePosition, "
        "reason = :reason, "
        "result = :result, "
        "seal_number = :sealNumber, "
        "replacement_duration_minutes = :replacementDuration, "
        "work_schedule_type = :workScheduleType "
        "WHERE id = :actId "
        "AND version_uuid = :originalVersionUuid;");

    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":actTypeId", actTypeId);
    query.bindValue(":actDate", actDate);
    query.bindValue(":substationId", substationId);
    query.bindValue(":connectionId", connectionId);

    query.bindValue(":employeeId", employeeId);
    query.bindValue(":employeeName", employeeName);
    query.bindValue(":employeePosition", employeePosition);

    query.bindValue(":reason", reason);
    query.bindValue(":result", result);
    query.bindValue(":sealNumber", sealNumber);

    query.bindValue(":replacementDuration", replacementDuration);

    query.bindValue(":workScheduleType", workScheduleType);

    query.bindValue(":actId", m_editActId);
    query.bindValue(":originalVersionUuid", m_originalVersionUuid);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось обновить акт: " + query.lastError().text());

        return false;
    }

    // Проверяем, что акт не был изменен кем-то во время наших изменений
    if (query.numRowsAffected() != 1)
    {
        QMessageBox::warning(this, "Конфликт версий",
            "Акт был изменён или удалён другим экземпляром программы.\n\n"
            "Сохранение отменено. Закройте окно и откройте акт заново.");

        return false;
    }

    // В случае изменения присоединения в акте так же меняем
    // данные в снапшоте БД
    if (connectionChanged)
    {
        QSqlQuery connectionQuery(m_database.getDatabase());

        connectionQuery.prepare(
            "SELECT "
            "ar.name AS area_name, "
            "s.name AS substation_name, "
            "CASE c.connection_type "
                "WHEN 1 THEN "
                    "'яч.' || printf('%g', c.voltage_kv) || "
                    "'кВ №' || c.cell_number || ' \"' || c.name || '\"' "
                "WHEN 2 THEN "
                    "'ВЛ-' || printf('%g', c.voltage_kv) || "
                    "'кВ \"' || c.name || '\"' "
                "ELSE c.name "
            "END AS connection_name, "
            "c.voltage_kv, "
            "CASE "
                "WHEN c.ct_secondary_current = 0 THEN 'Прямого включения' "
                "ELSE CAST(c.ct_primary_current AS TEXT) || '/' || "
                     "CAST(c.ct_secondary_current AS TEXT) "
            "END AS ct_ratio "
            "FROM connections c "
            "JOIN substations s ON s.id = c.substation_id "
            "JOIN areas ar ON ar.id = s.area_id "
            "WHERE c.id = :connectionId;"
        );

        connectionQuery.bindValue(":connectionId", connectionId);

        if (!connectionQuery.exec() || !connectionQuery.next())
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось получить данные нового присоединения: "
                + connectionQuery.lastError().text());

            return false;
        }

        QSqlQuery snapshotQuery(m_database.getDatabase());

        snapshotQuery.prepare(
            "UPDATE acts SET "
            "area_name = :areaName, "
            "substation_name = :substationName, "
            "connection_name = :connectionName, "
            "connection_voltage_kv = :connectionVoltage, "
            "connection_ct_ratio = :connectionCtRatio "
            "WHERE id = :actId;");

        snapshotQuery.bindValue(
            ":areaName", connectionQuery.value("area_name"));

        snapshotQuery.bindValue(
            ":substationName", connectionQuery.value("substation_name"));

        snapshotQuery.bindValue(
            ":connectionName", connectionQuery.value("connection_name"));

        snapshotQuery.bindValue(
            ":connectionVoltage", connectionQuery.value("voltage_kv"));

        snapshotQuery.bindValue(
            ":connectionCtRatio", connectionQuery.value("ct_ratio"));

        snapshotQuery.bindValue(":actId", m_editActId);

        if (!snapshotQuery.exec())
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось обновить список присоединений: "
                + snapshotQuery.lastError().text());

            return false;
        }
    }

    return true;
}

bool CreateActWidget::deleteActDetails()
{
    QSqlQuery query(m_database.getDatabase());

    // Приборы учета
    query.prepare(
        "DELETE FROM act_meters "
        "WHERE act_id = :actId;");

    query.bindValue(":actId", m_editActId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось удалить старые данные приборов учета: "
            + query.lastError().text());

        return false;
    }

    // Трансформаторы тока
    query.prepare(
        "DELETE FROM act_current_transformers "
        "WHERE act_id = :actId;");

    query.bindValue(":actId", m_editActId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось удалить старые данные ТТ: " + query.lastError().text());

        return false;
    }

    // Сторонние представители
    query.prepare(
        "DELETE FROM external_representatives "
        "WHERE act_id = :actId;");

    query.bindValue(":actId", m_editActId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось удалить старых представителей: "
            + query.lastError().text());

        return false;
    }

    // Векторная диаграмма
    query.prepare(
        "DELETE FROM vector_diagrams "
        "WHERE act_id = :actId;");

    query.bindValue(":actId", m_editActId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка баз данных",
            "Не удалось удалить старую векторную диаграмму: "
            + query.lastError().text());

        return false;
    }

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

    // Для снятия показаний с одного присоединения
    // сохраняем его ID в основной записи акта.
    QVariant connectionId;

    if (actTypeId == 3)
    {
        if (m_readingConnectionWidgets.size() == 1)
        {
            connectionId =
                m_readingConnectionWidgets.first()->selectedConnectionId();
        }
    }
    else
    {
        connectionId = ui->connectionComboBox->currentData();
    }

    const int substationId = ui->substationComboBox->currentData().toInt();
    const int employeeId = ui->employeeComboBox->currentData().toInt();
    const QString actDate = ui->actDateEdit->date().toString(Qt::ISODate);
    const QString reason = ui->reasonPlainTextEdit->toPlainText().trimmed();
    const QString result = ui->resultPlainTextEdit->toPlainText().trimmed();
    QString sealNumber;
    if (actTypeId != 4)
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

    // Получаем данные места проведения работ для исторического снимка
    // Подготавливаем исторические сведения о месте проведения работ
    QString areaName;
    QString substationName;
    QVariant connectionName;
    QVariant connectionVoltage;
    QVariant connectionCtRatio;

    QSqlQuery locationQuery(m_database.getDatabase());

    if (actTypeId == 3 && connectionId.isNull())
    {
        // Групповой акт: получаем только участок и подстанцию
        locationQuery.prepare(
            "SELECT "
            "ar.name AS area_name, "
            "s.name AS substation_name "
            "FROM substations s "
            "JOIN areas ar ON ar.id = s.area_id "
            "WHERE s.id = :substationId;");

        locationQuery.bindValue(":substationId", substationId);
    }
    else
    {
        // Обычный акт: получаем также сведения об одном присоединении
        locationQuery.prepare(
            "SELECT "
            "ar.name AS area_name, "
            "s.name AS substation_name, "
            "CASE c.connection_type "
                "WHEN 1 THEN "
                    "'яч.' || printf('%g', c.voltage_kv) || "
                    "'кВ №' || c.cell_number || ' \"' || c.name || '\"' "
                "WHEN 2 THEN "
                    "'ВЛ-' || printf('%g', c.voltage_kv) || "
                    "'кВ \"' || c.name || '\"' "
                "ELSE c.name "
            "END AS connection_name, "
            "c.voltage_kv, "
            "CASE "
                "WHEN c.ct_secondary_current = 0 THEN 'Прямого включения' "
                "ELSE CAST(c.ct_primary_current AS TEXT) || '/' || "
                     "CAST(c.ct_secondary_current AS TEXT) "
            "END AS ct_ratio "
            "FROM connections c "
            "JOIN substations s ON s.id = c.substation_id "
            "JOIN areas ar ON ar.id = s.area_id "
            "WHERE c.id = :connectionId "
            "AND s.id = :substationId;");

        locationQuery.bindValue(":connectionId", connectionId);
        locationQuery.bindValue(":substationId", substationId);
    }

    // Выполняем запрос и проверяем наличие выбранного объекта
    if (!locationQuery.exec() || !locationQuery.next())
    {
        QMessageBox::critical(
            this,
            "Ошибка базы данных",
            "Не удалось получить сведения о месте проведения работ:\n"
            + locationQuery.lastError().text());

        return -1;
    }

    // Сохраняем названия для исторического снимка акта
    areaName = locationQuery.value("area_name").toString();
    substationName = locationQuery.value("substation_name").toString();

    if (!connectionId.isNull())
    {
        // Для обычного акта сохраняем параметры присоединения
        connectionName = locationQuery.value("connection_name");
        connectionVoltage = locationQuery.value("voltage_kv");
        connectionCtRatio = locationQuery.value("ct_ratio");
    }

    // вносим данные в базу данных
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO acts ("
            "uuid, "
            "version_uuid, "
            "act_type_id, "
            "act_date, "
            "substation_id, "
            "connection_id, "
            "area_name, "
            "substation_name, "
            "connection_name, "
            "connection_voltage_kv, "
            "connection_ct_ratio, "
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
            ":uuid, "
            ":versionUuid, "
            ":actTypeId, "
            ":actDate, "
            ":substationId, "
            ":connectionId, "
            ":areaName, "
            ":substationName, "
            ":connectionName, "
            ":connectionVoltage, "
            ":connectionCtRatio, "
            ":employeeId, "
            ":employeeName, "
            ":employeePosition, "
            ":reason, "
            ":result, "
            ":sealNumber,"
            ":replacementDuration, "
            ":workScheduleType"
        ");");

    query.bindValue(":uuid", UuidUtils::generate());
    query.bindValue(":versionUuid", UuidUtils::generate());

    query.bindValue(":actTypeId", actTypeId);
    query.bindValue(":actDate", actDate);
    query.bindValue(":substationId", substationId);
    query.bindValue(":connectionId", connectionId);

    query.bindValue(":areaName", areaName);
    query.bindValue(":substationName", substationName);
    query.bindValue(":connectionName", connectionName);
    query.bindValue(":connectionVoltage", connectionVoltage);
    query.bindValue(":connectionCtRatio", connectionCtRatio);


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

// Сохраняет прибор с привязкой к указанному присоединению
int CreateActWidget::insertActMeter(
    const int actId,
    const MeterActWidget* meterWidget,
    const int role,
    int connectionId)
{
    // Для обычных актов получаем присоединение из основной формы
    if (connectionId == -1)
    {
        connectionId = ui->connectionComboBox->currentData().toInt();
    }

    // Получаем исторические данные присоединения.
    QSqlQuery connectionQuery(m_database.getDatabase());

    connectionQuery.prepare(
        "SELECT "
        "CASE connection_type "
            "WHEN 1 THEN "
                "'яч.' || printf('%g', voltage_kv) || "
                "'кВ №' || cell_number || ' \"' || name || '\"' "
            "WHEN 2 THEN "
                "'ВЛ-' || printf('%g', voltage_kv) || "
                "'кВ \"' || name || '\"' "
            "ELSE name "
        "END AS connection_name, "
        "voltage_kv, "
        "CASE "
            "WHEN ct_secondary_current = 0 "
                "THEN 'Прямого включения' "
            "ELSE CAST(ct_primary_current AS TEXT) || '/' || "
                 "CAST(ct_secondary_current AS TEXT) "
        "END AS ct_ratio "
        "FROM connections "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId;");

    connectionQuery.bindValue(":connectionId", connectionId);
    connectionQuery.bindValue(
        ":substationId",
        ui->substationComboBox->currentData().toInt());

    if (!connectionQuery.exec() || !connectionQuery.next())
    {
        QMessageBox::critical(
            this,
            "Ошибка базы данных",
            "Не удалось получить данные присоединения: "
            + connectionQuery.lastError().text());

        return -1;
    }

    // Сохраняем прибор вместе с историческим снимком.
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO act_meters ("
        "uuid, "
        "act_id, "
        "meter_id, "
        "connection_id, "
        "connection_name, "
        "connection_voltage_kv, "
        "connection_ct_ratio, "
        "role, "
        "meter_name, "
        "serial_number, "
        "accuracy_class, "
        "verification_year"
        ") VALUES ("
        ":uuid, "
        ":actId, "
        ":meterId, "
        ":connectionId, "
        ":connectionName, "
        ":connectionVoltage, "
        ":connectionCtRatio, "
        ":role, "
        ":meterName, "
        ":serialNumber, "
        ":accuracyClass, "
        ":verificationYear"
        ");");

    query.bindValue(":uuid", UuidUtils::generate());
    query.bindValue(":actId", actId);
    query.bindValue(":meterId", meterWidget->meterId());
    query.bindValue(":connectionId", connectionId);

    query.bindValue(
        ":connectionName",
        connectionQuery.value("connection_name"));

    query.bindValue(
        ":connectionVoltage",
        connectionQuery.value("voltage_kv"));

    query.bindValue(
        ":connectionCtRatio",
        connectionQuery.value("ct_ratio"));

    query.bindValue(":role", role);
    query.bindValue(":meterName", meterWidget->meterName());
    query.bindValue(":serialNumber", meterWidget->serialNumber());
    query.bindValue(":accuracyClass", meterWidget->accuracyClass());
    query.bindValue(":verificationYear", meterWidget->verificationYear());

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Ошибка базы данных",
            "Не удалось сохранить данные прибора в акте: "
            + query.lastError().text());

        return -1;
    }

    // возвращаем ID добавленного прибора
    return query.lastInsertId().toInt();
}

// Сохраняет все присоединения, приборы и показания группового акта
bool CreateActWidget::insertReadingConnections(int actId)
{
    // Последовательно обрабатываем каждую карточку
    for (ReadingConnectionWidget* readingWidget : m_readingConnectionWidgets)
    {
        const int connectionId = readingWidget->selectedConnectionId();
        MeterActWidget* meterWidget = readingWidget->meterWidget();

        // Сохраняем прибор с историческими данными присоединения
        const int actMeterId = insertActMeter(
            actId,
            meterWidget,
            ReadingMeter,
            connectionId
        );

        if (actMeterId < 0)
            return false;

        // Сохраняем выбранные показания прибора
        if (!insertReadings(actMeterId, meterWidget))
            return false;

        // Обновляем актуальный год поверки в справочнике приборов
        if (!updateMeterVerificationYear(meterWidget))
            return false;
    }

    return true;
}

bool CreateActWidget::updateReadingConnections(const ActData &original)
{
    // Обновляем существующие приборы группового акта.
    // Метод должен вызываться внутри общей транзакции.

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE act_meters SET "
        "meter_id = :meterId, "
        "meter_name = :meterName, "
        "serial_number = :serialNumber, "
        "accuracy_class = :accuracyClass, "
        "verification_year = :verificationYear "
        "WHERE id = :actMeterId "
        "AND act_id = :actId "
        "AND connection_id = :connectionId "
        "AND role = 4;");

    for (ReadingConnectionWidget *widget : m_readingConnectionWidgets)
    {
        const ActMeterData meter = widget->data();

        // При редактировании у каждой карточки
        // должна существовать исходная запись в БД.
        if (meter.id < 0 || meter.meterId < 0 ||
            meter.connectionId < 0)
        {
            QMessageBox::warning(
                this,
                "Ошибка данных",
                "Не удалось определить прибор или присоединение "
                "редактируемого акта.");

            return false;
        }

        // Ищем исходные данные прибора по ID записи акта.
        const ActMeterData *originalMeter = nullptr;

        for (const ActMeterData &item : original.meters)
        {
            if (item.id == meter.id)
            {
                originalMeter = &item;
                break;
            }
        }

        if (!originalMeter)
        {
            QMessageBox::warning(
                this,
                "Ошибка данных",
                "Не удалось найти исходные данные прибора учёта.");

            return false;
        }

        // Определяем, изменились ли характеристики прибора.
        const bool meterChanged =
            meter.meterId != originalMeter->meterId ||
            meter.name != originalMeter->name ||
            meter.serialNumber != originalMeter->serialNumber ||
            meter.accuracyClass != originalMeter->accuracyClass ||
            meter.verificationYear != originalMeter->verificationYear;

        if (meterChanged)
        {
            query.bindValue(":meterId", meter.meterId);
            query.bindValue(":meterName", meter.name);
            query.bindValue(":serialNumber", meter.serialNumber);
            query.bindValue(":accuracyClass", meter.accuracyClass);
            query.bindValue(":verificationYear", meter.verificationYear);

            query.bindValue(":actMeterId", meter.id);
            query.bindValue(":actId", m_editActId);
            query.bindValue(":connectionId", meter.connectionId);

            if (!query.exec())
            {
                QMessageBox::critical(
                    this,
                    "Ошибка базы данных",
                    "Не удалось обновить прибор учёта:\n"
                    + query.lastError().text());

                return false;
            }


            // Если запись не найдена, продолжать сохранение нельзя.
            if (query.numRowsAffected() != 1)
            {
                QMessageBox::warning(
                    this,
                    "Ошибка данных",
                    "Запись прибора учёта не найдена "
                    "или её присоединение изменилось.");

                return false;
            }
        }

        // Обновляем показания прибора.
        // Существующие записи сохраняют свои ID.
        QSqlQuery readingQuery(m_database.getDatabase());

        readingQuery.prepare(
            "INSERT INTO meter_readings ("
            "act_meter_id, reading_type_id, value"
            ") VALUES ("
            ":actMeterId, :readingTypeId, :value"
            ") "
            "ON CONFLICT(act_meter_id, reading_type_id) "
            "DO UPDATE SET value = excluded.value;");

        for (const ActMeterReadingData &reading : meter.readings)
        {
            // Ищем исходное показание того же типа.
            const ActMeterReadingData *originalReading = nullptr;

            for (const ActMeterReadingData &item : originalMeter->readings)
            {
                if (item.typeId == reading.typeId)
                {
                    originalReading = &item;
                    break;
                }
            }

            // Новое или изменённое показание необходимо сохранить.
            const bool readingChanged =
                !originalReading ||
                std::abs(reading.value - originalReading->value) > 1e-6;

            // Проверяем тип показания
            if (reading.typeId < 1 || reading.typeId > 4)
            {
                QMessageBox::warning(
                    this,
                    "Ошибка данных",
                    "Обнаружен неизвестный тип показания.");

                return false;
            }

            if (!readingChanged)
                continue;

            readingQuery.bindValue(":actMeterId", meter.id);
            readingQuery.bindValue(":readingTypeId", reading.typeId);
            readingQuery.bindValue(":value", reading.value);

            if (!readingQuery.exec())
            {
                QMessageBox::critical(
                    this,
                    "Ошибка базы данных",
                    "Не удалось обновить показания прибора:\n"
                    + readingQuery.lastError().text());

                return false;
            }
        }

        // Удаляем показания, отметки которых сняты в форме.
        QSqlQuery deleteQuery(m_database.getDatabase());

        deleteQuery.prepare(
            "DELETE FROM meter_readings "
            "WHERE act_meter_id = :actMeterId "
            "AND reading_type_id = :readingTypeId;");

        for (const ActMeterReadingData &oldReading : originalMeter->readings)
        {
            // Проверяем, осталось ли исходное показание в форме.
            bool stillExists = false;

            for (const ActMeterReadingData &currentReading : meter.readings)
            {
                if (currentReading.typeId == oldReading.typeId)
                {
                    stillExists = true;
                    break;
                }
            }

            // Показание осталось — удалять ничего не нужно.
            if (stillExists)
                continue;

            // Отметка снята — удаляем только это показание.
            deleteQuery.bindValue(":actMeterId", meter.id);
            deleteQuery.bindValue(":readingTypeId", oldReading.typeId);

            if (!deleteQuery.exec())
            {
                QMessageBox::critical(
                    this,
                    "Ошибка базы данных",
                    "Не удалось удалить снятое показание:\n"
                    + deleteQuery.lastError().text());

                return false;
            }

            if (deleteQuery.numRowsAffected() != 1)
            {
                QMessageBox::warning(
                    this,
                    "Ошибка данных",
                    "Исходное показание не найдено в базе данных.");

                return false;
            }
        }
    }

    return true;
}

// Обновление сторонних представителей редактируемого акта.
// Вызывается только внутри уже открытой транзакции.
bool CreateActWidget::updateExternalRepresentatives(
    const ActData &original)
{
    QSqlQuery query(m_database.getDatabase());

    // Удаляем только представителей, отсутствующих в текущей форме.
    for (const ExternalRepresentativeData &oldRepresentative :
         original.externalRepresentatives)
    {
        // Проверяем, осталась ли исходная запись в форме.
        bool exists = false;

        for (ExternalRepresentativeWidget *widget :
             m_externalRepresentativeWidgets)
        {
            if (widget->id() == oldRepresentative.id)
            {
                exists = true;
                break;
            }
        }

        // Существующего представителя пока не трогаем.
        if (exists)
            continue;

        QSqlQuery query(m_database.getDatabase());

        if (!query.prepare(
            "DELETE FROM external_representatives "
            "WHERE id = :id AND act_id = :actId;"))
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось подготовить удаление представителя:\n"
                + query.lastError().text());
            return false;
        }

        query.bindValue(":id", oldRepresentative.id);
        query.bindValue(":actId", m_editActId);

        if (!query.exec() || query.numRowsAffected() != 1)
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось удалить стороннего представителя:\n"
                + query.lastError().text());
            return false;
        }
    }

    // Обновляем существующих и добавляем новых представителей.
    for (ExternalRepresentativeWidget *widget :
         m_externalRepresentativeWidgets)
    {
        // Новый представитель ещё не имеет идентификатора в базе данных.
        if (widget->id() == -1)
        {
            QSqlQuery query(m_database.getDatabase());

            if (!query.prepare(
                "INSERT INTO external_representatives ("
                "act_id, organization, short_name, position"
                ") VALUES ("
                ":actId, :organization, :shortName, :position"
                ");"))
            {
                QMessageBox::critical(this, "Ошибка базы данных",
                    "Не удалось подготовить добавление представителя:\n"
                    + query.lastError().text());
                return false;
            }

            query.bindValue(":actId", m_editActId);
            query.bindValue(":organization", widget->organization());
            query.bindValue(":shortName", widget->shortName());
            query.bindValue(":position", widget->position());

            if (!query.exec())
            {
                QMessageBox::critical(this, "Ошибка базы данных",
                    "Не удалось добавить стороннего представителя:\n"
                    + query.lastError().text());
                return false;
            }

            continue;
        }

        // Находим исходные данные существующего представителя.
        const ExternalRepresentativeData *originalRepresentative = nullptr;

        for (const ExternalRepresentativeData &representative :
             original.externalRepresentatives)
        {
            if (representative.id == widget->id())
            {
                originalRepresentative = &representative;
                break;
            }
        }

        // Не допускаем обновления записи, отсутствующей в исходном акте.
        if (!originalRepresentative)
        {
            QMessageBox::critical(this, "Ошибка данных",
                "Не найден исходный сторонний представитель.");
            return false;
        }

        // Неизменённую запись оставляем без обновления.
        if (originalRepresentative->organization == widget->organization() &&
            originalRepresentative->shortName == widget->shortName() &&
            originalRepresentative->position == widget->position())
        {
            continue;
        }

        QSqlQuery query(m_database.getDatabase());

        if (!query.prepare(
            "UPDATE external_representatives SET "
            "organization = :organization, "
            "short_name = :shortName, "
            "position = :position "
            "WHERE id = :id AND act_id = :actId;"))
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось подготовить обновление представителя:\n"
                + query.lastError().text());
            return false;
        }

        query.bindValue(":organization", widget->organization());
        query.bindValue(":shortName", widget->shortName());
        query.bindValue(":position", widget->position());
        query.bindValue(":id", widget->id());
        query.bindValue(":actId", m_editActId);

        if (!query.exec() || query.numRowsAffected() != 1)
        {
            QMessageBox::critical(this, "Ошибка базы данных",
                "Не удалось обновить стороннего представителя:\n"
                + query.lastError().text());
            return false;
        }
    }

    return true;
}

bool CreateActWidget::validateOriginalReadingConnections(
    const ActData &original)
{
    // Проверяем, что количество карточек не изменилось.
    if (m_readingConnectionWidgets.size() != original.meters.size())
    {
        QMessageBox::warning(
            this,
            "Ошибка данных",
            "Количество присоединений в акте изменилось.");

        return false;
    }

    // Собираем исходные записи по ID.
    QHash<int, int> originalConnections;

    for (const ActMeterData &meter : original.meters)
    {
        if (meter.id < 0 || meter.role != 4 ||
            originalConnections.contains(meter.id))
        {
            QMessageBox::warning(
                this,
                "Ошибка данных",
                "Исходные данные приборов акта некорректны.");

            return false;
        }

        originalConnections.insert(meter.id, meter.connectionId);
    }

    // Проверяем каждую карточку редактируемого акта.
    for (ReadingConnectionWidget *widget : m_readingConnectionWidgets)
    {
        const ActMeterData meter = widget->data();

        // Карточка должна соответствовать существующей записи.
        if (!originalConnections.contains(meter.id))
        {
            QMessageBox::warning(
                this,
                "Ошибка данных",
                "Обнаружена новая или неизвестная карточка присоединения.");

            return false;
        }

        // Присоединение должно остаться прежним.
        if (originalConnections.value(meter.id) != meter.connectionId)
        {
            QMessageBox::warning(
                this,
                "Ошибка данных",
                "Изменение присоединений при редактировании запрещено.");

            return false;
        }

        // Удаляем проверенную запись, чтобы обнаружить дубликаты.
        originalConnections.remove(meter.id);
    }

    if (!originalConnections.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Ошибка данных",
            "Некоторые исходные присоединения отсутствуют.");

        return false;
    }

    return true;
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
        "SET verification_year = :verificationYear, "
            "version_uuid = :versionUuid "
        "WHERE id = :meterId "
            "AND verification_year IS NOT :verificationYear;");

    query.bindValue(":verificationYear", verificationYear);
    query.bindValue(":versionUuid", UuidUtils::generate());
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
    // Условие выбора акта "Снятие показаний"
    const bool isReadingAct =
        ui->actTypeComboBox->currentData().isValid() &&
        ui->actTypeComboBox->currentData().toInt() == 3;

    // Скрываем поля присоединения и его характеристик
    ui->connectionLabel->setVisible(!isReadingAct);
    ui->connectionComboBox->setVisible(!isReadingAct);

    ui->voltageLabel->setVisible(!isReadingAct);
    ui->voltageLineEdit->setVisible(!isReadingAct);

    ui->ctRatioLabel->setVisible(!isReadingAct);
    ui->ctRatioLineEdit->setVisible(!isReadingAct);

    // Новый интерфейс для снятия показаний
    ui->readingMetersContainer->setVisible(isReadingAct);
    ui->addReadingMeterButton->setVisible(isReadingAct);
    // Скрываем старую карточку при групповом снятии показаний
    ui->primaryMeterCard->setVisible(!isReadingAct);

    // Показываем или скрываем подсказку
    ui->connectionInfoLabel->setVisible(isReadingAct);

    // Временно для проверки скрываем панели
    ui->removedCtCard->setVisible(false);
    ui->installedCtCard->setVisible(false);

    // Если тип акта не выбран
    if (!ui->actTypeComboBox->currentData().isValid())
    {
        ui->primaryMeterCardTitle->setText("Прибор учета");
        ui->secondaryMeterCard->setVisible(false);
        // скрываем поле пломбы пока не выберется тип акта
        ui->sealWidget->setVisible(false);
        // скрываем блоки ТТ
        ui->removedCtCard->setVisible(false);
        ui->installedCtCard->setVisible(false);
        // Обновляем отображение полей векторной диаграммы
        updateVectorDiagramUi();
        return;
    }
    // получаем id выбранного акта
    int actTypeId = ui->actTypeComboBox->currentData().toInt();
    // Отображаем поле пломбы везде кроме демонтажа
    ui->sealWidget->setVisible(actTypeId != 4);
    // скрываем блоки ТТ
    ui->removedCtCard->setVisible(false);
    ui->installedCtCard->setVisible(false);

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
            ui->primaryMeterCardTitle->setText("Проверяемый прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            break;
        }
        case 2:     // Замена
        {
            ui->primaryMeterCardTitle->setText("Снимаемый прибор учета");
            ui->secondaryMeterCardTitle->setText("Устанавливаемый прибор учета");
            ui->secondaryMeterCard->setVisible(true);
            break;
        }
        case 3:     // Снятие показаний
        {
            ui->primaryMeterCardTitle->setText("Прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            break;
        }
        case 4:     // Демонтаж
        {
            ui->primaryMeterCardTitle->setText("Демонтируемый прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            break;
        }
        case 5:     // Установка
        {
            ui->primaryMeterCardTitle->setText("Устанавливаемый прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            break;
        }
        case 6:     // Установка прибора учета и ТТ
        {
            ui->primaryMeterCardTitle->setText("Устанавливаемый прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            // поля для ввода трансформаторов тока
            ui->removedCtCard->setVisible(false);
            ui->installedCtCard->setVisible(true);

            break;
        }
        case 7:     // Замена ТТ
        {
            ui->primaryMeterCardTitle->setText("Прибор учета");
            ui->secondaryMeterCard->setVisible(false);
            // поля для ввода трансформаторов тока
            ui->removedCtCard->setVisible(true);
            ui->installedCtCard->setVisible(true);

            break;
        }
        default:
        {
            ui->primaryMeterCardTitle->setText("Прибор учета");
            ui->secondaryMeterCard->setVisible(false);
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
    auto *layout = qobject_cast<QVBoxLayout *>(
        ui->externalRepresentativesContainer->layout());

    layout->insertWidget(layout->count() - 1, representative);

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
    // Если тип акта всё еще не выбран
    if (!ui->actTypeComboBox->currentData().isValid())
    {
        // устанавливаем видимость
        ui->vectorDiagramCard->setVisible(false);
        ui->replacementDurationCard->setVisible(false);
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
    // Устанавливаем видимость карточки векторной диаграммы
    ui->vectorDiagramCard->setVisible(supportsVectorDiagram);
    // меняем видимость в случае изменения переменной
    if (!supportsVectorDiagram)
    {
        ui->replacementDurationCard->setVisible(false);
        return;
    }
    // булева переменная выбора наличия векторной диаграммы
    const bool hasVectorDiagram = ui->hasVectorDiagramCheckBox->isChecked();

    // устанавливаем видимость самой векторной диаграммы
    ui->vectorDiagramContainerWidget->setVisible(hasVectorDiagram);

    // переменная показа времени замены прибора
    const bool showReplacementDuration = actTypeId == 2 && hasVectorDiagram;

    // меняем видимость
    ui->replacementDurationCard->setVisible(showReplacementDuration);
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


// универсальный метод проверки заполненности формы ТТ
bool CreateActWidget::validateCurrentTransformerReplacement()
{
    const QList<ActCurrentTransformerData> removed =
        m_removedCtPhasePanel->data(RemovedCurrentTransformer);

    const QList<ActCurrentTransformerData> installed =
        m_installedCtPhasePanel->data(InstalledCurrentTransformer);

    QSet<int> removedTransformerIds;

    for (const ActCurrentTransformerData &transformer : removed)
    {
        removedTransformerIds.insert(
            transformer.currentTransformerId);
    }

    for (const ActCurrentTransformerData &transformer : installed)
    {
        if (removedTransformerIds.contains(
            transformer.currentTransformerId))
        {
            QMessageBox::warning(this, "Ошибка выбора трансформатора тока",
                "Трансформатор тока с заводским номером "
                + transformer.serialNumber
                + " одновременно указан как снятый и установленный.");

            return false;
        }
    }

    return true;
}


bool CreateActWidget::insertActCurrentTransformer(
    int actId,
    const ActCurrentTransformerData &transformer)
{
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

    query.bindValue(":currentTransformerId", transformer.currentTransformerId);
    query.bindValue(":role", transformer.role);
    query.bindValue(":phase", transformer.phase);
    query.bindValue(":name", transformer.name);
    query.bindValue(":serialNumber", transformer.serialNumber);
    query.bindValue(":transformationRatio", transformer.transformationRatio);
    query.bindValue(":accuracyClass", transformer.accuracyClass);

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось добавить трансформатор тока в акт: "
            + query.lastError().text());

        return false;
    }

    return true;
}

bool CreateActWidget::insertActCurrentTransformers(
    int actId,
    const QList<ActCurrentTransformerData> &transformers)
{
    for (const ActCurrentTransformerData &transformer : transformers)
    {
        if (!insertActCurrentTransformer(
            actId, transformer))
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
    // Удаляем ТТ из карточек
    if (m_removedCtPhasePanel)
    {
        m_removedCtPhasePanel->clear();
    }

    if (m_installedCtPhasePanel)
    {
        m_installedCtPhasePanel->clear();
    }
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

    m_originalVersionUuid = data.versionUUid;
    qDebug() << "Original act version UUID: " << m_originalVersionUuid;

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

    // При редактировании запрещаем изменять
    // тип акта и место проведения работ
    ui->actTypeComboBox->setEnabled(false);
    ui->areaComboBox->setEnabled(false);
    ui->substationComboBox->setEnabled(false);

    // Загружаем присоединение
    const int connectionIndex =
        ui->connectionComboBox->findData(data.connectionId);

    if (connectionIndex >= 0)
    {
        ui->connectionComboBox->setCurrentIndex(connectionIndex);
    }

    // Восстанавливаем карточки группового снятия показаний
    if (data.actTypeId == 3)
    {
        int readingIndex = 0;

        for (const ActMeterData& meter : data.meters)
        {
            // Обрабатываем только приборы снятия показаний
            if (meter.role != ReadingMeter)
                continue;

            // Первая карточка уже создана в конструкторе
            if (readingIndex > 0)
            {
                // Создаём следующую карточку штатным обработчиком
                ui->addReadingMeterButton->click();
            }

            ReadingConnectionWidget* widget =
                m_readingConnectionWidgets.at(readingIndex);

            // Восстанавливаем присоединение, прибор и показания
            if (!widget->setData(meter))
            {
                QMessageBox::warning(
                    this,
                    "Ошибка загрузки",
                    QString(
                        "Не удалось восстановить присоединение "
                        "для карточки № %1.\n"
                        "ID присоединения: %2"
                    )
                        .arg(readingIndex + 1)
                        .arg(meter.connectionId)
                );
            }

            // При редактировании существующего акта запрещаем
            // изменение присоединения и удаление его карточки
            widget->setConnectionLocked(true);

            ++readingIndex;
        }

        // Запрещаем добавление новых присоединений
        // при редактировании существующего акта
        ui->addReadingMeterButton->hide();

        // Обновляем номера и доступность присоединений
        updateReadingWidgets();
        updateAvailableConnections();
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

    // Загружаем основной прибор учета
    // Основной прибор восстанавливаем только для обычных актов
    if (data.actTypeId != 3)
    {
        const int primaryRole = primaryMeterRole();

        if (const ActMeterData* meter = meterByRole(data, primaryRole))
        {
            m_primaryMeterWidget->setData(*meter);
        }
    }
    // Загружаем дополнительный прибор учета если имеется
    if (data.actTypeId == 2)
    {
        if (const ActMeterData* meter = meterByRole(data, InstalledMeter))
        {
            m_secondaryMeterWidget->setData(*meter);
        }
    }

    // Загружаем данные заключения
    ui->reasonPlainTextEdit->setPlainText(data.reason);
    ui->resultPlainTextEdit->setPlainText(data.result);
    // Загружаем пломбу
    ui->sealLineEdit->setText(data.sealNumber);

    // Загружаем длительность замены прибора
    if (data.actTypeId == 2 && data.vectorDiagram.exists)
    {
        ui->replacementDurationSpinBox->setValue(data.replacementDurationMinutes);
    }

    // Загружаем сторонних представителей
    clearExternalRepresentatives();

    for (const ExternalRepresentativeData &representativeData :
        data.externalRepresentatives)
    {
        addExternalRepresentative();

        ExternalRepresentativeWidget* representative =
            m_externalRepresentativeWidgets.last();

        representative->setData(representativeData);
    }

    // Загружаем векторную диаграмму
    ui->hasVectorDiagramCheckBox->setChecked(data.vectorDiagram.exists);

    if (data.vectorDiagram.exists)
    {
        m_vectorDiagramWidget->setData(data.vectorDiagram);
    }

    // Загружаем трансформаторы тока
    QList<ActCurrentTransformerData> removedTransformers;
    QList<ActCurrentTransformerData> installedTransformers;

    for (const ActCurrentTransformerData &transformer :
        data.currentTransformers)
    {
        if (transformer.role == RemovedCurrentTransformer)
        {
            removedTransformers.append(transformer);
        }
        else if (transformer.role == InstalledCurrentTransformer)
        {
            installedTransformers.append(transformer);
        }
    }

    m_removedCtPhasePanel->setData(removedTransformers);
    m_installedCtPhasePanel->setData(installedTransformers);
}

const ActMeterData * CreateActWidget::meterByRole(const ActData &data, int role) const
{
    for (const ActMeterData &meter : data.meters)
    {
        if (meter.role == role)
        {
            return &meter;
        }
    }

    return nullptr;
}

ActData CreateActWidget::collectActData() const
{
    ActData data;

    data.id = m_editActId;

    // Основные данные акта
    data.actTypeId =
        ui->actTypeComboBox->currentData().toInt();

    data.date = ui->actDateEdit->date();

    data.areaId =
        ui->areaComboBox->currentData().toInt();

    data.substationId =
        ui->substationComboBox->currentData().toInt();

    // Для акта снятия показаний с одним присоединением
    // сохраняем его ID в основных данных.
    // Для нескольких присоединений общего ID нет.
    if (data.actTypeId == 3)
    {
        data.connectionId =
            m_readingConnectionWidgets.size() == 1
                ? m_readingConnectionWidgets.first()->selectedConnectionId()
                : -1;
    }
    else
    {
        data.connectionId =
            ui->connectionComboBox->currentData().toInt();
    }

    data.employeeId =
        ui->employeeComboBox->currentData().toInt();

    // Характер работ
    if (data.actTypeId == 1 ||
        data.actTypeId == 2 ||
        data.actTypeId == 7)
    {
        data.workScheduleType =
            ui->workScheduleComboBox->currentData().toInt();
    }

    // Заключение
    data.reason =
        ui->reasonPlainTextEdit->toPlainText().trimmed();

    data.result =
        ui->resultPlainTextEdit->toPlainText().trimmed();

    if (data.actTypeId != 4)
    {
        data.sealNumber =
            ui->sealLineEdit->text().trimmed();
    }

    // Длительность замены
    if (data.actTypeId == 2 &&
        ui->hasVectorDiagramCheckBox->isChecked())
    {
        data.replacementDurationMinutes =
            ui->replacementDurationSpinBox->value();
    }

    // Сторонние представители
    for (const ExternalRepresentativeWidget* widget :
        m_externalRepresentativeWidgets)
    {
        ExternalRepresentativeData representative;

        representative.organization = widget->organization();
        representative.shortName = widget->shortName();
        representative.position = widget->position();

        // Сохраняем идентификатор существующего представителя.
        representative.id = widget->id();

        data.externalRepresentatives.append(representative);
    }

    // Собираем прибор и его показания
    auto collectMeter = [](const MeterActWidget* widget, int role)
    {
        ActMeterData meter;

        meter.meterId = widget->meterId();
        meter.role = role;

        meter.name = widget->meterName();
        meter.serialNumber = widget->serialNumber();
        meter.accuracyClass = widget->accuracyClass();
        meter.verificationYear = widget->verificationYear();

        for (const auto& reading : widget->readings())
        {
            ActMeterReadingData value;

            value.typeId = reading.typeId;
            value.value = reading.value;

            meter.readings.append(value);
        }

        return meter;
    };

    if (data.actTypeId == 3)
    {
        // Групповой акт: собираем приборы из всех карточек
        for (const ReadingConnectionWidget* widget :
             m_readingConnectionWidgets)
        {
            data.meters.append(widget->data());
        }
    }
    else
    {
        // Обычный акт: основной прибор
        data.meters.append(
            collectMeter(m_primaryMeterWidget, primaryMeterRole()));

        // Второй прибор при замене
        if (data.actTypeId == 2)
        {
            data.meters.append(
                collectMeter(m_secondaryMeterWidget, InstalledMeter));
        }
    }

    // Трансформаторы тока
    if (data.actTypeId == 6 || data.actTypeId == 7)
    {
        if (data.actTypeId == 7)
        {
            data.currentTransformers.append(
                m_removedCtPhasePanel->data(RemovedCurrentTransformer));
        }

        data.currentTransformers.append(
            m_installedCtPhasePanel->data(InstalledCurrentTransformer));
    }

    // Добавляем векторную диаграмму
    const bool supportsVectorDiagram =
        data.actTypeId == 1 ||
        data.actTypeId == 2 ||
        data.actTypeId == 5 ||
        data.actTypeId == 6 ||
        data.actTypeId == 7;

    if (supportsVectorDiagram &&
        ui->hasVectorDiagramCheckBox->isChecked())
    {
        auto& vector = data.vectorDiagram;

        vector.exists = true;

        vector.ia = m_vectorDiagramWidget->currentA();
        vector.ib = m_vectorDiagramWidget->currentB();
        vector.ic = m_vectorDiagramWidget->currentC();

        vector.angleA = m_vectorDiagramWidget->angleA();
        vector.angleB = m_vectorDiagramWidget->angleB();
        vector.angleC = m_vectorDiagramWidget->angleC();

        vector.angleAType = m_vectorDiagramWidget->angleAType();
        vector.angleBType = m_vectorDiagramWidget->angleBType();
        vector.angleCType = m_vectorDiagramWidget->angleCType();

        vector.uab = m_vectorDiagramWidget->uab();
        vector.ubc = m_vectorDiagramWidget->ubc();
        vector.uca = m_vectorDiagramWidget->uca();
    }

    return data;
}

bool CreateActWidget::hasActChanges(
    const ActData &original,
    const ActData &current) const
{

    // 1. Основные данные
    if (original.actTypeId != current.actTypeId ||
        original.date != current.date ||
        original.connectionId != current.connectionId ||
        original.employeeId != current.employeeId ||
        original.workScheduleType != current.workScheduleType ||
        original.reason != current.reason ||
        original.result != current.result ||
        original.sealNumber != current.sealNumber ||
        original.replacementDurationMinutes !=
            current.replacementDurationMinutes)
    {
        return true;
    }

    // 2. Сторонние представители.
    // Сравниваем записи по ID, независимо от порядка карточек.
    if (original.externalRepresentatives.size() !=
        current.externalRepresentatives.size())
    {
        return true;
    }

    for (const ExternalRepresentativeData &representative :
         current.externalRepresentatives)
    {
        // Новый представитель означает изменение состава акта.
        if (representative.id == -1)
            return true;

        const ExternalRepresentativeData *originalRepresentative = nullptr;

        // Находим исходную запись по её идентификатору.
        for (const ExternalRepresentativeData &oldRepresentative :
             original.externalRepresentatives)
        {
            if (oldRepresentative.id == representative.id)
            {
                originalRepresentative = &oldRepresentative;
                break;
            }
        }

        // Представитель отсутствовал в исходном акте.
        if (!originalRepresentative)
            return true;

        // Проверяем изменения данных найденного представителя.
        if (originalRepresentative->organization != representative.organization ||
            originalRepresentative->shortName != representative.shortName ||
            originalRepresentative->position != representative.position)
        {
            return true;
        }
    }

    // 3. Приборы учёта.
    if (original.meters.size() != current.meters.size())
        return true;

    auto findMeter = [](const QList<ActMeterData> &meters,
        int role) -> const ActMeterData*
    {
        for (const auto &meter : meters)
        {
            if (meter.role == role)
                return &meter;
        }

        return nullptr;
    };

    for (const auto &a : original.meters)
    {
        // Для группового акта сопоставляем приборы по присоединению
        const ActMeterData* b = nullptr;

        if (original.actTypeId == 3 && current.actTypeId == 3)
        {
            for (const auto& meter : current.meters)
            {
                if (meter.connectionId == a.connectionId)
                {
                    b = &meter;
                    break;
                }
            }
        }
        else
        {
            // Для обычных актов сохраняем сравнение по роли
            b = findMeter(current.meters, a.role);
        }

        if (!b)
            return true;

        // Для группового снятия показаний проверяем
        // исторические параметры присоединения
        if (original.actTypeId == 3 && current.actTypeId == 3)
        {
            if (a.connectionId != b->connectionId ||
                a.connectionName != b->connectionName ||
                a.connectionVoltage != b->connectionVoltage ||
                a.connectionCtRatio != b->connectionCtRatio)
            {
                return true;
            }
        }

        if (a.meterId != b->meterId ||
            a.name != b->name ||
            a.serialNumber != b->serialNumber ||
            a.accuracyClass != b->accuracyClass ||
            a.verificationYear != b->verificationYear)
        {
            return true;
        }

        if (a.readings.size() != b->readings.size())
            return true;

        for (const auto &readingA : a.readings)
        {
            bool found = false;

            for (const auto &readingB : b->readings)
            {
                if (readingA.typeId == readingB.typeId)
                {
                    if (std::abs(readingA.value - readingB.value) > 0.000001)
                        return true;

                    found = true;
                    break;
                }
            }

            if (!found)
                return true;
        }
    }


    // 4. Трансформаторы тока
    if (original.currentTransformers.size() !=
        current.currentTransformers.size())
    {
        return true;
    }

    auto normalizeTransformers =
        [](const QList<ActCurrentTransformerData> &items)
        {
            QStringList result;

            for (const auto &ct : items)
            {
                result.append(
                    QString::number(ct.role) + "|" +
                    ct.phase + "|" +
                    QString::number(ct.currentTransformerId) + "|" +
                    ct.name + "|" +
                    ct.serialNumber + "|" +
                    ct.transformationRatio + "|" +
                    ct.accuracyClass);
            }

            result.sort();
            return result;
        };

    if (normalizeTransformers(original.currentTransformers) !=
        normalizeTransformers(current.currentTransformers))
    {
        return true;
    }

    // 5. Векторная диаграмма
    const auto &a = original.vectorDiagram;
    const auto &b = current.vectorDiagram;

    if (a.exists != b.exists)
        return true;

    if (a.exists)
    {
        if (a.ia != b.ia ||
            a.ib != b.ib ||
            a.ic != b.ic ||
            a.angleA != b.angleA ||
            a.angleB != b.angleB ||
            a.angleC != b.angleC ||
            a.angleAType != b.angleAType ||
            a.angleBType != b.angleBType ||
            a.angleCType != b.angleCType ||
            a.uab != b.uab ||
            a.ubc != b.ubc ||
            a.uca != b.uca)
        {
            return true;
        }
    }

    return false;
}

bool CreateActWidget::checkActVersion()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT version_uuid "
        "FROM acts "
        "WHERE id = :actId;");

    query.bindValue(":actId", m_editActId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка базы данных",
            "Не удалось проверить версию акта:\n"
            + query.lastError().text());

        return false;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Акт не найден",
            "Акт был удален из базы данных.");

        return false;
    }

    const QString currentVersionUuid =
        query.value("version_uuid").toString();

    if (currentVersionUuid != m_originalVersionUuid)
    {
        QMessageBox::warning(this, "Конфликт версий",
            "Этот акт был изменён после открытия окна редактирования.\n\n"
            "Чтобы избежать потери данных, сохранение отменено.\n"
            "Закройте окно и откройте акт заново.");

        return false;
    }


    return true;
}

void CreateActWidget::updateReadingWidgets()
{
    const bool canRemove = m_readingConnectionWidgets.size() > 1;

    for (int i = 0; i < m_readingConnectionWidgets.size(); ++i) {
        auto *widget = m_readingConnectionWidgets.at(i);

        widget->setNumber(i + 1);
        widget->setRemoveEnabled(canRemove);
    }
}

void CreateActWidget::removeReadingWidget(ReadingConnectionWidget *widget)
{
    if (m_readingConnectionWidgets.size() <= 1)
        return;

    m_readingConnectionWidgets.removeOne(widget);

    ui->readingMetersLayout->removeWidget(widget);

    widget->hide();
    widget->deleteLater();

    updateReadingWidgets();
    updateAvailableConnections();
}

void CreateActWidget::updateAvailableConnections()
{
    QSet<int> selectedIds;

    // Собираем ID всех выбранных присоединений
    for (ReadingConnectionWidget *widget : m_readingConnectionWidgets) {
        const int id = widget->selectedConnectionId();

        if (id >= 0)
            selectedIds.insert(id);
    }

    // Обновляем доступность присоединений во всех карточках
    for (ReadingConnectionWidget *widget : m_readingConnectionWidgets) {
        widget->setUnavailableConnections(selectedIds);
    }
}

bool CreateActWidget::validateReadingConnections()
{
    for (int i = 0; i < m_readingConnectionWidgets.size(); ++i) {
        ReadingConnectionWidget *widget =
            m_readingConnectionWidgets.at(i);

        // Проверяем присоединение
        if (widget->selectedConnectionId() < 0) {
            scrollToReadingWidget(widget);

            QMessageBox::warning(
                this,
                "Не заполнено присоединение",
                QString(
                    "В карточке «Присоединение № %1» "
                    "не выбрано присоединение."
                ).arg(i + 1)
            );

            return false;
        }

        // Проверяем прибор учёта и показания
        if (!widget->validateMeter())
        {
            scrollToReadingWidget(widget);
            return false;
        }
    }

    return true;
}

void CreateActWidget::scrollToReadingWidget(
    ReadingConnectionWidget *widget)
{
    if (!widget)
        return;

    // Переключаемся на вкладку «Оборудование»
    ui->actTabWidget->setCurrentWidget(ui->metersTab);

    // Прокручиваем область к нужной карточке
    ui->ctScrollArea->ensureWidgetVisible(widget, 0, 20);
}

bool CreateActWidget::validateUniqueMeters()
{
    QHash<QString, int> serialNumbers;

    for (int i = 0; i < m_readingConnectionWidgets.size(); ++i) {
        ReadingConnectionWidget *widget =
            m_readingConnectionWidgets.at(i);

        const QString serial =
            widget->meterSerialNumber();

        if (serial.isEmpty())
            continue;

        // Нормализуем серийный номер
        const QString key = serial.toUpper();

        if (serialNumbers.contains(key)) {
            const int previousNumber = serialNumbers.value(key);

            scrollToReadingWidget(widget);

            QMessageBox::warning(
                this,
                "Повторяющийся прибор учёта",
                QString(
                    "Прибор учёта с серийным номером «%1» "
                    "уже используется в карточке № %2.\n\n"
                    "Повторное использование обнаружено "
                    "в карточке № %3."
                )
                .arg(serial)
                .arg(previousNumber)
                .arg(i + 1)
            );

            return false;
        }

        serialNumbers.insert(key, i + 1);
    }

    return true;
}



















