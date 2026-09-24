#pragma once

#ifndef METERINGACTS_ACTDATA_H
#define METERINGACTS_ACTDATA_H

#include <QString>
#include <QDate>
#include <QList>

// структура показаний прибора учета
struct ActMeterReadingData
{
    int typeId = -1;
    QString code;
    double value = 0.0;
};

// структура приборов учета
struct ActMeterData
{
    int meterId =-1;
    int role = -1;

    QString name;
    QString serialNumber;
    QString accuracyClass;

    int verificationYear = 0;

    QList<ActMeterReadingData> readings;
};

// структура трансформаторов тока
struct ActCurrentTransformerData
{
    int currentTransformerId = -1;
    int role = -1;

    QString phase;
    QString name;
    QString serialNumber;
    QString transformationRatio;
    QString accuracyClass;
};

// структура сторонних представителей
struct ExternalRepresentativeData
{
    QString organization;
    QString shortName;
    QString position;
};

// структура векторной диаграммы
struct VectorDiagramData
{
    bool exists = false;

    double ia = 0.0;
    double angleA = 0.0;
    QString angleAType;

    double ib = 0.0;
    double angleB = 0.0;
    QString angleBType;

    double ic = 0.0;
    double angleC = 0.0;
    QString angleCType;

    double uab = 0.0;
    double ubc = 0.0;
    double uca = 0.0;

};

// Выбор плановой/внеплановой работы
enum WorkScheduleType
{
    PlannedWork = 1,
    UnplannedWork = 2
};

// структура данных акта
struct ActData
{
    int id = -1;
    int actTypeId = -1;

    QString actTypeName;
    QDate date;

    int workScheduleType = 0;

    // Место проведения работ
    int areaId = -1;
    int substationId = -1;
    int connectionId = -1;

    QString areaName;
    QString substationName;
    QString connectionName;
    QString voltage;
    QString ctRatio;

    // Представитель предприятия
    int employeeId = -1;
    QString employeeName;
    QString employeePosition;

    QList<ExternalRepresentativeData> externalRepresentatives;

    // Оборудование
    QList<ActMeterData> meters;
    QList<ActCurrentTransformerData> currentTransformers;

    // Измерения
    VectorDiagramData vectorDiagram;

    int replacementDurationMinutes = 0;

    // Заключение
    QString sealNumber;
    QString reason;
    QString result;
};

#endif //METERINGACTS_ACTDATA_H













