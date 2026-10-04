#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERSWIDGET_H
#define METERINGACTS_CURRENTTRANSFORMERSWIDGET_H

#include <QWidget>

#include "database.h"
#include "actviewdialog.h"

class QSqlQueryModel;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class CurrentTransformersWidget;
}

QT_END_NAMESPACE

class CurrentTransformersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CurrentTransformersWidget(
        Database &database, QWidget *parent = nullptr);

    ~CurrentTransformersWidget() override;

public slots:
    void reloadCurrentTransformers();


private:
    Ui::CurrentTransformersWidget *ui;

    Database &m_database;

    QSqlQueryModel* m_currentTransformerModel = nullptr;
    QSqlQueryModel* m_historyModel = nullptr;

    void loadCurrentTransformers(const QString &searchText = QString());

    void setupCurrentTransformersTable();

    void loadHistory(int currentTransformerId);
    void clearHistory();
    void setupHistoryTable();
    void setupHistoryColumns();

    void updateButtons();

    void deleteCurrentTransformer();

    void openSelectedHistoryAct();
};


#endif //METERINGACTS_CURRENTTRANSFORMERSWIDGET_H












