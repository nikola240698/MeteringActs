#pragma once


#ifndef METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H
#define METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H

#include <QWidget>

#include "actdata.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
    class ExternalRepresentativeWidget;
}

QT_END_NAMESPACE

class ExternalRepresentativeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExternalRepresentativeWidget(QWidget *parent = nullptr);

    ~ExternalRepresentativeWidget() override;

    QString organization() const;
    QString position() const;
    QString shortName() const;

    // Возвращает идентификатор записи представителя в акте.
    int id() const;

    bool validate();

    // Загрузка данных для редактирования акта
    void setData(const ExternalRepresentativeData &data);


signals:
    void removeRequested();

private:
    Ui::ExternalRepresentativeWidget *ui;

    // Идентификатор записи в external_representatives.
    // Для нового представителя равен -1.
    int m_id = -1;
};


#endif //METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H