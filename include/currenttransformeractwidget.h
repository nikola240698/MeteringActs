#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERACTWIDGET_H
#define METERINGACTS_CURRENTTRANSFORMERACTWIDGET_H

#include <QWidget>

#include "database.h"
#include "actdata.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class CurrentTransformerActWidget;
}

QT_END_NAMESPACE

class CurrentTransformerActWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CurrentTransformerActWidget(Database &database, QWidget *parent = nullptr);

    ~CurrentTransformerActWidget() override;

    int currentTransformerId() const;

    QString phase() const;
    QString name() const;
    QString serialNumber() const;
    QString transformerRatio() const;
    QString accuracyClass() const;

    bool validate();
    void clear();

    // Загрузка данных для редактирования акта
    void setData(const ActCurrentTransformerData &data);

signals:
    void removeRequested();

private:
    Ui::CurrentTransformerActWidget *ui;

    Database &m_database;

    int m_currentTransformerId = -1;

    void findCurrentTransformer();
    void loadCurrentTransformer(int currentTransformerId);
    void resetCurrentTransformer();
};


#endif //METERINGACTS_CURRENTTRANSFORMERACTWIDGET_H