#pragma once

#ifndef METERINGACTS_METERSWIDGET_H
#define METERINGACTS_METERSWIDGET_H

#include <QWidget>

#include "database.h"

class QSqlQueryModel;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MetersWidget;
}

QT_END_NAMESPACE

class MetersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MetersWidget(Database &database, QWidget *parent = nullptr);

    ~MetersWidget() override;

private:
    Ui::MetersWidget *ui;

    Database &m_database;

    // Модель основного списка приборов
    QSqlQueryModel* m_metersModel = nullptr;
    // Модель истории прибора учета
    QSqlQueryModel* m_historyModel = nullptr;

    // метод загрузки приборов
    void loadMeters(const QString &searchText = QString());
    // Метод настройки таблицы
    void setupMetersTable();
    // Метод настройки кнопок
    void updateButtons();
    // Метод удаления выбранного прибора
    void deleteSelectedMeter();

    // Метод загрузки истории прибора
    void loadMeterHistory(int meterId);
    // Метод очистки истории (не браузера)))
    void clearMeterHistory();
    // Настройка таблицы истории прибора
    void setupHistoryTale();
    // Метод открытия акта
    void openSelectedHistoryAct();
};


#endif //METERINGACTS_METERSWIDGET_H