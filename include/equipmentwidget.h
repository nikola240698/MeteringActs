#pragma once

#ifndef METERINGACTS_EQUIPMENTWIDGET_H
#define METERINGACTS_EQUIPMENTWIDGET_H

#include <QWidget>

#include "database.h"

class MetersWidget;
class CurrentTransformersWidget;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class EquipmentWidget;
}

QT_END_NAMESPACE

class EquipmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EquipmentWidget(
        Database &database, QWidget *parent = nullptr);

    ~EquipmentWidget() override;

    MetersWidget* metersWidget() const;
    CurrentTransformersWidget* currentTransformerWidget() const;

private:
    Ui::EquipmentWidget *ui;

    Database &m_database;

    MetersWidget* m_metersWidget = nullptr;
    CurrentTransformersWidget* m_currentTransformerWidget = nullptr;
};


#endif //METERINGACTS_EQUIPMENTWIDGET_H