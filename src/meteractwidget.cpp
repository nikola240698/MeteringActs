
#include "meteractwidget.h"
#include "ui_meteractwidget.h"


MeterActWidget::MeterActWidget(Database &database, QWidget *parent)
    : QWidget(parent), ui(new Ui::MeterActWidget), m_database(database)
{
    ui->setupUi(this);

    ui->verificationYearLineEdit->setValidator(new QIntValidator(1900, 2100, this));
    // метод настройки поля ввода показаний
    setupReadings();
    // слот нажатия кнопки "Найти"
    connect(ui->findMeterButton, &QPushButton::clicked, this, &MeterActWidget::findMeterBySerial);
    // слот нажатия Enter при вводе серийного номера
    connect(ui->serialNumberLineEdit, &QLineEdit::returnPressed, this,
        [this]()
        {
            findMeterBySerial();
        });
    // слот стирания параметров прибора при изменении введенного серийного номера
    connect(ui->serialNumberLineEdit, &QLineEdit::textEdited, this,
        [this]()
        {
            resetMeter();
        });
    // вводим текст-подсказку в поле
    ui->serialNumberLineEdit->setPlaceholderText("Введите серийный номер...");

}

MeterActWidget::~MeterActWidget()
{
    delete ui;
}

int MeterActWidget::meterId() const
{
    return m_meterId;
}

QString MeterActWidget::meterName() const
{
    return ui->meterNameLineEdit->text().trimmed();
}

QString MeterActWidget::serialNumber() const
{
    return ui->serialNumberLineEdit->text().trimmed();
}

QString MeterActWidget::accuracyClass() const
{
    return ui->accuracyClassLineEdit->text().trimmed();
}

int MeterActWidget::verificationYear() const
{
    return ui->verificationYearLineEdit->text().toInt();
}


QList<MeterActWidget::MeterReading> MeterActWidget::readings() const
{
    QList<MeterReading> result;

    QLocale locale(QLocale::Russian);

    if (ui->activeImportCheckBox->isChecked())
    {
        result.append({1, locale.toDouble(ui->activeImportLineEdit->text())});
    }
    if (ui->activeExportCheckBox->isChecked())
    {
        result.append({2, locale.toDouble(ui->activeExportLineEdit->text())});
    }
    if (ui->reactiveImportCheckBox->isChecked())
    {
        result.append({3, locale.toDouble(ui->reactiveImportLineEdit->text())});
    }
    if (ui->reactiveExportCheckBox->isChecked())
    {
        result.append({4, locale.toDouble(ui->reactiveExportLineEdit->text())});
    }

    return result;
}

bool MeterActWidget::validate()
{
    if (m_meterId < 0)
    {
        QMessageBox::warning(this,
            "Прибор не выбран",
            "Найдите прибор учета по серийному номеру или добавьте новый прибор.");

        ui->serialNumberLineEdit->setFocus();

        return false;
    }

    QString yearText = ui->verificationYearLineEdit->text().trimmed();

    bool yearOk = false;

    int year = yearText.toInt(&yearOk);

    if (!yearOk || year < 1900 || year > 2100)
    {
        QMessageBox::warning(this,
            "Некорректный год",
            "Укажите корректный год поверки.");

        ui->verificationYearLineEdit->setFocus();
        ui->verificationYearLineEdit->selectAll();

        return false;
    }

    if (!ui->activeImportCheckBox->isChecked() &&
        !ui->activeExportCheckBox->isChecked() &&
        !ui->reactiveImportCheckBox->isChecked() &&
        !ui->reactiveExportCheckBox->isChecked())
    {
        QMessageBox::warning(this,
            "Не выбраны показания",
            "Выберите хотя бы один вид показаний.");

        return false;
    }

    QLocale locale(QLocale::Russian);

    auto checkReading =
        [this, &locale](
            QCheckBox* checkBox,
            QLineEdit* lineEdit,
            const QString &name)
        {
            if (!checkBox->isChecked())
                return true;

            QString text = lineEdit->text().trimmed();

            bool ok = false;

            locale.toDouble(text, &ok);

            if (text.isEmpty() || !ok)
            {
                QMessageBox::warning(this,
                    "Некорректное показание",
                    "Введите корректное показание " + name + ".");

                lineEdit->setFocus();
                lineEdit->selectAll();
                return false;
            }

            return true;
        };
    if (!checkReading(ui->activeImportCheckBox, ui->activeImportLineEdit, "A+"))
        return false;
    if (!checkReading(ui->activeExportCheckBox, ui->activeExportLineEdit, "A-"))
        return false;
    if (!checkReading(ui->reactiveImportCheckBox, ui->reactiveImportLineEdit, "R+"))
        return false;
    if (!checkReading(ui->reactiveExportCheckBox, ui->reactiveExportLineEdit, "R-"))
        return false;

    return true;
}

