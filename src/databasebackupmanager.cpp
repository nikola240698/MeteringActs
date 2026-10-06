
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QUuid>
#include <QSet>
#include <QFile>

#include "databasebackupmanager.h"
#include "database.h"

DatabaseBackupManager::DatabaseBackupManager(Database &database)
    : m_database(database)
{


}

bool DatabaseBackupManager::createBackup(
    const QString &backupDirectory,
    QString &createdBackupPath,
    BackupType type)
{
    m_lastError.clear();
    createdBackupPath.clear();

    if (!m_database.isOpen())
    {
        m_lastError = "База данных не открыта.";
        return false;
    }

    QDir directory(backupDirectory);

    if (!directory.exists())
    {
        if (!directory.mkpath("."))
        {
            m_lastError = "Не удалось создать каталог резервных копий.";
            return false;
        }
    }

    const QString fileName =
        "meteringacts_"
        + backupTypeName(type)
        + "_"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-zzz")
        + ".db";

    const QString backupPath = directory.filePath(fileName);

    QSqlQuery query(m_database.getDatabase());

    QString escapedPath = backupPath;
    escapedPath.replace("'", "''");

    const QString sql = QString("VACUUM INTO '%1';")
        .arg(escapedPath);

    if (!query.exec(sql))
    {
        m_lastError = "Не удалось создать резервную копию: "
            + query.lastError().text();

        return false;
    }

    if (!QFileInfo::exists(backupPath))
    {
        m_lastError =
            "SQLite не сообщил об ошибке, но файл резервной копии не найден.";

        return false;
    }

    createdBackupPath = backupPath;

    return true;
}

QString DatabaseBackupManager::lastError() const
{
    return m_lastError;
}

bool DatabaseBackupManager::validateBackup(const QString &backupPath)
{
    m_lastError.clear();

    // Проверяем существование файла
    if (!QFileInfo::exists(backupPath))
    {
        m_lastError = "Файл резервной копии не найден.";

        return false;
    }

    // Для проверки создаем отдельное соединение с SQLite.
    // Уникальное имя позволяет не конфликтовать с основным соединением
    const QString connectionName = "backup_validation_"
        + QUuid::createUuid().toString(QUuid::WithoutBraces);

    bool result = false;
    QString error;
    {
        QSqlDatabase backupDb =
            QSqlDatabase::addDatabase("QSQLITE", connectionName);

        backupDb.setDatabaseName(backupPath);

        if (!backupDb.open())
        {
            error = "Не удалось открыть резервную копию:\n"
                + backupDb.lastError().text();
        }
        else
        {
            // Проверяем целостность SQLite
            QSqlQuery integrityQuery(backupDb);

            if (!integrityQuery.exec(
                "PRAGMA integrity_check;"))
            {
                error = "Не удалось проверить целостность базы:\n"
                    + integrityQuery.lastError().text();
            }
            else if (!integrityQuery.next()
                || integrityQuery.value(0).toString()
                        .compare("ok", Qt::CaseInsensitive) != 0)
            {
                error = "Проверка целостности резервной копии завершилась с ошибкой.";
            }
            else
            {
                // Проверяем наличие основных таблиц
                const QSet<QString> requiredTables =
                {
                    "acts",
                    "act_types",
                    "areas",
                    "substations",
                    "connections",
                    "employees",
                    "meters",
                    "act_meters",
                    "meter_readings",
                    "current_transformers",
                    "act_current_transformers"
                };

                const QStringList tables =
                    backupDb.tables();

                QStringList missingTables;

                for (const QString &table : requiredTables)
                {
                    if (!tables.contains(table, Qt::CaseInsensitive))
                    {
                        missingTables.append(table);
                    }
                }

                if (!missingTables.isEmpty())
                {
                    error = "Файл SQLite открыт, но не похож на базу данных "
                            "MeteringActs.\n\n"
                            "Отсутствуют таблицы:\n"
                            + missingTables.join(", ");
                }
                else
                {
                    // Проверяем внешние ключи
                    QSqlQuery foreignKeyQuery(backupDb);

                    if (!foreignKeyQuery.exec(
                        "PRAGMA foreign_key_check;"))
                    {
                        error = "Не удалось проверить внешние ключи:\n"
                            + foreignKeyQuery.lastError().text();
                    }
                    else if (foreignKeyQuery.next())
                    {
                        error = "В резервной копии обнаружены "
                                "нарушения внешний ключей.";
                    }
                    else
                    {
                        result = true;
                    }
                }
            }

            backupDb.close();
        }
    }

    // removeDatabase вызываем только после того,
    // на backupDb вышел из области видимости
    QSqlDatabase::removeDatabase(connectionName);

    if (!result)
    {
        m_lastError = error;
    }

    return result;
}

