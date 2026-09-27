#pragma once

#ifndef METERINGACTS_DIRECTORIESWIDGET_H
#define METERINGACTS_DIRECTORIESWIDGET_H

#include <QWidget>

#include "database.h"

class QSqlQuery;

enum class DirectoryLevel
{
    Areas,
    Substations
};


QT_BEGIN_NAMESPACE

namespace Ui
{
    class DirectoriesWidget;
}

QT_END_NAMESPACE

class DirectoriesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DirectoriesWidget(Database &database, QWidget *parent = nullptr);

    ~DirectoriesWidget() override;

private:
    Ui::DirectoriesWidget *ui;
    Database &m_database;

    DirectoryLevel m_level = DirectoryLevel::Areas;
    int m_currentAreId = -1;

    QSqlQueryModel* m_leftModel = nullptr;
    QSqlQueryModel* m_rightModel = nullptr;

    // метод загрузки списка участков
    void loadAreas();
    // метод загрузки списка подстанций
    void loadSubstations(int areaId);
    // метод очистки списка подстанций
    void clearSubstations();

    // Метод вывода содержимого участка
    void showAreasLevel();
    // Метод вывода содержимого подстанции
    void showSubstationsLevel(int areaId);

    // Метод загрузки присоединений
    void loadConnections(int substationId);
    // Метод очистки окна присоединений
    void clearConnections();
    // Метод настройки названий столбцов таблицы присоединений
    void setupConnectionsTable();
};


#endif //METERINGACTS_DIRECTORIESWIDGET_H