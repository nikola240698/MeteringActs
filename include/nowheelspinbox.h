#pragma once

#ifndef METERINGACTS_NOWHEELSPINBOX_H
#define METERINGACTS_NOWHEELSPINBOX_H

#include <QSpinBox>

class QWheelEvent;

class NoWheelSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit NoWheelSpinBox(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};




#endif //METERINGACTS_NOWHEELSPINBOX_H