bool DatabaseBackupManager::restoreBackup(const QString &backupPath, QString &safetyBackupPath)
{
    m_lastError.clear();
    safetyBackupPath.clear();

    // 1. Проверяем выбранную копию
    if (!validateBackup(backupPath))
    {
        return false;
    }

    if (!m_database.isOpen())
    {
        m_lastError = "Рабочая база данных не открыта.";
        return false;
    }

    const QString databasePath = m_database.databasePath();

    if (databasePath.isEmpty())
    {
        m_lastError = "не удалось определить путь к рабочей базе данных.";
        return false;
    }

    // Запрещаем восстановление БД самой из себя
    if (QFileInfo(backupPath).canonicalFilePath()
        == QFileInfo(databasePath).canonicalFilePath())
    {
        m_lastError = "Нельзя использовать текущую рабочую базу "
                      "в качестве резервной копии.";
        return false;
    }

    // 2. Создаем страховочную копию текущей рабочей БД
    const QFileInfo databaseInfo(databasePath);

    QDir projectDirectory = databaseInfo.dir();
    projectDirectory.cdUp();

    const QString backupDirectory = projectDirectory.filePath("backups");

    QString createdSafetyBackup;

    if (!createBackup(backupDirectory, createdSafetyBackup, BackupType::Safety))
    {
        m_lastError = "Не удалось создать страховочную копию "
                      "текущей базы данных.\n\n"
                      + m_lastError;
        return false;
    }

    safetyBackupPath = createdSafetyBackup;

    // 3. Закрываем рабочую БД перед заменой файла
    m_database.close();

    // 4. Удаляем текущий файл БД
    if (!QFile::remove(databasePath))
    {
        // Если удалить не получилось, то он всё ещё должен оставаться
        m_database.open();

        m_lastError =
            "Не удалось заменить текущую базу данных.\n"
            "Рабочая база не была изменена.";

        return false;
    }

    // 5. Копируем выбранную резервную копию на место рабочей базы данных
    if (!QFile::copy(backupPath, databasePath))
    {
        // Если копирование не удалось, то возвращаем страховочную копию
        // так как основная уже удалена
        QFile::copy(safetyBackupPath, databasePath);

        if (!m_database.open())
        {
            m_lastError =
                "Не удалось восстановить выбранную резервную копию.\n\n"
                "Так же не удалось автоматически открыть возвращенную "
                "страховочную копию.\n\n"
                "Страховочная копия находится здесь:\n"
                + safetyBackupPath;

            return false;
        }

        m_lastError =
            "Не удалось восстановить выбранную резервную копию.\n\n"
            "Исходная база данных была автоматически восстановлена.";
        return false;
    }

    // 6. Проверяем, что нова рабочая БД открывается
    if (!m_database.open())
    {
        // Удаляем новую и возвращаем старую
        m_database.close();

        QFile::remove(databasePath);

        if (!QFile::copy(safetyBackupPath, databasePath))
        {
            m_lastError =
                "Восстановленная база данных не открылась.\n\n"
                "Не удалось автоматически вернуть предыдущую базу данных.\n\n"
                "Страховочная копия находится здесь:\n"
                + safetyBackupPath;

            return false;
        }

        if (!m_database.open())
        {
            m_lastError =
                "Восстановленная база данных не открылась.\n\n"
                "Предыдущий файл базы был возвращен, "
                "но открыть его не удалось.\n\n"
                "Страховочная копия находится здесь:\n"
                +safetyBackupPath;

            return false;
        }

        m_lastError =
            "Восстановленная база данных не открылась.\n\n"
            "Предыдущая база данных была автоматически возвращена.";
        return false;
    }

    return true;
}

bool DatabaseBackupManager::removeOldAutomaticBackups(const QString &backupDirectory, int keepCount)
{
    m_lastError.clear();

    if (keepCount < 1)
    {
        m_lastError =
            "Количество хранимых автоматических резервных копий "
            "должно быть не меньше 1.";

        return false;
    }

    QDir directory(backupDirectory);

    if (!directory.exists())
        return true;

    const QFileInfoList files =
        directory.entryInfoList(
            QStringList() << "meteringacts_auto_*.db",
            QDir::Files,
            QDir::Time);

    // QDir::Time возвращает файлы от новых к старых
    // Первые keepCount  оставляем.
    for (int i = keepCount; i < files.size(); ++i)
    {
        if (!QFile::remove(files.at(i).absoluteFilePath()))
        {
            m_lastError =
                "Не удалось удалить старую автоматическую резервную копию:\n"
                + files.at(i).absoluteFilePath();

            return false;
        }
    }

    return true;
}

QDateTime DatabaseBackupManager::lastAutomaticBackupTime(const QString &backupDirectory) const
{
    const QDir directory(backupDirectory);

    if (!directory.exists())
        return {};

    const QFileInfoList files =
        directory.entryInfoList(
            QStringList() << "meterinigacts_auto_*.db",
            QDir::Files,
            QDir::Time);

    if (files.isEmpty())
        return {};

    return files.first().lastModified();
}

QString DatabaseBackupManager::backupTypeName(BackupType type) const
{
    switch (type)
    {
        case BackupType::Manual:
            return "manual";
        case BackupType::Automatic:
            return "auto";
        case BackupType::Safety:
            return "safety";
    }

    return "unknown";
}













