//
// Created by RZAbook1 on 10.11.2026.
//

#ifndef METERINGACTS_READINGCONNECTIONWIDGET_H
#define METERINGACTS_READINGCONNECTIONWIDGET_H

#include <QWidget>

#include "database.h"
#include "meteractwidget.h"
#include "actdata.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
    class ReadingConnectionWidget;
}

QT_END_NAMESPACE

class ReadingConnectionWidget : public QWidget
{
    Q_OBJECT

signals:
    void removeRequested(ReadingConnectionWidget *widget);
    void connectionChanged(ReadingConnectionWidget *widget, int connectionId);

    void meterSelected(ReadingConnectionWidget *widget);


public:
    explicit ReadingConnectionWidget(
        Database &database,
        QWidget *parent = nullptr
    );


    ~ReadingConnectionWidget() override;

    void setSubstation(int substationId);

    // Восстанавливает присоединение, прибор и показания из акта
    bool setData(const ActMeterData& data);

    void setNumber(int number);

    void setRemoveEnabled(bool enabled);

    // Блокирует изменение присоединения при редактировании акта.
    // Прибор учета и его показания остаются доступными для изменения.
    void setConnectionLocked(bool locked);

    int selectedConnectionId() const;
    void setUnavailableConnections(const QSet<int> &connectionIds);

    bool validateMeter();

    QString meterSerialNumber() const;

    // Возвращает виджет прибора для сохранения его данных и показаний
    MeterActWidget* meterWidget() const;

    // Собирает данные присоединения, прибора и показаний
    ActMeterData data() const;

private:
    Ui::ReadingConnectionWidget *ui;

    Database &m_database;

    MeterActWidget *m_meterWidget = nullptr;

    // Историческое название присоединения из сохранённого акта
    QString m_originalConnectionName;

    // ID записи прибора в существующем акте.
    // Не путать с ID прибора из справочника meters.
    int m_actMeterId = -1;

    void loadConnections(int substationId);

    void loadConnectionData(int connectionId);


};


#endif //METERINGACTS_READINGCONNECTIONWIDGET_H