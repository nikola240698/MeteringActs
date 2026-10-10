#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERPHASEPANEL_H
#define METERINGACTS_CURRENTTRANSFORMERPHASEPANEL_H

#include <QWidget>
#include <QList>

#include "database.h"
#include "actdata.h"
#include "currenttransformerphasewidget.h"

class CurrentTransformerPhaseWidget;
class QHBoxLayout;

class CurrentTransformerPhasePanel : public QWidget
{
    Q_OBJECT

public:
    explicit CurrentTransformerPhasePanel(
        Database &database,
        QWidget *parent = nullptr);


    CurrentTransformerPhaseWidget* phaseA() const;
    CurrentTransformerPhaseWidget* phaseB() const;
    CurrentTransformerPhaseWidget* phaseC() const;

    void clear();
    void setData(const QList<ActCurrentTransformerData> &transformers);
    QList<ActCurrentTransformerData> data(int role) const;

    bool validate();

    bool isEmpty() const;

signals:
    void currentTransformerCreated();

private:
    Database &m_database;

    CurrentTransformerPhaseWidget* m_phaseA = nullptr;
    CurrentTransformerPhaseWidget* m_phaseB = nullptr;
    CurrentTransformerPhaseWidget* m_phaseC = nullptr;
};

inline bool CurrentTransformerPhasePanel::isEmpty() const
{
    return !m_phaseA->hasCurrentTransformer()
            && !m_phaseB->hasCurrentTransformer()
            && !m_phaseC->hasCurrentTransformer();
}


#endif //METERINGACTS_CURRENTTRANSFORMERPHASEPANEL_H
