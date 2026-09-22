
#include "actcalculator.h"

#include <cmath>

bool ActCalculator::parseCtRatio(const QString &text, double &ratio)
{
    QString value = text.trimmed();

    value.remove(' ');
    value.replace(',', '.');

    const QStringList parts = value.split('/');

    if (parts.size() != 2)
    {
        return false;
    }

    bool primaryOk = false;
    bool secondaryOk = false;

    const double primary = parts.at(0).toDouble(&primaryOk);

    const double secondary = parts.at(1).toDouble(&secondaryOk);

    if (!primaryOk || !secondaryOk || primary <= 0.0 || secondary <= 0.0)
    {
        return false;
    }

    ratio = primary / secondary;

    return true;
}

double ActCalculator::calculateSecondaryActivePower(const VectorDiagramData &diagram)
{
    // Абсолютные углы токов относительно Uab
    const double currentAngleA =
        currentAngle(diagram.angleA, diagram.angleAType);
    const double currentAngleB =
        currentAngle(diagram.angleB, diagram.angleBType);
    const double currentAngleC =
        currentAngle(diagram.angleC, diagram.angleCType);

    // Фазные напряжения относительно Uab
    constexpr double voltageAngleA = -30.0;
    constexpr double voltageAngleB = -150.0;
    constexpr double voltageAngleC = 90.0;

    // Углы между фазным напряжением и током
    const double phiA = currentAngleA - voltageAngleA;
    const double phiB = currentAngleB - voltageAngleB;
    const double phiC = currentAngleC - voltageAngleC;

    // Перевод токов в А
    const double currentA = diagram.ia / 1000.0;
    const double currentB = diagram.ib / 1000.0;
    const double currentC = diagram.ic / 1000.0;

    // Получаем фазные напряжения
    const double sqrt3 = std::sqrt(3.0);

    const double voltageA = diagram.uab / sqrt3;
    const double voltageB = diagram.ubc / sqrt3;
    const double voltageC = diagram.uca / sqrt3;

    const double powerA = voltageA * currentA * std::cos(degreeToRadians(phiA));
    const double powerB = voltageB * currentB * std::cos(degreeToRadians(phiB));
    const double powerC = voltageC * currentC * std::cos(degreeToRadians(phiC));

    return powerA + powerB + powerC;


}

bool ActCalculator::calculatePrimaryActivePowerMW(const VectorDiagramData &diagram, const QString &ctRatio,
    double connectionVoltageKv, double &powerMW)
{
    double currentRatio = 0.0;

    if (!parseCtRatio(ctRatio, currentRatio))
    {
        return false;
    }

    if (connectionVoltageKv <= 0.0)
    {
        return false;
    }
    const double secondaryPower = calculateSecondaryActivePower(diagram);

    // Переводим напряжение кВ -> В
    const double primaryVoltage = connectionVoltageKv * 1000.0;

    const double secondaryVoltage = 100.0;

    const double voltageRatio = primaryVoltage / secondaryVoltage;

    const double primaryPowerW = secondaryPower * currentRatio * voltageRatio;

    powerMW = primaryPowerW / 1'000'000.0;

    return true;
}

double ActCalculator::calculateUnmeteredEnergyKWh(double powerMW, int durationMinutes)
{
    if (durationMinutes <= 0)
    {
        return 0.0;
    }

    const double powerKW = powerMW * 1000.0;

    const double durationHours = static_cast<double>(durationMinutes) / 60.0;

    return powerKW * durationHours;
}

double ActCalculator::currentAngle(double angle, const QString &angleType)
{
    if (angleType.compare("L", Qt::CaseInsensitive) == 0)
    {
        return -angle;
    }
    if (angleType.compare("C", Qt::CaseInsensitive) == 0)
    {
        return angle;
    }

    return 0.0;

}

double ActCalculator::degreeToRadians(double degrees)
{
    constexpr double Pi = 3.14159265358979323846;

    return degrees * Pi / 180.0;
}
