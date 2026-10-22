#pragma once

#ifndef METERINGACTS_SETTINGSWIDGET_H
#define METERINGACTS_SETTINGSWIDGET_H

#include <QWidget>

#include "database.h"
#include "databasebackupmanager.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class SettingsWidget;
}

QT_END_NAMESPACE

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(
        Database &database, QWidget *parent = nullptr);

    ~SettingsWidget() override;

private:
    Ui::SettingsWidget *ui;

    Database &m_database;
    DatabaseBackupManager m_backupManager;

    QString backupDirectory() const;

    void createBackup();
    void restoreBackup();

    void loadSettings();
    void saveSettings();

    // Каталог для сохранения DOCX
    void selectDocxDirectory();

    // Выбор рабочей базы данных
    void selectDatabase();


};


#endif //METERINGACTS_SETTINGSWIDGET_H