#pragma once

#ifndef METERINGACTS_ACTDATA_H
#define METERINGACTS_ACTDATA_H

#include <QString>
#include <QDate>
#include <QList>

struct ActMeterReadingData
{
    int typeId = -1;
    QString code;
    double value = 0.0;
};

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

struct ExternalRepresentativeData
{
    QString organization;
    QString shortName;
    QString position;
};

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

    double uad = 0.0;
    double ubc = 0.0;
    double uca = 0.0;

};

struct ActData
{
    int id = -1;
    int actTypeId = -1;

    QString actTypeName;
    QDate date;

    // Место проведения работ
    QString areaName;
    QString substationName;
    QString connectionName;
    QString voltage;
    QString ctRatio;

    // Представитель предприятия
    QString employeeName;
    QString employeePosition;

    QList<ExternalRepresentativeData> externalRepresentatives;

    // Оборудование
    QList<ActMeterData> meters;
    QList<ActMeterReadingData> currentTransformers;

    // Измерения
    VectorDiagramData vectorDiagram;

    int replacementDurationMinutes = 0;

    // Заключение
    QString sealNumber;
    QString reason;
    QString result;
};

#endif //METERINGACTS_ACTDATA_H













