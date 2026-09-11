
#include "createactwidget.h"
#include "ui_CreateActWidget.h"


CreateActWidget::CreateActWidget(Database &database, QWidget *parent)
        : QWidget(parent), ui(new Ui::CreateActWidget), m_database(database)
{
    ui->setupUi(this);
}

CreateActWidget::~CreateActWidget()
{
    delete ui;
}