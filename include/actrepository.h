#pragma once
#ifndef METERINGACTS_ACTREPOSITORY_H
#define METERINGACTS_ACTREPOSITORY_H

#include "database.h"
#include "actdata.h"

class ActRepository
{
public:
    explicit ActRepository(Database &database);

    bool loadAct(int actId, ActData &data);

    QString lastError() const;

private:
    Database &m_database;
    QString m_lastError;

    bool loadMainData(int actId, ActData &data);
    bool loadExternalRepresentatives(int actId, ActData &data);
    bool loadMeters(int actId, ActData &data);
    bool loadMeterReadings(int actMeterId, ActMeterData &data);
    bool loadCurrentTransformer(int actId, ActData &data);
    bool loadVectorDiagram(int actId, ActData &data);
};




#endif //METERINGACTS_ACTREPOSITORY_H