#pragma once

#ifndef METERINGACTS_CREATEACTWIDGET_H
#define METERINGACTS_CREATEACTWIDGET_H

#include <QWidget>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDate>
#include <QMessageBox>
#include <QList>
#include <QSet>

#include "database.h"
#include "meteractwidget.h"
#include "externalrepresentativewidget.h"
#include "vectordiagramwidget.h"
#include "currenttransformeractwidget.h"


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

    // Роль работы окна ввода акта
    enum class Mode
    {
        Create,
        Edit
    };



    explicit CreateActWidget(Database &database, QWidget *parent = nullptr);

    explicit CreateActWidget(
        Database &database, int actId, QWidget* parent = nullptr);

    ~CreateActWidget() override;



private:
    Ui::CreateActWidget *ui;

    Database &m_database;

    MeterActWidget* m_primaryMeterWidget = nullptr;
    MeterActWidget* m_secondaryMeterWidget = nullptr;

    VectorDiagramWidget* m_vectorDiagramWidget = nullptr;

    // Поля для работы акта в режиме создания
    Mode m_mode = Mode::Create;
    int m_editActId = -1;

    // Коллекция для сторонних представителей
    QList<ExternalRepresentativeWidget *> m_externalRepresentativeWidgets;
    // Коллекции для трансформаторов тока
    QList<CurrentTransformerActWidget *> m_removedCurrentTransformerWidgets;
    QList<CurrentTransformerActWidget *> m_installedCurrentTransformerWidgets;

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
    // Главный метод обновления существующего акта
    bool updateAct();
    // обновление основной записи acts
    bool updateActMainData();
    // Удаление старых дочерних данных акта
    bool deleteActDetails();
    // метод сохранения параметров акта
    int insertAct();
    // метод вставки прибора с привязкой к акту
    int insertActMeter(int actId, const MeterActWidget* meterWidget, int role);
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

    // методы для работы с трансформаторами тока
    void addRemovedCurrentTransformer();
    void addInstalledCurrentTransformer();
    void removeRemovedCurrentTransformer(CurrentTransformerActWidget* transformer);
    void removeInstalledCurrentTransformer(CurrentTransformerActWidget* transformer);

    // универсальный метод проверки заполнения формы ТТ
    bool validateCurrentTransformers(
        const QList<CurrentTransformerActWidget *> &transformers,
        const QString &groupName);
    // Метод проверки на совпадение снятых и установленных ТТ по id
    bool validateCurrentTransformerReplacement();
    // метод сохранения акта трансформаторов тока
    bool insertActCurrentTransformer(
        int actId, CurrentTransformerActWidget* transformer, int role);
    // маленький метод упрощения создания актов ТТ
    bool insertActCurrentTransformers(
        int actId, const QList<CurrentTransformerActWidget *> &transformers, int role);

    // методы для очистки формы
    void clearForm();
    void clearExternalRepresentatives();
    void clearCurrentTransformers();
    // методы очистки каждого окна отдельно
    void clearMainTab();
    void clearEquipmentTab();
    void clearMeasurementTab();
    void clearConclusionTab();

    // метод загрузки акта для редактирования
    void loadActForEditing(int actId);
    // Загружаем данные прибора учета по роли
    const ActMeterData* meterByRole(const ActData &data, int role) const;

};


#endif //METERINGACTS_CREATEACTWIDGET_H