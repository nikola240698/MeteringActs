
#include "externalrepresentativewidget.h"

#include <QMessageBox>

#include "ui_externalrepresentativewidget.h"


ExternalRepresentativeWidget::ExternalRepresentativeWidget(QWidget *parent) : QWidget(parent),
                                                                              ui(new Ui::ExternalRepresentativeWidget)
{
    ui->setupUi(this);

    connect(ui->removeButton, &QPushButton::clicked, this,
        [this]()
        {
            emit removeRequested();
        });
}

ExternalRepresentativeWidget::~ExternalRepresentativeWidget()
{
    delete ui;
}

QString ExternalRepresentativeWidget::organization() const
{
    return ui->organizationLineEdit->text().trimmed();
}

QString ExternalRepresentativeWidget::position() const
{
    return ui->positionLineEdit->text().trimmed();
}

QString ExternalRepresentativeWidget::shortName() const
{
    return ui->nameLineEdit->text().trimmed();
}

bool ExternalRepresentativeWidget::validate()
{
    if (organization().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите организацию стороннего представителя.");
        ui->organizationLineEdit->setFocus();
        return false;
    }

    if (position().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите должность стороннего представителя.");
        ui->positionLineEdit->setFocus();
        return false;
    }

    if (shortName().isEmpty())
    {
        QMessageBox::warning(this, "Не заполнено поле",
            "Укажите фамилию и инициалы стороннего представителя");
        ui->nameLineEdit->setFocus();
        return false;
    }

    return true;
}


















