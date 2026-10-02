#pragma once

#ifndef METERINGACTS_METERSWIDGET_H
#define METERINGACTS_METERSWIDGET_H

#include <QWidget>

#include "database.h"

class QSqlQueryModel;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MetersWidget;
}

QT_END_NAMESPACE

class MetersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MetersWidget(Database &database, QWidget *parent = nullptr);

    ~MetersWidget() override;

private:
    Ui::MetersWidget *ui;

    Database &m_database;

    QSqlQueryModel* m_metersModel = nullptr;

    void loadMeters(const QString &searchText = QString());
    void setupMetersTable();

    void updateButtons();

    void deleteSelectedMeter();
};


#endif //METERINGACTS_METERSWIDGET_H