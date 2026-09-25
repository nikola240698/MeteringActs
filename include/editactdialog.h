#pragma once

#ifndef METERINGACTS_EDITACTDIALOG_H
#define METERINGACTS_EDITACTDIALOG_H

#include <QDialog>

#include "database.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class EditActDialog;
}

QT_END_NAMESPACE

class EditActDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditActDialog(Database &database, int actId, QWidget *parent = nullptr);

    ~EditActDialog() override;

private:
    Ui::EditActDialog *ui;
};


#endif //METERINGACTS_EDITACTDIALOG_H