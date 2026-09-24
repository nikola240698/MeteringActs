

#include <QStringList>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <algorithm>
#include <QFileDialog>
#include <QMessageBox>

#include "actviewdialog.h"
#include "ui_actviewdialog.h"
#include "actrepository.h"
#include "database.h"
#include "actcalculator.h"
#include "docxgenerator.h"


ActViewDialog::ActViewDialog(
    Database &database, int actId, QWidget *parent)
        : QDialog(parent), ui(new Ui::ActViewDialog), m_database(database), m_actId(actId)
{
    ui->setupUi(this);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(ui->generateDocxButton, &QPushButton::clicked, this,
        &ActViewDialog::generateDocx);

    loadAct();
}

ActViewDialog::~ActViewDialog()
{
    delete ui;
}

void ActViewDialog::loadAct()
{
    ActRepository repository(m_database);

    if (!repository.loadAct(m_actId, m_data))
    {
        QMessageBox::critical(this, "ОШибка",
            "Не удалось загрузить акт: " + repository.lastError());

        reject();
        return;
    }

    fillActData();
}

void ActViewDialog::fillActData()
{
    ui->databaseIdLabel->setText(
        QString("Запись в БД: №%1").arg(m_actId));

    ui->substationLabel->setText(
        "ПС" + m_data.substationName);

    ui->areaLabel->setText(
        m_data.areaName);

    ui->actTitleLabel->setText(
        actTitle());

    ui->actDateLabel->setText(
        formatActDate());

    ui->connectionLabel->setText(
        m_data.connectionName);

    ui->ctRatioLabel->setText(
        "Ктт=" + m_data.ctRatio);

    ui->workDescriptionLabel->setText(
        workDescription());

    ui->conclusionLabel->setText(
        m_data.reason + " " + m_data.result);

    ui->sealLabel->setText(
        "Клеммная крышка прибора учета опломбирована пломбами : "
        + m_data.sealNumber);


    ui->signatureWidget->hide();

    if (m_data.actTypeId == 4)
    {
        ui->sealLabel->hide();
    }
    else
    {
        ui->sealLabel->show();
        ui->sealLabel->setText(
            "Клеммная крышка прибора учета опломбирована пломбами: "
            + m_data.sealNumber);
    }

    switch (m_data.actTypeId)
    {
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
            ui->measurementDeviceLabel->show();
            ui->measurementDeviceLabel->setText(
                "Проверка приборов учета производилась средствами измерений: "
                "ВАФ-ПАРМА.");
            break;
        default:
            ui->measurementDeviceLabel->hide();
            break;
    }

    ui->equipmentWidget->show();

    // Заполняем данные прибора учета
    fillMeters();
    // Заполняем поля данных ТТ
    fillCurrentTransformers();
    // Заполняем векторную диграмму
    fillVectorDiagram();
    // Указываем время замены и недоучтенную энергию
    fillReplacementInfo();
    // Заполняем представителей в верхнем блоке акта
    fillRepresentatives();
    // Заполняем блок с подписями
    fillSignature();
}

QWidget * ActViewDialog::createMeterTable(const ActMeterData &meter, const QString &title)
{
    auto* container = new QWidget;

    auto* layout = new QVBoxLayout(container);

    layout->setContentsMargins(0, 6, 0, 6);
    layout->setSpacing(4);

    // Заголовок над таблицей
    auto* titleLabel = new QLabel(title);

    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    layout->addWidget(titleLabel);

    // Сама таблица
    auto* tableWidget = new QWidget;
    auto* grid = new QGridLayout(tableWidget);

    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    // Заголовки
    grid->addWidget(
        createTableCell("Тип прибора учета", true), 0, 0);
    grid->addWidget(
        createTableCell("Заводской номер", true), 0, 1);
    grid->addWidget(
        createTableCell("Показания", true), 0, 2);
    grid->addWidget(
        createTableCell("Класс точности", true), 0, 3);
    grid->addWidget(
        createTableCell("Год поверки", true), 0, 4);

    const QString readingText = meterReadingsText(meter);

    // Данные
    grid->addWidget(createTableCell(meter.name), 1, 0);

    grid->addWidget(createTableCell(meter.serialNumber), 1, 1);

    grid->addWidget(createTableCell(readingText), 1, 2);

    grid->addWidget(createTableCell(meter.accuracyClass), 1, 3);

    grid->addWidget(createTableCell(QString::number(meter.verificationYear)), 1, 4);

    // Пропорции колонок
    grid->setColumnStretch(0, 3);
    grid->setColumnStretch(1, 2);
    grid->setColumnStretch(2, 3);
    grid->setColumnStretch(3, 2);
    grid->setColumnStretch(4, 2);

    layout->addWidget(tableWidget);

    return container;
}

