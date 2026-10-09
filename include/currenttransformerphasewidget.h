#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H
#define METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class CurrentTransformerPhaseWidget;
}

QT_END_NAMESPACE

class CurrentTransformerPhaseWidget : public QWidget
{
    Q_OBJECT

public:
    enum class State
    {
        Empty,
        Editing,
        Filled
    };
    explicit CurrentTransformerPhaseWidget(
        const QString &phase,
        QWidget *parent = nullptr);

    ~CurrentTransformerPhaseWidget() override;

    QString phase() const;
    State state() const;

private:
    Ui::CurrentTransformerPhaseWidget *ui;

    QString m_phase;
    State m_state = State::Empty;

    void setState(State state);
};


#endif //METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H











