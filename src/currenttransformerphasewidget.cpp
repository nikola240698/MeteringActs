
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

#include "currenttransformerphasewidget.h"
#include "currenttransformerdialog.h"
#include "actviewdialog.h"
#include "ui_currenttransformerphasewidget.h"


CurrentTransformerPhaseWidget::CurrentTransformerPhaseWidget(
    Database &database,
    const QString &phase,
    QWidget *parent)
    : QWidget(parent),
      ui(new Ui::CurrentTransformerPhaseWidget),
      m_database(database),
      m_phase(phase)
{
    ui->setupUi(this);

    ui->phaseGroupBox->setTitle("Фаза " + m_phase);

    ui->serialNumberLineEdit->setPlaceholderText(
        "Введите заводской номер...");

    setState(State::Empty);

    connect(ui->addButton, &QPushButton::clicked, this,
        [this]()
        {
            m_previousCurrentTransformerId = -1;
            resetCurrentTransformer();
            setState(State::Editing);
            ui->serialNumberLineEdit->setFocus();
        });

    connect(ui->cancelButton, &QPushButton::clicked, this,
        [this]()
        {
            if (m_previousCurrentTransformerId >= 0)
            {
                // Возвращаем ТТ который был выбран до начала редактирования
                const int previousId = m_previousCurrentTransformerId;

                m_previousCurrentTransformerId = -1;

                loadCurrentTransformer(previousId);
            }
            else
            {
                // Карточка до редактирования была пустой
                resetCurrentTransformer();
                setState(State::Empty);
            }

        });

    connect(ui->editButton, &QPushButton::clicked, this,
        [this]()
        {

            // Запоминаем ID выбранного ТТ на случай отмены
            m_previousCurrentTransformerId = m_currentTransformerId;

            setState(State::Editing);

            ui->serialNumberLineEdit->setFocus();
            ui->serialNumberLineEdit->selectAll();
        });

    connect(ui->clearButton, &QPushButton::clicked, this,
        [this]()
        {
            m_previousCurrentTransformerId = -1;

            resetCurrentTransformer();
            setState(State::Empty);
        });

    connect(ui->findButton, &QPushButton::clicked,
        this, &CurrentTransformerPhaseWidget::findCurrentTransformer);

    connect(ui->serialNumberLineEdit, &QLineEdit::returnPressed,
        this, &CurrentTransformerPhaseWidget::findCurrentTransformer);

    connect(ui->serialNumberLineEdit, &QLineEdit::textEdited, this,
        [this]()
        {
            m_currentTransformerId = -1;

            ui->nameValueLabel->setText("-");
            ui->ratioValueLabel->setText("-");
            ui->accuracyValuelabel->setText("-");
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

int CurrentTransformerPhaseWidget::currentTransformerId() const
{
    return m_currentTransformerId;
}

QString CurrentTransformerPhaseWidget::name() const
{
    return ui->nameValueLabel->text().trimmed();
}

QString CurrentTransformerPhaseWidget::serialNumber() const
{
    return ui->serialNumberLineEdit->text().trimmed();
}

QString CurrentTransformerPhaseWidget::transformerRatio() const
{
    return ui->ratioValueLabel->text().trimmed();
}

QString CurrentTransformerPhaseWidget::accuracyClass() const
{
    return ui->accuracyValuelabel->text().trimmed();
}

bool CurrentTransformerPhaseWidget::hasCurrentTransformer() const
{
    return m_currentTransformerId >= 0;
}

bool CurrentTransformerPhaseWidget::validate()
{
    if (m_currentTransformerId < 0)
    {
        QMessageBox::warning(this, "Трансформатор тока не выбран",
            "Выберите трансформатор тока для фазы " + m_phase + ".");

        return false;
    }

    return true;
}

void CurrentTransformerPhaseWidget::clear()
{
    m_previousCurrentTransformerId = -1;

    resetCurrentTransformer();
    setState(State::Empty);
}

void CurrentTransformerPhaseWidget::setData(const ActCurrentTransformerData &data)
{
    clear();

    // Карточка отвечает только за свою фазу
    if (data.phase != m_phase)
        return;

    m_currentTransformerId = data.currentTransformerId;
    ui->serialNumberLineEdit->setText(data.serialNumber);
    ui->serialValueLabel->setText(data.serialNumber);
    ui->nameValueLabel->setText(data.name);
    ui->ratioValueLabel->setText(data.transformationRatio);
    ui->accuracyValuelabel->setText(data.accuracyClass);

    setState(State::Filled);
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
            ui->stateStackedWidget->setCurrentWidget(ui->dataPage);

            ui->serialNumberLineEdit->setVisible(true);
            ui->findButton->setVisible(true);

            ui->serialValueLabel->setVisible(false);

            ui->cancelButton->setVisible(true);
            ui->editButton->setVisible(false);
            ui->clearButton->setVisible(false);

            break;

        case State::Filled:
            ui->stateStackedWidget->setCurrentWidget(ui->dataPage);

            ui->serialNumberLineEdit->setVisible(false);
            ui->findButton->setVisible(false);

            ui->serialValueLabel->setVisible(true);

            ui->cancelButton->setVisible(false);
            ui->editButton->setVisible(true);
            ui->clearButton->setVisible(true);

            break;
    }
}

void CurrentTransformerPhaseWidget::findCurrentTransformer()
{
    const QString serialNumber =
        ui->serialNumberLineEdit->text().trimmed();

    if (serialNumber.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Введите заводской номер трансформатора тока.");

        ui->serialNumberLineEdit->setFocus();
        return;
    }

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id "
        "FROM current_transformers "
        "WHERE serial_number = :serialNumber");

    query.bindValue(":serialNumber", serialNumber);

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось выполнить поиск трансформаторов тока: "
            + query.lastError().text());

        return;
    }

    if (!query.next())
    {
        const auto answer = QMessageBox::question(
            this, "Трансформатор ток не найден",
            "Трансформатор тока с заводским номером "
            + serialNumber
            + " не найден.\n\nДобавить новый?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);

        if (answer != QMessageBox::Yes)
            return;

        CurrentTransformerDialog dialog(m_database, this);

        dialog.setSerialNumber(serialNumber);

        if (dialog.exec() != QDialog::Accepted)
            return;

        loadCurrentTransformer(dialog.currentTransformerId());

        emit currentTransformerCreated();

        return;
    }

    loadCurrentTransformer(query.value("id").toInt());

}