QString ActViewDialog::meterReadingValue(const ActMeterData &meter, const QString &code) const
{
    for (const ActMeterReadingData &reading : meter.readings)
    {
        if (reading.code == code)
        {
            return QString::number(reading.value, 'f', 2);
        }
    }

    return {};
}

QString ActViewDialog::meterReadingsText(const ActMeterData &meter) const
{
    QStringList lines;

    const QString aPlus = meterReadingValue(meter, "A+");
    const QString aMinus = meterReadingValue(meter, "A-");
    const QString rPlus = meterReadingValue(meter, "R+");
    const QString rMinus = meterReadingValue(meter, "R-");

    if (!aPlus.isEmpty())
        lines.append("А(отд.)   " + aPlus);
    if (!aMinus.isEmpty())
        lines.append("А(пр.)    " + aMinus);
    if (!rPlus.isEmpty())
        lines.append("R(отд.)   " + rPlus);
    if (!rMinus.isEmpty())
        lines.append("R(пр.)    " + rMinus);

    return lines.join('\n');
}

void ActViewDialog::fillMeters()
{

    switch (m_data.actTypeId)
    {
        case 1:
        {
            const ActMeterData* meter = findMeterByRole(1);

            if (meter)
            {
                ui->equipmentLayout->addWidget(createMeterTable(*meter, "Прибор учета: "));
            }
            break;
        }
        case 2:
        {
            const ActMeterData* removedMeter = findMeterByRole(2);

            const ActMeterData* installedMeter = findMeterByRole(3);

            if (removedMeter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*removedMeter, "Снятый прибор учета: "));
            }

            if (installedMeter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*installedMeter, "Установленный прибор учета: "));
            }

            break;
        }
        case 3:
        {
            const ActMeterData* meter = findMeterByRole(4);

            if (meter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*meter, "Пибор учета: "));
            }
            break;
        }
        case 4:
        {
            const ActMeterData* meter = findMeterByRole(2);

            if (meter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*meter, "Снятый прибор учета: "));
            }
            break;
        }
        case 5:
        case 6:
        {
            const ActMeterData* meter = findMeterByRole(3);

            if (meter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*meter, "Установленный прибор учета: "));
            }
            break;
        }
        case 7:
        {
            const ActMeterData* meter = findMeterByRole(5);

            if (meter)
            {
                ui->equipmentLayout->addWidget(
                    createMeterTable(*meter, "Прибор учета: "));
            }
            break;
        }
        default:
            break;
    }
}

QLabel * ActViewDialog::createTableCell(const QString &text, bool header) const
{
    auto* label = new QLabel(text);

    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);

    label->setMinimumHeight(32);

    if (header)
    {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }

    label->setStyleSheet(
        "QLabel {"
        "   border: 1px solid palette(mid);"
        "   padding: 4px;"
        "}");

    return label;
}

const ActMeterData * ActViewDialog::findMeterByRole(int role) const
{
    for (const ActMeterData &meter : m_data.meters)
    {
        if (meter.role == role)
            return &meter;
    }

    return nullptr;
}

QString ActViewDialog::actTitle() const
{
    return QString("АКТ\n%1").arg(m_data.actTypeName.toLower());
}

QString ActViewDialog::workDescription() const
{
    QString work;

    switch (m_data.actTypeId)
    {
        case 1:
            work = "проверке прибора учета";
            break;
        case 2:
            work = "замене прибора учета";
            break;
        case 3:
            work = "снятию показаний прибора учета";
            break;
        case 4:
            work = "снятию прибора учета";
            break;
        case 5:
            work = "установке прибора учета";
            break;
        case 6:
            work = "установке трансформаторов тока и прибора учета";
            break;
        case 7:
            work = "замене трансформаторов тока";
            break;
        default:
            return {};
    }

    QString schedule;

    if (m_data.actTypeId == 1 ||
        m_data.actTypeId == 2 ||
        m_data.actTypeId == 7)
    {
        schedule = workScheduleText();
    }

    if (!schedule.isEmpty())
    {
        return QString("Произведена %1 работа по %2 на стороне %2кВ.")
            .arg(schedule)
            .arg(work)
            .arg(m_data.voltage);
    }

    return QString("Произведена работа по %1 на стороне %2кВ.")
            .arg(work)
            .arg(m_data.voltage);
}

