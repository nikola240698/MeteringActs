
#include "editactdialog.h"
#include "ui_editactdialog.h"
#include "createactwidget.h"


EditActDialog::EditActDialog(Database &database, int actId, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditActDialog)
{
    ui->setupUi(this);

    setWindowTitle("Редактирование акта");

    auto *editor = new CreateActWidget(database, actId, ui->editorContainerWidget);

    ui->editorContainerWidget->layout()->addWidget(editor);

    connect(editor, &CreateActWidget::actUpdated, this, &QDialog::accept);
}

EditActDialog::~EditActDialog()
{
    delete ui;
}
