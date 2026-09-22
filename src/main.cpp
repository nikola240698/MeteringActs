#include <QApplication>
#include <QMessageBox>
#include <ui_MainWindow.h>
#include <QString>
#include <QDebug>
#include <QDir>

#include "database.h"
#include "mainwindow.h"
#include "docxgenerator.h"
#include "actrepository.h"
#include "actdata.h"
#include "actcalculator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
/*
    DocxGenerator generator;

    const QString templatePath =
        QDir(QStringLiteral(PROJECT_ROOT)).filePath("templates/act_check.docx");

    if (!generator.unpackTemplate(templatePath))
    {
        qDebug() << "DOCX ERROR: " << generator.lastError();
    } else
    {
        qDebug() << "DOCX unpacked successfully";

        if (!generator.replacePlaceholder("meter_serial", "12345678"))
        {
            qDebug() << "Replace ERROR: " << generator.lastError();
        } else
        {
            qDebug() << "Placeholder replaced successfully";

            const QString outputPath =
                QDir(QStringLiteral(PROJECT_ROOT)).filePath("docx_test_result.docx");

            if (!generator.saveDocument(outputPath))
            {
                qDebug() << "SAve ERROR: " << generator.lastError();
            }
            else
            {
                qDebug() << "DOCX created: " << outputPath;
            }
        }

    }
*/
    Database db;

    if (!db.open())
    {
        QMessageBox::critical(nullptr, "Error", "Failed to open Database.");

        return -1;
    }

    ActRepository repository(db);

    ActData actData;

    // Временно указываем id существующего акта
    const int testActId = 33;

    if (!repository.loadAct(testActId, actData))
    {
        qDebug() << "Repository ERROR: " << repository.lastError();
    }
    else
    {
        DocxGenerator generator;

        const QString templatePath =
            QDir(QStringLiteral(PROJECT_ROOT)).filePath("templates/act_check.docx");

        const QString outputPath =
            QDir(QStringLiteral(PROJECT_ROOT)).filePath("generated_act.docx");

        //---------------------------------------------------------
        qDebug() << "Act ID:" << actData.id;
        qDebug() << "Act type:" << actData.actTypeId;
        qDebug() << "Meters count:" << actData.meters.size();

        for (const ActMeterData &meter : actData.meters)
        {
            qDebug()
                << "Meter ID:" << meter.meterId
                << "Role:" << meter.role
                << "Name:" << meter.name
                << "Serial:" << meter.serialNumber;

            for (const ActMeterReadingData &reading : meter.readings)
            {
                qDebug()
                    << "   Reading:"
                    << reading.code
                    << "="
                    << reading.value;
            }
        }

        //---------------------------------------------------------

        //---------------------------------------------------------
        //  Пробуем рассчитать неучтенную энергию
        //---------------------------------------------------------
        double ctRatio = 0.0;

        qDebug() << "CT ratio: "
            << ActCalculator::parseCtRatio(actData.ctRatio, ctRatio) << ctRatio;

        qDebug() << "Secondary power: "
            << ActCalculator::calculateSecondaryActivePower(actData.vectorDiagram);

        double powerMW = 0.0;

        const bool powerOk =
            ActCalculator::calculatePrimaryActivePowerMW(
                actData.vectorDiagram,
                actData.ctRatio,
                actData.voltage.toDouble(),
                powerMW);

        qDebug() << "Primary power" << powerOk << powerMW;
        qDebug() << "Energy: " << ActCalculator::calculateUnmeteredEnergyKWh(
            powerMW,
            actData.replacementDurationMinutes);
        //----------------------------------------------------------


        if (!generator.generate(actData, outputPath))
        {
            qDebug() << "DOCX ERROR: " << generator.lastError();
        }
        else
        {
            qDebug() << "DOCX generated successfully: " << outputPath;
        }

    }

    MainWindow w(db);
    w.show();

    return QApplication::exec();
}