QString ActViewDialog::formatActDate() const
{
    static const QStringList months =
    {
        "",
        "января",
        "февраля",
        "марта",
        "апреля",
        "мая",
        "июня",
        "июля",
        "августа",
        "сентября",
        "октября",
        "ноября",
        "декабря"
    };

    if (!m_data.date.isValid())
        return {};

    return QString("\"%1\" %2 %3 год")
        .arg(m_data.date.day())
        .arg(months.at(m_data.date.month()))
        .arg(m_data.date.year());
}

void ActViewDialog::fillCurrentTransformers()
{
    constexpr int RemovedCurrentTransformerRole = 1;
    constexpr int InstalledCurrentTransformerRole = 2;

    switch (m_data.actTypeId)
    {
        case 6:
        {
            const auto installedTransformers =
                currentTransformerByRole(InstalledCurrentTransformerRole);

            if (!installedTransformers.isEmpty())
            {
                ui->equipmentLayout->addWidget(
                    createCurrentTransformerTable(
                        installedTransformers, "Установленные трансформаторы тока:"));
            }
            break;
        }

        case 7:
        {
            const auto removedTransformers = currentTransformerByRole(
                RemovedCurrentTransformerRole);

            const auto installedTransformers = currentTransformerByRole(
                InstalledCurrentTransformerRole);

            if (!removedTransformers.isEmpty())
            {
                ui->equipmentLayout->addWidget(
                    createCurrentTransformerTable(
                        removedTransformers, "Снятые трансформаторы тока: "));
            }

            if (!installedTransformers.isEmpty())
            {
                ui->equipmentLayout->addWidget(
                    createCurrentTransformerTable(
                        installedTransformers, "Установленные трансфомраотры тока: "));
            }
            break;
        }

        default:
            break;
    }
}

QWidget * ActViewDialog::createCurrentTransformerTable(const QList<ActCurrentTransformerData> &transformers,
                                                       const QString &title)
{
    auto* container = new QWidget;

    auto* layout = new QVBoxLayout(container);

    layout->setContentsMargins(0, 6, 0, 6);
    layout->setSpacing(4);

    // Название таблицы
    auto* titleLabel = new QLabel(title);

    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    layout->addWidget(titleLabel);

    // таблица
    auto* tableWidget = new QWidget;
    auto* grid = new QGridLayout(tableWidget);

    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    // Заголовки
    grid->addWidget(
        createTableCell("Фаза", true), 0, 0);
    grid->addWidget(
        createTableCell("Ктт", true), 0, 1);
    grid->addWidget(
        createTableCell("Тип ТТ", true), 0, 2);
    grid->addWidget(
        createTableCell("Заводской номер", true), 0, 3);
    grid->addWidget(
        createTableCell("Класс точности", true), 0, 4);

    // Данные ТТ
    int row = 1;
    for (const ActCurrentTransformerData &transformer : transformers)
    {
        grid->addWidget(
            createTableCell(transformer.phase), row, 0);
        grid->addWidget(
            createTableCell(transformer.transformationRatio), row, 1);
        grid->addWidget(
            createTableCell(transformer.name), row, 2);
        grid->addWidget(
            createTableCell(transformer.serialNumber), row, 3);
        grid->addWidget(
            createTableCell(transformer.accuracyClass), row, 4);

        ++row;
    }

    // Пропорции колонок
    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 2);
    grid->setColumnStretch(2, 3);
    grid->setColumnStretch(3, 3);
    grid->setColumnStretch(4, 2);

    layout->addWidget(tableWidget);

    return container;
}

QList<ActCurrentTransformerData> ActViewDialog::currentTransformerByRole(int role) const
{
    QList<ActCurrentTransformerData> result;

    for (const ActCurrentTransformerData &transformer :
        m_data.currentTransformers)
    {
        if (transformer.role == role)
        {
            result.append(transformer);
        }
    }

    auto phaseOrder = [](const QString &phase)
    {
        if (phase == "A")
            return 0;
        if (phase == "B")
            return 1;
        if (phase == "C")
            return 2;
        return 3;
    };

    std::sort(
        result.begin(),
        result.end(),
        [&phaseOrder](
            const ActCurrentTransformerData &a,
            const ActCurrentTransformerData &b)
        {
            return phaseOrder(a.phase) < phaseOrder(b.phase);
        });

    return result;
}

