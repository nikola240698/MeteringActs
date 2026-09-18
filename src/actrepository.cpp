
#include "actrepository.h"

ActRepository::ActRepository(Database &database) : m_database(database)
{
}

bool ActRepository::loadAct(int actId, ActData &data)
{
    m_lastError.clear();

    // Полностью сбрасываем старые данные
    data = ActData{};

    if (actId <= 0)
    {
        m_lastError = "Некорректный id акта.";
        return false;
    }

    if (!loadMainData(actId, data))
        return false;

    if (!loadExternalRepresentatives(actId, data))
        return false;

    if (!loadMeters(actId, data))
        return false;

    if (!loadCurrentTransformer(actId, data))
        return false;

    if (!loadVectorDiagram(actId, data))
        return false;

    return true;
}

QString ActRepository::lastError() const
{
    return m_lastError;
}

bool ActRepository::loadMainData(int actId, ActData &data)
{
}

bool ActRepository::loadExternalRepresentatives(int actId, ActData &data)
{
}

bool ActRepository::loadMeters(int actId, ActData &data)
{
}

bool ActRepository::loadMeterReadings(int actMeterId, ActMeterData &data)
{
}

bool ActRepository::loadCurrentTransformer(int actId, ActData &data)
{
}

bool ActRepository::loadVectorDiagram(int actId, ActData &data)
{
}
