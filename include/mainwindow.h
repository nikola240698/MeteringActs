#pragma once

#ifndef MAINWINDOW_H
#define AINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QButtonGroup>

#include "database.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database &database, QWidget *parent = nullptr);

    ~MainWindow() override;

private:
    Ui::MainWindow *ui;

    Database &m_database;

    void setupNavigation();

    void setCurrentPage(int index) const;
};


#endif //MAINWINDOW_H