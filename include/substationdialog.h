#pragma once

#ifndef METERINGACTS_SUBSTATIONDIALOG_H
#define METERINGACTS_SUBSTATIONDIALOG_H

#include <QDialog>

#include  "database.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class SubstationDialog;
}

QT_END_NAMESPACE

class SubstationDialog : public QDialog
{
    Q_OBJECT

public:
    // Добавление новой подстанции
    explicit SubstationDialog(
        Database &database, int areaId, QWidget *parent = nullptr);

    // Редактирование существующей ПС
    explicit SubstationDialog(
        Database &database, int areaId, int substationId, QWidget* parent = nullptr);

    ~SubstationDialog() override;

private:
    Ui::SubstationDialog *ui;
    Database &m_database;

    int m_areaId = -1;
    int m_substationId = -1;

    bool save();
    bool insertSubstation();
    bool updateSubstation();

    void loadSubstation();
};

#endif //METERINGACTS_SUBSTATIONDIALOG_H