#pragma once

#ifndef METERINGACTS_CREATEACTWIDGET_H
#define METERINGACTS_CREATEACTWIDGET_H

#include <QWidget>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDate>
#include <QMessageBox>
#include <QList>

#include "database.h"
#include "meteractwidget.h"
#include "externalrepresentativewidget.h"
#include "vectordiagramwidget.h"



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

    // Роли прибора учета
    enum MeterRole
    {
        CheckedMeter = 1,
        RemovedMeter = 2,
        InstalledMeter = 3,
        ReadingMeter = 4,
        ExistingMeter = 5
    };
    // Роли трансформатора тока
    enum CurrentTransformerRole
    {
        RemovedCurrentTransformer = 1,
        InstalledCurrentTransformer = 2
    };

    explicit CreateActWidget(Database &database, QWidget *parent = nullptr);

    ~CreateActWidget() override;



private:
    Ui::CreateActWidget *ui;

    Database &m_database;

    MeterActWidget* m_primaryMeterWidget = nullptr;
    MeterActWidget* m_secondaryMeterWidget = nullptr;

    VectorDiagramWidget* m_vectorDiagramWidget = nullptr;

    QList<ExternalRepresentativeWidget *> m_externalRepresentativeWidgets;

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

    // метод проверки правильности ввода данных
    bool validateForm();

    // методы сохранения акта
    // главный метод сохранения
    bool saveAct();
    // метод сохранения параметров акта
    int insertAct();
    // метод вставки прибора с привязкой к акту
    int insertActMeter(int actId, MeterActWidget* meterWidget, int role);
    // метод вставки показаний с привязкой к прибору
    bool insertReadings(int actMeterId, MeterActWidget* meterWidget);
    // метод обновления актуального года поверки прибора
    bool updateMeterVerificationYear(MeterActWidget* meterWidget);
    // метод показа/скрытия поля для второго прибора учета
    void updateActTypeUi();
    // метод определения роли первого прибора учета
    int primaryMeterRole() const;

    // метод добавления полей ввода представителя
    void addExternalRepresentative();
    // метод удаления полей представителя
    void removeExternalRepresentative(ExternalRepresentativeWidget *representative);
    // метод сохранения представителей в БД
    bool insertExternalRepresentatives(int actId);

    // метод управления блоком векторной диаграммы
    void updateVectorDiagramUi();
    // метод сохранения векторной диаграммы
    bool insertVectorDiagram(int actId);

};


#endif //METERINGACTS_CREATEACTWIDGET_H