void ActViewDialog::fillVectorDiagram()
{
    ui->vectorDiagramWidget->hide();

    switch (m_data.actTypeId)
    {
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
            break;
        default:
            return;
    }

    ui->vectorDiagramWidget->show();

    if (m_data.vectorDiagram.exists)
    {
        ui->vectorDiagramWidgetLayout->addWidget(
            createVectorDiagramTable(m_data.vectorDiagram));

        return;
    }

    auto* noLoadLabel = new QLabel("Без нагрузки.");

    QFont font = noLoadLabel->font();
    font.setBold(true);
    noLoadLabel->setFont(font);

    ui->vectorDiagramWidgetLayout->addWidget(noLoadLabel);
}

QWidget * ActViewDialog::createVectorDiagramTable(const VectorDiagramData &diagram)
{

    //------------------------------------------------
    // Заголовок
    //------------------------------------------------
    auto* container = new QWidget();

    auto* layout = new QVBoxLayout(container);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Заголовок
    auto* titleLabel = new QLabel("Векторная диаграмма:");
    titleLabel->setStyleSheet("font-weight: bold;");

    layout->addWidget(titleLabel);

    //------------------------------------------------
    // Таблица фаз
    //------------------------------------------------
    auto* tableWidget = new QWidget;
    auto* grid = new QGridLayout(tableWidget);

    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(0);
    grid->setVerticalSpacing(0);

    // заголовки фаз
    grid->addWidget(
        createTableCell("ф.А", true), 0, 0);
    grid->addWidget(
        createTableCell("ф.В", true), 0, 1);
    grid->addWidget(
        createTableCell("ф.С", true), 0, 2);

    // Фаза А
    const QString phaseA =
        QString("%1 мА - %2%3")
            .arg(QString::number(diagram.ia, 'f', 0))
            .arg(QString::number(diagram.angleA, 'f', 0))
            .arg(diagram.angleAType);

    // Фаза B
    const QString phaseB =
        QString("%1 мА - %2%3")
            .arg(QString::number(diagram.ib, 'f', 0))
            .arg(QString::number(diagram.angleB, 'f', 0))
            .arg(diagram.angleBType);
    // Фаза C
    const QString phaseC =
        QString("%1 мА - %2%3")
            .arg(QString::number(diagram.ic, 'f', 0))
            .arg(QString::number(diagram.angleC, 'f', 0))
            .arg(diagram.angleCType);

    grid->addWidget(
        createTableCell(phaseA), 1, 0);
    grid->addWidget(
        createTableCell(phaseB), 1, 1);
    grid->addWidget(
        createTableCell(phaseC), 1, 2);

    grid->setColumnStretch(0 , 1);
    grid->setColumnStretch(1 , 1);
    grid->setColumnStretch(2 , 1);

    layout->addWidget(tableWidget);

    //----------------------------------------------------
    // Напряжение
    //----------------------------------------------------
    auto* voltageLabel = new QLabel(
        QString(
            "Напряжение: Uab = %1 В; Ubc = %2 В; Uca = %3 В.")
                .arg(QString::number(diagram.uab, 'f',1))
                .arg(QString::number(diagram.ubc, 'f',1))
                .arg(QString::number(diagram.uca, 'f',1)));

    voltageLabel->setWordWrap(true);

    layout->addWidget(voltageLabel);

    return container;
}

void ActViewDialog::fillReplacementInfo()
{
    ui->replacementInfoLabel->hide();

    // Время замены относится только к замене прибора учета
    if (m_data.actTypeId != 2)
    {
        return;
    }

    // Если нет векторной то и учета никакого нет
    if (!m_data.vectorDiagram.exists)
    {
        return;
    }

    bool voltageOk = false;

    const double voltageKv = m_data.voltage.toDouble(&voltageOk);

    if (!voltageOk || voltageKv <= 0.0)
    {
        ui->replacementInfoLabel->setText(
            "Не удалось рассчитать недоучтенную электроэнергию: "
            "некорректное напряжение присоединения.");

        ui->replacementInfoLabel->show();
        return;
    }

    double powerMW = 0.0;

    if (!ActCalculator::calculatePrimaryActivePowerMW(
        m_data.vectorDiagram,
        m_data.ctRatio,
        voltageKv,
        powerMW))
    {
        ui->replacementInfoLabel->setText(
            "Не удалось рассчитать недоучтенную электроэнергию: "
            "проверьте Ктт и напряжение.");

        ui->replacementInfoLabel->show();
        return;
    }

    const double energyKWh =
        ActCalculator::calculateUnmeteredEnergyKWh(
            powerMW, m_data.replacementDurationMinutes);

    const QString text =
        QString(
            "Время замены прибора учета ≈%1 мин. "
            "При нагрузке присоединения Р≈%2 МВт "
            "недоучет составил ≈%3 кВт·ч.")
            .arg(m_data.replacementDurationMinutes)
            .arg(powerMW, 0, 'f', 3)
            .arg(energyKWh, 0, 'f', 3);

    ui->replacementInfoLabel->setText(text);
    ui->replacementInfoLabel->show();
}

