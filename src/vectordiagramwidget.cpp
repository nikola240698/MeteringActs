
#include <QComboBox>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QLocale>

#include "vectordiagramwidget.h"
#include "ui_VectorDiagramWidget.h"

namespace
{
    double lineEditValue(const QLineEdit* lineEdit)
    {
        QLocale locale(QLocale::Russian);

        bool ok = false;
        double value = locale.toDouble(lineEdit->text(), &ok);

        return ok ? value : 0.0;
    }
}


VectorDiagramWidget::VectorDiagramWidget(QWidget *parent) : QWidget(parent), ui(new Ui::VectorDiagramWidget)
{
    ui->setupUi(this);

    setupValidators();
    setupAngleTypes();
}

VectorDiagramWidget::~VectorDiagramWidget()
{
    delete ui;
}

double VectorDiagramWidget::currentA() const
{
    return lineEditValue(ui->currentALineEdit);
}

double VectorDiagramWidget::currentB() const
{
    return lineEditValue(ui->currentBLineEdit);
}

double VectorDiagramWidget::currentC() const
{
    return lineEditValue(ui->currentCLineEdit);
}

double VectorDiagramWidget::angleA() const
{
    return lineEditValue(ui->angleALineEdit);
}

double VectorDiagramWidget::angleB() const
{
    return lineEditValue(ui->angleBLineEdit);
}

double VectorDiagramWidget::angleC() const
{
    return lineEditValue(ui->angleCLineEdit);
}

QString VectorDiagramWidget::angleAType() const
{
    return ui->angleATypeComboBox->currentText();
}

QString VectorDiagramWidget::angleBType() const
{
    return ui->angleBTypeComboBox->currentText();
}

QString VectorDiagramWidget::angleCType() const
{
    return ui->angleCTypeComboBox->currentText();
}

double VectorDiagramWidget::uab() const
{
    return lineEditValue(ui->uabLineEdit);
}

double VectorDiagramWidget::ubc() const
{
    return lineEditValue(ui->ubcLineEdit);
}

double VectorDiagramWidget::uca() const
{
    return lineEditValue(ui->ucaLineEdit);
}

bool VectorDiagramWidget::validate()
{
    struct Field
    {
        QLineEdit* lineEdit;
        QString name;
    };

    const QList<Field> fields =
    {
        Field{ui->currentALineEdit, "ток фазы А"},
        Field{ui->angleALineEdit, "угол фазы А"},
        Field{ui->currentBLineEdit, "ток фазы B"},
        Field{ui->angleBLineEdit, "угол фазы B"},
        Field{ui->currentCLineEdit, "ток фазы C"},
        Field{ui->angleCLineEdit, "угол фазы C"},
        Field{ui->uabLineEdit, "напряжение Uab"},
        Field{ui->ubcLineEdit, "напряжение Ubc"},
        Field{ui->ucaLineEdit, "напряжение Uca"}
    };

    QLocale locale(QLocale::Russian);

    for (const Field &field : fields)
    {
        if (field.lineEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, "Поле не заполнено",
                "Укажите " + field.name + ".");

            field.lineEdit->setFocus();
            return false;
        }

        bool ok = false;
        locale.toDouble(field.lineEdit->text(), &ok);

        if (!ok)
        {
            QMessageBox::warning(this, "Некорректное значение",
                "Проверьте значение: " + field.name + ".");

            field.lineEdit->setFocus();
            field.lineEdit->selectAll();

            return false;
        }
    }

    return true;
}

void VectorDiagramWidget::clear()
{
    ui->currentALineEdit->clear();
    ui->angleALineEdit->clear();

    ui->currentBLineEdit->clear();
    ui->angleBLineEdit->clear();

    ui->currentCLineEdit->clear();
    ui->angleCLineEdit->clear();

    ui->uabLineEdit->clear();
    ui->ubcLineEdit->clear();
    ui->ucaLineEdit->clear();

    // выставляем пол умолчанию
    ui->angleATypeComboBox->setCurrentText("L");
    ui->angleBTypeComboBox->setCurrentText("L");
    ui->angleCTypeComboBox->setCurrentText("C");
}

void VectorDiagramWidget::setupValidators()
{
    QLocale locale(QLocale::Russian);

    auto setupValidator =
        [this, &locale](QLineEdit* lineEdit, double minimum, double maximum)
        {
            auto* validator = new QDoubleValidator(minimum, maximum, 2, lineEdit);

            validator->setLocale(locale);
            validator->setNotation(QDoubleValidator::StandardNotation);
            lineEdit->setValidator(validator);
        };

    // Токи, мА
    setupValidator(ui->currentALineEdit, 0.0, 10000.0);
    setupValidator(ui->currentBLineEdit, 0.0, 10000.0);
    setupValidator(ui->currentCLineEdit, 0.0, 10000.0);

    // Углы
    setupValidator(ui->angleALineEdit, 0.0, 180.0);
    setupValidator(ui->angleBLineEdit, 0.0, 180.0);
    setupValidator(ui->angleCLineEdit, 0.0, 180.0);

    // Напряжения
    setupValidator(ui->uabLineEdit, 0.0, 600.0);
    setupValidator(ui->ubcLineEdit, 0.0, 600.0);
    setupValidator(ui->ucaLineEdit, 0.0, 600.0);

}

void VectorDiagramWidget::setupAngleTypes()
{
    ui->angleATypeComboBox->clear();
    ui->angleBTypeComboBox->clear();
    ui->angleCTypeComboBox->clear();

    ui->angleATypeComboBox->addItems({"L", "C"});
    ui->angleBTypeComboBox->addItems({"L", "C"});
    ui->angleCTypeComboBox->addItems({"L", "C"});

    // выставляем пол умолчанию
    ui->angleATypeComboBox->setCurrentText("L");
    ui->angleBTypeComboBox->setCurrentText("L");
    ui->angleCTypeComboBox->setCurrentText("C");
}
