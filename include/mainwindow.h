#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

    // Метод для смены картинки кнопок левой панели
    void updateNavigationIcons(int activeId);

};


#endif //MAINWINDOW_H