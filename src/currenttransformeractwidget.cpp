
#include "currenttransformeractwidget.h"

#include <QMessageBox>

#include "ui_CurrentTransformerActWidget.h"


CurrentTransformerActWidget::CurrentTransformerActWidget(
    Database &database, QWidget *parent)
        : QWidget(parent),
        ui(new Ui::CurrentTransformerActWidget),
        m_database(database)
{
    ui->setupUi(this);

    // заполняем список фаз
    ui->phaseComboBox->addItem("A", "A");
    ui->phaseComboBox->addItem("В", "В");
    ui->phaseComboBox->addItem("С", "С");

    // Подсказка для заводского номера
    ui->serialNumberLineEdit->setPlaceholderText("Введите заводской номер");

    // Поиск по кнопке
    connect(ui->findButton, &QPushButton::clicked, this,
        &CurrentTransformerActWidget::findCurrentTransformer);

    // Поиск по Enter
    connect(ui->serialNumberLineEdit, &QLineEdit::returnPressed, this,
        &CurrentTransformerActWidget::findCurrentTransformer);

    // Если пользователь изменил серийный номер
    // то необходимо сбросить найденный ТТ
    connect(ui->serialNumberLineEdit, &QLineEdit::textEdited, this,
        [this]()
        {
            resetCurrentTransformer();
        });

    // Просим родительский виджет удалить данную строку
    connect(ui->removeButton, &QPushButton::clicked, this,
        [this]()
        {
            emit removeRequested();
        });


}

CurrentTransformerActWidget::~CurrentTransformerActWidget()
{
    delete ui;
}

int CurrentTransformerActWidget::currentTransformerId() const
{
    return m_currentTransformerId;
}

QString CurrentTransformerActWidget::phase() const
{
    return ui->phaseComboBox->currentData().toString();
}

QString CurrentTransformerActWidget::name() const
{
    return ui->nameLineEdit->text().trimmed();
}

QString CurrentTransformerActWidget::serialNumber() const
{
    return ui->serialNumberLineEdit->text().trimmed();
}

QString CurrentTransformerActWidget::transformerRatio() const
{
    return ui->ratioLineEdit->text().trimmed();
}

QString CurrentTransformerActWidget::accuracyClass() const
{
    return ui->accuracyLineEdit->text().trimmed();
}

bool CurrentTransformerActWidget::validate()
{
    if (m_currentTransformerId < 0)
    {
        QMessageBox::warning(this, "Трансформатор тока не выбран",
            "Найдите трансформатор тока по заводскому номеру.");
        ui->serialNumberLineEdit->setFocus();
        return false;
    }

    if (phase().isEmpty())
    {
        QMessageBox::warning(this, "Не выбрана фаза",
            "Выберите фазу трансформатора тока.");

        ui->phaseComboBox->setFocus();
        return false;
    }

    return true;

}

void CurrentTransformerActWidget::clear()
{
    m_currentTransformerId = -1;

    ui->phaseComboBox->setCurrentIndex(0);

    ui->serialNumberLineEdit->clear();
    ui->nameLineEdit->clear();
    ui->ratioLineEdit->clear();
    ui->accuracyLineEdit->clear();
}

void CurrentTransformerActWidget::findCurrentTransformer()
{
    QString serialNumber = ui->serialNumberLineEdit->text().trimmed();

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
        "WHERE serial_nu,ber = :serialNumber");

    query.bindValue(":serialNumber", serialNumber);

    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось выполнить поиск трансформатора тока: "
            + query.lastError().text());
        return;
    }

    if (!query.next())
    {
        QMessageBox::information(this, "Трансформатор тока не найден",
            "Трансформатор тока с заводсикм номером " + serialNumber
            + " не найден.");

        return;
    }

    int currentTransformerId = query.value("id").toInt();

    loadCurrentTransformer(currentTransformerId);
}

void CurrentTransformerActWidget::loadCurrentTransformer(int currentTransformerId)
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

    ui->nameLineEdit->setText(
        query.value("name").toString());

    ui->ratioLineEdit->setText(
        query.value("transformation_ratio").toString());

    ui->accuracyLineEdit->setText(
        query.value("accuracy_class").toString());
}

void CurrentTransformerActWidget::resetCurrentTransformer()
{
    m_currentTransformerId = -1;

    ui->nameLineEdit->clear();
    ui->ratioLineEdit->clear();
    ui->accuracyLineEdit->clear();
}




















