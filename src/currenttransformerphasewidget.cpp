//
// Created by RZAbook1 on 09.10.2026.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CurrentTransformerPhaseWidget.h" resolved

#include "currenttransformerphasewidget.h"
#include "ui_currenttransformerphasewidget.h"


CurrentTransformerPhaseWidget::CurrentTransformerPhaseWidget(
    const QString &phase,
    QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CurrentTransformerPhaseWidget),
      m_phase(phase)
{
    ui->setupUi(this);

    ui->phaseGroupBox->setTitle("Фаза " + m_phase);

    connect(ui->addButton, &QPushButton::clicked, this,
        [this]()
        {
            setState(State::Editing);
        });

    connect(ui->cancelButton, &QPushButton::clicked, this,
        [this]()
        {
            setState(State::Empty);
        });

    connect(ui->editButton, &QPushButton::clicked, this,
        [this]()
        {
            setState(State::Editing);
        });

    connect(ui->clearButton, &QPushButton::clicked, this,
        [this]()
        {
            setState(State::Empty);
        });
}

CurrentTransformerPhaseWidget::~CurrentTransformerPhaseWidget()
{
    delete ui;
}

QString CurrentTransformerPhaseWidget::phase() const
{
    return m_phase;
}

CurrentTransformerPhaseWidget::State CurrentTransformerPhaseWidget::state() const
{
    return m_state;
}

void CurrentTransformerPhaseWidget::setState(State state)
{
    m_state = state;

    switch (m_state)
    {
        case State::Empty:
            ui->stateStackedWidget->setCurrentWidget(ui->emptyPage);
            break;

        case State::Editing:
            ui->stateStackedWidget->setCurrentWidget(ui->editPage);
            break;

        case State::Filled:
            ui->stateStackedWidget->setCurrentWidget(ui->filledPage);
            break;
    }
}


