// полная очистка формы
void MeterActWidget::clear()
{
    m_meterId = -1;

    ui->serialNumberLineEdit->clear();
    ui->meterNameLineEdit->clear();
    ui->accuracyClassLineEdit->clear();
    ui->verificationYearLineEdit->clear();
    // отмечаем автоматически выбранный А+
    ui->activeImportCheckBox->setChecked(true);
    ui->activeExportCheckBox->setChecked(false);
    ui->reactiveImportCheckBox->setChecked(false);
    ui->reactiveExportCheckBox->setChecked(false);
    // очищаем все поля
    ui->activeImportLineEdit->clear();
    ui->activeExportLineEdit->clear();
    ui->reactiveImportLineEdit->clear();
    ui->reactiveExportLineEdit->clear();
}

// очистка параметров прибора
void MeterActWidget::resetMeter()
{
    m_meterId = -1;

    ui->meterNameLineEdit->clear();
    ui->accuracyClassLineEdit->clear();
    ui->verificationYearLineEdit->clear();
}

void MeterActWidget::loadMeterData(int meterId)
{
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT name, serial_number, accuracy_class, verification_year "
        "FROM meters "
        "WHERE id = :meterId;");
    // биндим значения в запрос
    query.bindValue(":meterId", meterId);
    // проверяем что запрос выполняется
    if (!query.exec())
    {
        qDebug() << "loadMeterData error: " << query.lastError().text();
        return;
    }
    // проверяем, что нашлось хоть одно значение
    if (!query.next())
        return;
    // вставляем в поля полученные данные
    ui->meterNameLineEdit->setText(query.value("name").toString());
    ui->serialNumberLineEdit->setText(query.value("serial_number").toString());
    ui->accuracyClassLineEdit->setText(query.value("accuracy_class").toString());
    ui->verificationYearLineEdit->setText(query.value("verification_year").toString());

}

// метод поиска прибора по серийному номеру
void MeterActWidget::findMeterBySerial()
{
    //сбрасываем текущий прибор
    m_meterId = -1;
    // очищаем на всякий случай поля
    ui->meterNameLineEdit->clear();
    ui->accuracyClassLineEdit->clear();
    ui->verificationYearLineEdit->clear();

    // получаем серийный номер
    QString serial = ui->serialNumberLineEdit->text().trimmed();
    // если не ввели серийный номер
    if (serial.isEmpty())
        return;
    // создаем запрос и подготавливаем его
    QSqlQuery query(m_database.getDatabase());
    query.prepare(
        "SELECT id, name, serial_number, accuracy_class, verification_year "
        "FROM meters "
        "WHERE serial_number = :serial;");
    // биндим значение
    query.bindValue(":serial", serial);
    // проверяем, что он выполняется
    if (!query.exec())
    {
        qDebug() << "findMeterBySerial error: " << query.lastError().text();
        return;
    }
    // если найдет прибор
    if (query.next())
    {
        // получаем его id
        m_meterId = query.value("id").toInt();
        // вставляем остальные значения
        ui->meterNameLineEdit->setText(query.value("name").toString());
        ui->accuracyClassLineEdit->setText(query.value("accuracy_class").toString());
        ui->verificationYearLineEdit->setText(query.value("verification_year").toString());
        return;
    }
    // если прибор не найден
    auto answer = QMessageBox::question(
        this,
        "Прибор не найден",
        "Прибор с серийным номером " + serial +
        " не найден. \n\nДобавить новый прибор?",
        QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
    {
        MeterDialog dialog(m_database, serial, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            m_meterId = dialog.createdMeterId();

            loadMeterData(m_meterId);
        }
    }
}

// метод настройки блока ввода показаний
void MeterActWidget::setupReadings()
{
    // отмечаем автоматически выбранный А+
    ui->activeImportCheckBox->setChecked(true);
    // переводим поля в режим соответствующий текущим измерениям
    ui->activeImportLineEdit->setEnabled(true);
    ui->activeExportLineEdit->setEnabled(false);
    ui->reactiveImportLineEdit->setEnabled(false);
    ui->reactiveExportLineEdit->setEnabled(false);
    // слоты связи выбора CheckBox и включения LineEdit
    connect(ui->activeImportCheckBox, &QCheckBox::toggled,
        ui->activeImportLineEdit, &QLineEdit::setEnabled);

    connect(ui->activeExportCheckBox, &QCheckBox::toggled,
        ui->activeExportLineEdit, &QLineEdit::setEnabled);

    connect(ui->reactiveImportCheckBox, &QCheckBox::toggled,
        ui->reactiveImportLineEdit, &QLineEdit::setEnabled);

    connect(ui->reactiveExportCheckBox, &QCheckBox::toggled,
        ui->reactiveExportLineEdit, &QLineEdit::setEnabled);

    // создаем валидатор десятичных чисел
    auto *validator = new QDoubleValidator(0.0,  9999999.999, 3, this);
    // настраиваем валидатор
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale(QLocale::Russian));
    // применяем его к полям
    ui->activeImportLineEdit->setValidator(validator);
    ui->activeExportLineEdit->setValidator(validator);
    ui->reactiveImportLineEdit->setValidator(validator);
    ui->reactiveExportLineEdit->setValidator(validator);

}
