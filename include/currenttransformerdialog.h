#pragma once

#ifndef METERINGACTS_CURRENTTRANSFORMERDIALOG_H
#define METERINGACTS_CURRENTTRANSFORMERDIALOG_H

#include <QDialog>

#include "database.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
    class CurrentTransformerDialog;
}

QT_END_NAMESPACE

class CurrentTransformerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CurrentTransformerDialog(Database &database, QWidget *parent = nullptr);

    ~CurrentTransformerDialog() override;

    // вставка уже введенного заводского номера
    void setSerialNumber(const QString &serialNumber);

    // получаем id созданного ТТ
    int currentTransformerId() const;

private:
    Ui::CurrentTransformerDialog *ui;

    Database &m_database;

    int m_currentTransformerId = -1;

    bool validateForm();
    bool saveCurrentTransformer();

    // метод проверки на существующий серийный номер в БД
    bool serialNumberExists(const QString &serialNumber);
};


#endif //METERINGACTS_CURRENTTRANSFORMERDIALOG_H