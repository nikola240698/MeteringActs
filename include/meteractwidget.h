#pragma once

#ifndef METERINGACTS_METERACTWIDGET_H
#define METERINGACTS_METERACTWIDGET_H

#include <QWidget>
#include <QList>
#include <QDoubleValidator>
#include <QLocale>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>


#include "database.h"
#include "meterdialog.h"
#include "actdata.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MeterActWidget;
}

QT_END_NAMESPACE

class MeterActWidget : public QWidget
{
    Q_OBJECT

public:

    // структура для упрощения сохранения показаний
    struct MeterReading
    {
        int typeId;
        double value;
    };


    explicit MeterActWidget(Database &database, QWidget *parent = nullptr);

    ~MeterActWidget() override;

    int meterId() const;

    QString meterName() const;
    QString serialNumber() const;
    QString accuracyClass() const;

    int verificationYear() const;



    QList<MeterReading> readings() const;

    bool validate();
    // полная очистка формы
    void clear();

    // Метод загрузки данных
    void setData(const ActMeterData &data);

private:
    Ui::MeterActWidget *ui;

    Database &m_database;

    int m_meterId = -1;

    void findMeterBySerial();
    void loadMeterData(int meterId);
    void setupReadings();
    // очистка параметров прибора
    void resetMeter();
};


#endif //METERINGACTS_METERACTWIDGET_H