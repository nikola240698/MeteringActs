
#include "aboutdialog.h"

#include <QCoreApplication>
#include <QPixmap>

#include "ui_aboutdialog.h"


AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->versionLabel->setText(
        QString("Версия %1")
            .arg(QCoreApplication::applicationVersion()));

    QPixmap logo(":/images/logo.png");

    ui->logoLabel->setPixmap(
        logo.scaled(ui->logoLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            )
    );

    connect(ui->closeButton, &QPushButton::clicked,
        this, &QDialog::accept);




}

AboutDialog::~AboutDialog()
{
    delete ui;
}