void ActViewDialog::fillRepresentatives()
{

    // Если сторонних представителей нет,
    // Верхний блок не нужен вообще
    if (m_data.externalRepresentatives.isEmpty())
    {
        ui->representativesWidget->hide();
        return;
    }




    // Представитель предприятия
    ui->representativesLabel->setText(
        QString("Представитель ТОО \"Межрегионэнерготрансит\": %1 %2\n"
                "В присутствии представителей ")
            .arg(m_data.employeePosition, m_data.employeeName));

    ui->representativesLabel->setWordWrap(true);



    // Сторонние представители
    for (const ExternalRepresentativeData &representative :
        m_data.externalRepresentatives)
    {
        auto* representativeLabel = new QLabel(
            QString("%1: %2 %3,")
                .arg(
                    representative.organization,
                    representative.position,
                    representative.shortName));

        representativeLabel->setWordWrap(true);

        ui->representativesWidgetLayout->addWidget(representativeLabel);
    }

    ui->representativesWidgetLayout->setContentsMargins(0, 6, 0, 6);
    ui->representativesWidgetLayout->setSpacing(4);

    ui->representativesWidget->show();
}

void ActViewDialog::fillSignature()
{
    QString employeeSignatureTitle;

    // Повторяем ту же логику, что используется
    // при формировании DOCX
    if (m_data.externalRepresentatives.isEmpty())
    {
        employeeSignatureTitle = m_data.employeePosition;
    }
    else
    {
        employeeSignatureTitle = "Представитель ТОО \"Межрегионэнерготранзит\"";
    }

    int row = 0;

    // Представитель предприятия
    addSignatureRow(employeeSignatureTitle, m_data.employeeName, row++);

    // Сторонние представители
    for (const ExternalRepresentativeData &representative :
        m_data.externalRepresentatives)
    {
        addSignatureRow("Представитель " + representative.organization,
            representative.shortName,
            row++);
    }

    ui->signatureLayout->setColumnStretch(0, 5);
    ui->signatureLayout->setColumnStretch(1, 4);
    ui->signatureLayout->setColumnStretch(2, 0);

    ui->signatureLayout->setHorizontalSpacing(15);
    ui->signatureLayout->setVerticalSpacing(10);

    ui->signatureLayout->setContentsMargins(0, 6, 0, 6);
    ui->signatureLayout->setSpacing(4);

    ui->signatureWidget->show();
}

void ActViewDialog::addSignatureRow(const QString &title, const QString &name, int row)
{

    auto* titleLabel = new QLabel(title);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Место для подписи
    auto* signatureLabel = new QLabel("_______________");
    signatureLabel->setAlignment(Qt::AlignCenter);

    // ФИО
    auto* nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    ui->signatureLayout->addWidget(titleLabel, row, 0);
    ui->signatureLayout->addWidget(signatureLabel, row, 1);
    ui->signatureLayout->addWidget(nameLabel, row, 2);

}

void ActViewDialog::generateDocx()
{
    QString suggestedFileName =
        QString("Акт_%1_%2.docx")
            .arg(m_data.id)
            .arg(m_data.date.toString("dd-MM-yyyy"));

    QString outputPath =
        QFileDialog::getSaveFileName(this, "Сохранить акт",
            suggestedFileName, "Документ Word (*.docx)");

    // Пользователь нажал "Отмена"
    if (outputPath.isEmpty())
    {
        return;
    }

    // На случай если пользователь вручную удалил расширение
    if (!outputPath.endsWith(".docx", Qt::CaseInsensitive))
    {
        outputPath += ".docx";
    }

    DocxGenerator generator;

    if (!generator.generate(m_data, outputPath))
    {
        QMessageBox::critical(this, "Ошибка формирования DOCX", generator.lastError());
        return;
    }

    QMessageBox::information(this, "Готово",
        "Документ успешно сформирован: " + outputPath);
}

QString ActViewDialog::workScheduleText() const
{
    switch (m_data.workScheduleType)
    {
        case 1:
            return "плановая";
        case 2:
            return "внеплановая";
        default:
            return {};
    }
}



