void CurrentTransformerPhaseWidget::loadCurrentTransformer(int currentTransformerId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
        "id, "
        "name, "
        "serial_number, "
        "transformation_ratio, "
        "accuracy_class "
        "FROM current_transformers "
        "WHERE id = :id;");

    query.bindValue(":id", currentTransformerId);

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось загрузить трансформатор тока: "
            + query.lastError().text());

        return;
    }

    if (!query.next())
        return;

    m_currentTransformerId = query.value("id").toInt();

    ui->serialNumberLineEdit->setText(
        query.value("serial_number").toString());
    ui->serialValueLabel->setText(
        query.value("serial_number").toString());
    ui->nameValueLabel->setText(
        query.value("name").toString());
    ui->ratioValueLabel->setText(
        query.value("transformation_ratio").toString());
    ui->accuracyValuelabel->setText(
        query.value("accuracy_class").toString());

    m_previousCurrentTransformerId = -1;

    setState(State::Filled);
}

void CurrentTransformerPhaseWidget::resetCurrentTransformer()
{
    m_currentTransformerId = -1;

    ui->serialNumberLineEdit->clear();

    ui->serialValueLabel->setText("-");
    ui->nameValueLabel->setText("-");
    ui->ratioValueLabel->setText("-");
    ui->accuracyValuelabel->setText("-");
}


















