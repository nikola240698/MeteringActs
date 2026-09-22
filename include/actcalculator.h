#pragma once


#ifndef METERINGACTS_ACTCALCULATOR_H
#define METERINGACTS_ACTCALCULATOR_H

#include "actdata.h"

#include <QString>

class ActCalculator
{
public:
    // Получение численного коэффициента из строки вида "400/5".
    static bool parseCtRatio(const QString &text, double &ratio);

    // Активная мощность во вторичных цепях
    static double calculateSecondaryActivePower(const VectorDiagramData &diagram);

    // Активная мощность в первичных величинах
    static bool calculatePrimaryActivePowerMW(
        const VectorDiagramData &diagram,
        const QString &ctRatio,
        double connectionVoltageKv,
        double &powerMW);

    // Недоучтенная энергия, кВт*ч
    static double calculateUnmeteredEnergyKWh(double powerMW, int durationMinutes);

private:
    // Перевод углов
    static double currentAngle(double angle, const QString &angleType);

    static double degreeToRadians(double degrees);
};

#endif //METERINGACTS_ACTCALCULATOR_H