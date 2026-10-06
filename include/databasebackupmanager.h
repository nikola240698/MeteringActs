#pragma once

#ifndef METERINGACTS_DATABASEBACKUPMANAGER_H
#define METERINGACTS_DATABASEBACKUPMANAGER_H

#include <QString>
#include <QDateTime>

class Database;

class DatabaseBackupManager
{
public:
    enum class BackupType
    {
        Manual,
        Automatic,
        Safety
    };
    explicit DatabaseBackupManager(Database &database);

    bool createBackup(
        const QString &backupDirectory,
        QString &createdBackupPath,
        BackupType type = BackupType::Manual);

    QString lastError() const;

    bool validateBackup(const QString &backupPath);

    bool restoreBackup(
        const QString &backupPath, QString &safetyBackupPath);

    bool removeOldAutomaticBackups(
        const QString &backupDirectory,
        int keepCount);

    QDateTime lastAutomaticBackupTime(const QString &backupDirectory) const;


private:
    Database &m_database;
    QString m_lastError;

    QString backupTypeName(BackupType type) const;

};


#endif //METERINGACTS_DATABASEBACKUPMANAGER_H









