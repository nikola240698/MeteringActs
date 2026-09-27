#pragma once

#ifndef METERINGACTS_AREADIALOG_H
#define METERINGACTS_AREADIALOG_H

#include <QDialog>

#include "database.h"

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
    // Создание нового участка
    explicit AreaDialog(Database &database, QWidget *parent = nullptr);

    // Редактирование существующего участка
    explicit AreaDialog(Database &database, int areaId, QWidget* parent = nullptr);
    ~AreaDialog() override;

private:
    Ui::AreaDialog *ui;
    Database &m_database;

    int m_areaId = -1;

    bool save();
    bool insertArea();
    bool updateArea();

    void loadArea();
};


#endif //METERINGACTS_AREADIALOG_H








