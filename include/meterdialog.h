#pragma once

#ifndef METERINGACTS_METERDIALOG_H
#define METERINGACTS_METERDIALOG_H

#include <QDialog>
#include <QMessageBox>

#include "database.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
    class MeterDialog;
}

QT_END_NAMESPACE

class MeterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeterDialog(Database &database, const QString &serial, QWidget *parent = nullptr);

    ~MeterDialog() override;

    int createdMeterId() const;

private:
    Ui::MeterDialog *ui;
    Database &m_database;


    int m_createdMeterId = -1;

    void saveMeter();

    // метод проверки на существующий серийный номер в БД
    bool serialNumberExists(const QString &serialNumber);
};


#endif //METERINGACTS_METERDIALOG_H