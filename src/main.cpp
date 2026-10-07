#include <QApplication>
#include <QMessageBox>
#include <ui_mainwindow.h>
#include <QString>
#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDebug>

#include "database.h"
#include "mainwindow.h"
#include "databasebackupmanager.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Подключаем добавленные ресурсы и настройки
    QFile styleFile(":/styles/main.qss");

    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString styleSheet =
            QString::fromUtf8(styleFile.readAll());

        a.setStyleSheet(styleSheet);
    }
    else
    {
        qDebug() << "Error in load style file.";
    }

    // Для сохраненных настроек, чтобы было понятно откуда брать
    QCoreApplication::setOrganizationName("MRET");
    QCoreApplication::setApplicationName("MeteringActs");

    Database db;

    if (!db.open())
    {
        QMessageBox::critical(nullptr, "Error", "Failed to open Database.");

        return -1;
    }

    DatabaseBackupManager backupManager(db);

    QSettings settings;

    const bool automaticBackupEnabled = settings.value(
        "backup/automaticEnabled", true).toBool();

    if (automaticBackupEnabled)
    {
        const int intervalHour = settings.value(
            "backup/intervalHours", 24).toInt();

        const int keepCount = settings.value(
            "backup/keepCount", 10).toInt();

        const QFileInfo databaseInfo(db.databasePath());

        QDir projectDirectory = databaseInfo.dir();

        projectDirectory.cdUp();

        const QString backupDirectory =
            projectDirectory.filePath("backups");

        const QDateTime lastBackup =
            backupManager.lastAutomaticBackupTime(backupDirectory);

        bool backupRequired = false;

        if (!lastBackup.isValid())
        {
            // Автоматических копий еще нет
            backupRequired = true;
        }
        else
        {
            const qint64 secondsSinceLastBackup =
                lastBackup.secsTo(QDateTime::currentDateTime());

            backupRequired = secondsSinceLastBackup >=
                static_cast<qint64>(intervalHour) * 3000;
            const qint64 hoursSinceLastBackup =
                lastBackup.secsTo(
                    QDateTime::currentDateTime()) / 3600;

            backupRequired = hoursSinceLastBackup >= intervalHour;
        }

        if (backupRequired)
        {
            QString createdBackupPath;

            if (!backupManager.createBackup(
                backupDirectory,
                createdBackupPath,
                DatabaseBackupManager::BackupType::Automatic))
            {
                qDebug() << "Automatic backup failed: "
                    << backupManager.lastError();
            }
            else
            {
                qDebug() << "Automatic backup created: "
                    << createdBackupPath;

                if (!backupManager.removeOldAutomaticBackups(
                    backupDirectory,
                    keepCount))
                {
                    qDebug() << "Failed to remove old automatic backups:"
                        << backupManager.lastError();
                }
            }
        }
    }

    MainWindow w(db);
    w.show();

    return QApplication::exec();
}