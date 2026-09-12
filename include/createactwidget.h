#pragma once

#ifndef METERINGACTS_CREATEACTWIDGET_H
#define METERINGACTS_CREATEACTWIDGET_H

#include <QWidget>

#include "database.h"
#include "meterdialog.h"



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

private:
    Ui::CreateActWidget *ui;

    Database &m_database;

    void loadActTypes() const;
    void loadAreas() const;
    void loadEmployees() const;

    void loadSubstations(int areaId) const;
    void loadConnections(int substationId) const;
    void loadConnectionData(int connectionId) const;

    void loadMeters() const;
    void loadMeterData(int meterId) const;
};


#endif //METERINGACTS_CREATEACTWIDGET_H