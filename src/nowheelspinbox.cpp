
#include "nowheelspinbox.h"

#include <QWheelEvent>

NoWheelSpinBox::NoWheelSpinBox(QWidget *parent) : QSpinBox(parent)
{
}

void NoWheelSpinBox::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
