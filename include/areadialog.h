//
// Created by RZAbook1 on 27.09.2026.
//

#ifndef METERINGACTS_AREADIALOG_H
#define METERINGACTS_AREADIALOG_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class AreaDialog;
}

QT_END_NAMESPACE

class AreaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AreaDialog(QWidget *parent = nullptr);

    ~AreaDialog() override;

private:
    Ui::AreaDialog *ui;
};


#endif //METERINGACTS_AREADIALOG_H