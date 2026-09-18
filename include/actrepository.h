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
    // строка ошибок при работе с БД
    QString m_lastError;
    // метод загрузки основных данных
    bool loadMainData(int actId, ActData &data);
    // метод загрузки сторонних представителей
    bool loadExternalRepresentatives(int actId, ActData &data);
    // метод загрузки данных измерительных приборов
    bool loadMeters(int actId, ActData &data);
    // метод загрузки показаний прибора учета
    bool loadMeterReadings(int actMeterId, ActMeterData &meter);
    // метод загрузки данных ТТ
    bool loadCurrentTransformers(int actId, ActData &data);
    // метод загрузки векторной диаграммы
    bool loadVectorDiagram(int actId, ActData &data);
};




#endif //METERINGACTS_ACTREPOSITORY_H