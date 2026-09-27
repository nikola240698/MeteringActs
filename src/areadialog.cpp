
#include "areadialog.h"
#include "ui_areadialog.h"


AreaDialog::AreaDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AreaDialog)
{
    ui->setupUi(this);
}

AreaDialog::~AreaDialog()
{
    delete ui;
}