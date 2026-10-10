
#include "currenttransformerphasepanel.h"
#include "currenttransformerphasewidget.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>

CurrentTransformerPhasePanel::CurrentTransformerPhasePanel(
    Database &database,
    QWidget *parent)
    : QWidget(parent),
      m_database(database)
{


    auto *layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_phaseA = new CurrentTransformerPhaseWidget(
        m_database, "A", this);
    m_phaseB = new CurrentTransformerPhaseWidget(
        m_database, "B", this);
    m_phaseC = new CurrentTransformerPhaseWidget(
        m_database, "C", this);

    layout->addWidget(m_phaseA);
    layout->addWidget(m_phaseB);
    layout->addWidget(m_phaseC);

    layout->setStretch(0, 1);
    layout->setStretch(1, 1);
    layout->setStretch(2, 1);

    connect(m_phaseA, &CurrentTransformerPhaseWidget::currentTransformerCreated,
        this, &CurrentTransformerPhasePanel::currentTransformerCreated);

    connect(m_phaseB, &CurrentTransformerPhaseWidget::currentTransformerCreated,
        this, &CurrentTransformerPhasePanel::currentTransformerCreated);

    connect(m_phaseC, &CurrentTransformerPhaseWidget::currentTransformerCreated,
        this, &CurrentTransformerPhasePanel::currentTransformerCreated);
}



CurrentTransformerPhaseWidget * CurrentTransformerPhasePanel::phaseA() const
{
    return m_phaseA;
}

CurrentTransformerPhaseWidget * CurrentTransformerPhasePanel::phaseB() const
{
    return m_phaseB;
}

CurrentTransformerPhaseWidget * CurrentTransformerPhasePanel::phaseC() const
{
    return m_phaseC;
}

void CurrentTransformerPhasePanel::clear()
{
    m_phaseA->clear();
    m_phaseB->clear();
    m_phaseC->clear();
}

void CurrentTransformerPhasePanel::setData(const QList<ActCurrentTransformerData> &transformers)
{
    clear();

    for (const auto &transformer : transformers)
    {
        if (transformer.phase == "A")
        {
            m_phaseA->setData(transformer);
        }
        else if (transformer.phase == "B")
        {
            m_phaseB->setData(transformer);
        }
        else if (transformer.phase == "C")
        {
            m_phaseC->setData(transformer);
        }
    }
}

QList<ActCurrentTransformerData> CurrentTransformerPhasePanel::data(int role) const
{
    QList<ActCurrentTransformerData> result;

    if (m_phaseA->hasCurrentTransformer())
    {
        result.append(m_phaseA->data(role));
    }
    if (m_phaseB->hasCurrentTransformer())
    {
        result.append(m_phaseB->data(role));
    }
    if (m_phaseC->hasCurrentTransformer())
    {
        result.append(m_phaseC->data(role));
    }

    return result;
}

bool CurrentTransformerPhasePanel::validate()
{
    if (isEmpty())
    {
        QMessageBox::warning(this, "Не добавлены трансформаторы тока",
            "Выберите хотя бы один трансформатор тока.");

        return false;
    }

    return true;
}
