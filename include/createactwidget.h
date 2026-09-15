#pragma once

#ifndef METERINGACTS_CREATEACTWIDGET_H
#define METERINGACTS_CREATEACTWIDGET_H

#include <QWidget>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDate>
#include <QMessageBox>

#include "database.h"
#include "meterdialog.h"

// структура для упрощения сохранения показаний
struct MeterReading
{
    int typeId;
    double value;
};


QT_BEGIN_NAMESPACE

namespace Ui
{
    class CreateActWidget;
}

QT_END_NAMESPACE

class CreateActWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CreateActWidget(Database &database, QWidget *parent = nullptr);

    ~CreateActWidget() override;

    // метод получения показаний
    QList<MeterReading> getReadings() const;

private:
    Ui::CreateActWidget *ui;

    Database &m_database;

    int m_currentMeterId = -1;

    // метод загрузки типов актов
    void loadActTypes() const;
    // метод загрузки всех ЛПУ
    void loadAreas() const;
    // метод загрузки представителей предприятия
    void loadEmployees() const;
    // метод загрузки подстанций
    void loadSubstations(int areaId) const;
    // метод загрузки присоединений
    void loadConnections(int substationId) const;
    // метод загрузки параметров присоединения
    void loadConnectionData(int connectionId) const;
    // метод загрузки данных прибора
    void loadMeterData(int meterId) const;
    // метод поиска данных прибора по серийному номеру
    void findMeterBySerial();
    // метод настройки ввода показаний
    void setupReadings();
    // метод проверки правильности ввода данных
    bool validateForm();

    // методы сохранения акта
    // главный метод сохранения
    bool saveAct();
    // метод сохранения параметров акта
    int insertAct();
    // метод вставки прибора с привязкой к акту
    int insertActMeter(int actId);
    // метод вставки показаний с привязкой к прибору
    bool insertReadings(int actMeterId);
    // метод обновления актуального года поверки прибора
    bool updateMeterVerificationYear();
};


#endif //METERINGACTS_CREATEACTWIDGET_H