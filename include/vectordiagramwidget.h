#pragma once

#ifndef METERINGACTS_VECTORDIAGRAMWIDGET_H
#define METERINGACTS_VECTORDIAGRAMWIDGET_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class VectorDiagramWidget;
}

QT_END_NAMESPACE

class VectorDiagramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VectorDiagramWidget(QWidget *parent = nullptr);

    ~VectorDiagramWidget() override;

    double currentA() const;
    double currentB() const;
    double currentC() const;

    double angleA() const;
    double angleB() const;
    double angleC() const;

    QString angleAType() const;
    QString angleBType() const;
    QString angleCType() const;

    double uab() const;
    double ubc() const;
    double uca() const;

    bool validate();
    void clear();

private:
    Ui::VectorDiagramWidget *ui;

    void setupValidators();
    void setupAngleTypes();
};


#endif //METERINGACTS_VECTORDIAGRAMWIDGET_H