#pragma once


#ifndef METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H
#define METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H

#include <QWidget>


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

    bool validate();

signals:
    void removeRequested();

private:
    Ui::ExternalRepresentativeWidget *ui;
};


#endif //METERINGACTS_EXTERNALREPRESENTATIVEWIDGET_H