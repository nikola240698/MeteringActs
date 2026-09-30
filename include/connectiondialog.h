#pragma once

#ifndef METERINGACTS_CONNECTIONDIALOG_H
#define METERINGACTS_CONNECTIONDIALOG_H

#include <QDialog>

#include "database.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
    class ConnectionDialog;
}

QT_END_NAMESPACE

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    // Добавление
    explicit ConnectionDialog(
        Database &database, int substationId, QWidget *parent = nullptr);

    // Редактирование
    explicit ConnectionDialog(
        Database &database, int substationId, int connectionId, QWidget* parent = nullptr);

    ~ConnectionDialog() override;

private:
    Ui::ConnectionDialog *ui;
    Database &m_database;

    int m_substationId = -1;
    int m_connectionId = -1;

    bool save();
    bool insertConnection();
    bool updateConnection();

    void loadConnection();
};


#endif //METERINGACTS_CONNECTIONDIALOG_H