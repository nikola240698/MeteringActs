#pragma once

#ifndef METERINGACTS_EMPLOYEEDIALOG_H
#define METERINGACTS_EMPLOYEEDIALOG_H

#include <QDialog>

class Database;


QT_BEGIN_NAMESPACE

namespace Ui
{
    class EmployeeDialog;
}

QT_END_NAMESPACE

class EmployeeDialog : public QDialog
{
    Q_OBJECT

public:
    // Добавление нового сотрудника
    explicit EmployeeDialog(
        Database &database, QWidget *parent = nullptr);

    // Редактирование существующего сотрудника
    explicit EmployeeDialog(
        Database &database, int employeeId, QWidget* parent = nullptr);

    ~EmployeeDialog() override;

private slots:
    void saveEmployee();

private:
    Ui::EmployeeDialog *ui;
    Database &m_database;

    int m_employeeId = -1;
    bool m_editMode = false;

    // метод подключения кнопок
    void setupConnections();
    void loadEmployee();

    bool insertEmployee();
    bool updateEmployee();
};


#endif //METERINGACTS_EMPLOYEEDIALOG_H