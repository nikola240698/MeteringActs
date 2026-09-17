
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

// геттер id текущего трансформатора тока
int CurrentTransformerActWidget::currentTransformerId() const
{
    return m_currentTransformerId;
}

// геттер фазы трансформатора тока
QString CurrentTransformerActWidget::phase() const
{
    return ui->phaseComboBox->currentData().toString();
}

// геттер типа трансформатора тока
QString CurrentTransformerActWidget::name() const
{
    return ui->nameLineEdit->text().trimmed();
}

// геттер серийного номера ТТ
QString CurrentTransformerActWidget::serialNumber() const
{
    return ui->serialNumberLineEdit->text().trimmed();
}

// геттер Ктт ТТ
QString CurrentTransformerActWidget::transformerRatio() const
{
    return ui->ratioLineEdit->text().trimmed();
}

// геттер класса точности ТТ
QString CurrentTransformerActWidget::accuracyClass() const
{
    return ui->accuracyLineEdit->text().trimmed();
}

// метод проверки правильности ввода данных
bool CurrentTransformerActWidget::validate()
{
    // проверяем, что выбран какой-либо ТТ
    if (m_currentTransformerId < 0)
    {
        QMessageBox::warning(this, "Трансформатор тока не выбран",
            "Найдите трансформатор тока по заводскому номеру.");
        ui->serialNumberLineEdit->setFocus();
        return false;
    }
    // проверяем, что ввели фазу ТТ
    if (phase().isEmpty())
    {
        QMessageBox::warning(this, "Не выбрана фаза",
            "Выберите фазу трансформатора тока.");

        ui->phaseComboBox->setFocus();
        return false;
    }
    return true;
}

// метод очистки виджета ТТ
void CurrentTransformerActWidget::clear()
{
    // сбрасываем id ТТ
    m_currentTransformerId = -1;
    // сбрасываем указатель фазы
    ui->phaseComboBox->setCurrentIndex(0);
    // очищаем поля ввода
    ui->serialNumberLineEdit->clear();
    ui->nameLineEdit->clear();
    ui->ratioLineEdit->clear();
    ui->accuracyLineEdit->clear();
}

// метод поиска ТТ
void CurrentTransformerActWidget::findCurrentTransformer()
{
    // получаем серийный номер ТТ
    QString serialNumber = ui->serialNumberLineEdit->text().trimmed();
    // проверяем, что он не пустой
    if (serialNumber.isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Введите заводской номер трансформатора тока.");
        ui->serialNumberLineEdit->setFocus();
        return;
    }
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id "
        "FROM current_transformers "
        "WHERE serial_number = :serialNumber");
    // биндим серийный номер
    query.bindValue(":serialNumber", serialNumber);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось выполнить поиск трансформатора тока: "
            + query.lastError().text());
        return;
    }
    // проверяем, что что-нибудь нашлось
    if (!query.next())
    {
        QMessageBox::information(this, "Трансформатор тока не найден",
            "Трансформатор тока с заводсикм номером " + serialNumber
            + " не найден.");

        return;
    }
    // получаем id найденного ТТ
    int currentTransformerId = query.value("id").toInt();
    // загружаем данные выбранного ТТ
    loadCurrentTransformer(currentTransformerId);
}

// метод загрузки данных выбранного ТТ
void CurrentTransformerActWidget::loadCurrentTransformer(int currentTransformerId)
{
    // создаем запрос и подготавливаем его
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
    // биндим id ТТ
    query.bindValue(":id", currentTransformerId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        QMessageBox::warning(this, "Ошибка базы данных",
            "Не удалось загрузить трансформатор тока: "
            + query.lastError().text());

        return;
    }
    // проверяем, что что-то нашлось
    if (!query.next())
        return;
    // получаем id ТТ
    m_currentTransformerId = query.value("id").toInt();
    // получаем и вставляем в поля параметры ТТ
    ui->serialNumberLineEdit->setText(
        query.value("serial_number").toString());
    ui->nameLineEdit->setText(
        query.value("name").toString());
    ui->ratioLineEdit->setText(
        query.value("transformation_ratio").toString());
    ui->accuracyLineEdit->setText(
        query.value("accuracy_class").toString());
}

// метод сброса найденного ТТ
void CurrentTransformerActWidget::resetCurrentTransformer()
{
    // сбрасываем id
    m_currentTransformerId = -1;
    // очищаем поля
    ui->nameLineEdit->clear();
    ui->ratioLineEdit->clear();
    ui->accuracyLineEdit->clear();
}




















