#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H
#define METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H

#include <QWidget>

#include "database.h"
#include "actdata.h"


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
        Database &database,
        const QString &phase,
        QWidget *parent = nullptr);

    ~CurrentTransformerPhaseWidget() override;

    QString phase() const;
    State state() const;

    int currentTransformerId() const;
    QString name() const;
    QString serialNumber() const;
    QString transformerRatio() const;
    QString accuracyClass() const;

    bool hasCurrentTransformer() const;

    bool validate();
    void clear();
    void setData(const ActCurrentTransformerData &data);

    ActCurrentTransformerData data(int role) const;

signals:
    void currentTransformerCreated();

private:
    Ui::CurrentTransformerPhaseWidget *ui;

    QString m_phase;
    State m_state = State::Empty;

    Database &m_database;

    int m_currentTransformerId = -1;
    int m_previousCurrentTransformerId = -1;

    void setState(State state);

    void findCurrentTransformer();
    void loadCurrentTransformer(int currentTransformerId);

    void resetCurrentTransformer();

};

inline ActCurrentTransformerData CurrentTransformerPhaseWidget::data(int role) const
{
    ActCurrentTransformerData result;

    result.currentTransformerId = m_currentTransformerId;

    result.role = role;
    result.phase = m_phase;

    result.name = name();

    result.serialNumber = serialNumber();
    result.transformationRatio = transformerRatio();
    result.accuracyClass = accuracyClass();

    return result;
}


#endif //METERINGACTS_CURRENTTRANSFORMERPHASEWIDGET_H











