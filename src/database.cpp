
#include <QSqlQuery>
#include <QSettings>
#include <QStandardPaths>
#include <QDateTime>
#include <QSet>

#include "database.h"
#include "databasebackupmanager.h"

// конструктор класса
Database::Database()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
}

// метод открытия БД
bool Database::open()
{
    // получаем путь к БД
    const QString path = getDatabasePath();
    // выводим в консоль полученный путь
    qDebug() << "Path of the DB: " << path;

    // если пути не существует
    if (!QFileInfo::exists(path))
    {
        // указываем ошибку
        qDebug() << "Database file not found. Creating new database:" << path;

        if (!createDatabase(path))
        {
            qDebug() << "Failed to create database.";
            return false;
        }

        // CreateDatabase() уже открыла созданную БД
        return true;
    }

    // открываем БД пол полученному пути
    db.setDatabaseName(path);
    // если не открылось
    if (!db.open())
    {
        // выводим сообщение, указав ошибку
        qDebug() << "Error opening database: ";
        qDebug() << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("PRAGMA foreign_keys = ON;"))
    {
        qDebug() << "Failed to enable foreign keys:";
        qDebug() << query.lastError().text();

        db.close();
        return false;
    }

    // Подключаем миграцию при открытии базы
    if (!applyMigrations())
    {
        qCritical() << "Failed to apply database migrations.";

        db.close();
        return false;
    }

    qDebug() << "Database was opened successfully";

    return true;
}

// метод закрытия БД
void Database::close()
{
    // проверяем, что соединение имеется и закрываем его
    if (db.isOpen())
    {
        db.close();
    }
}

// метод проверки состояния БД
bool Database::isOpen() const
{
    return db.isOpen();
}

// метод получения БД
QSqlDatabase Database::getDatabase() const
{
    return db;
}

// метод получения пути к БД
QString Database::databasePath() const
{
    return db.databaseName();
}

// метод начала транзакции
bool Database::transaction()
{
    return db.transaction();
}

// метод коммита изменений
bool Database::commit()
{
    return db.commit();
}

// метод отката изменений
bool Database::rollback()
{
    return db.rollback();
}

// метод получения последней ошибки
QString Database::lastError() const
{
    return db.lastError().text();
}

// Метод создания БД из SQL скрипта
bool Database::createDatabase(const QString &path)
{
    // Создаем каталог, в котором будет находиться БД
    const QFileInfo databaseInfo(path);
    QDir directory = databaseInfo.dir();

    if (!directory.exists())
    {
        if (!directory.mkpath("."))
        {
            qDebug() << "Failed to create database directory:"
                    << directory.absolutePath();
            return false;
        }
    }

    // Открываем встроенный SQL-скрипт
    QFile schemaFile(":/database/schema.sql");

    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open database schema:"
                << schemaFile.errorString();
        return false;
    }

    const QString schema = QString::fromUtf8(schemaFile.readAll());

    schemaFile.close();

    // Указываем путь к новой БД
    db.setDatabaseName(path);

    if (!db.open())
    {
        qDebug() << "Failed to create database:"
                << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("PRAGMA foreign_keys = ON;"))
    {
        qDebug() << "Failed to enable foreign keys:"
                << query.lastError().text();

        db.close();
        return false;
    }

    // Создание выполняемой транзакции
    if (!db.transaction())
    {
        qDebug() << "Failed to start database transaction:"
                << db.lastError().text();

        db.close();
        return false;
    }

    QString statement;

    const QStringList lines = schema.split('\n');

    for (const QString &line : lines)
    {
        const QString trimmedLine = line.trimmed();

        // Пустые строки и комментарии пропускаем
        if (trimmedLine.isEmpty() || trimmedLine.startsWith("--"))
        {
            continue;
        }


        statement += line + '\n';

        if (!trimmedLine.endsWith(';'))
        {
            continue;
        }

        if (!query.exec(statement))
        {
            qDebug() << "Failed to execute schema statement:";
            qDebug() << statement;
            qDebug() << query.lastError().text();

            db.rollback();
            db.close();

            // Не оставляем поврежденную недосозданную БД
            QFile::remove(path);

            return false;
        }

        statement.clear();
    }

    if (!statement.trimmed().isEmpty())
    {
        qDebug() << "Incomplete SQL statement in schema.";

        db.rollback();
        db.close();
        QFile::remove(path);

        return false;
    }

    // Устанавливаем версию для новой базы
    if (!query.exec("PRAGMA user_version = 1;"))
    {
        qCritical() << "Failed to set database schema version: " <<
            query.lastError().text();

        db.rollback();
        db.close();

        QFile::remove(path);
        return false;
    }

    if (!db.commit())
    {
        qDebug() << "Failed to commit database schema:"
                << db.lastError().text();

        db.close();
        QFile::remove(path);

        return false;
    }

    qDebug() << "Database created successfully:" << path;

    return true;
}

// метод получения пути к БД
QString Database::getDatabasePath() const
{

    QSettings settings;

    const QString savedPath =
        settings.value("database/path").toString();

    if (!savedPath.isEmpty())
    {
        return savedPath;
    }


    const QString dataDirectory =
        QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation);

    return QDir(dataDirectory).filePath("meteringacts.db");
}

bool Database::applyMigrations()
{
    constexpr int CURRENT_SCHEMA_VERSION = 1;

    QSqlQuery query(db);

    if (!query.exec("PRAGMA user_version;") || !query.next())
    {
        qCritical() << "Cannot read database schema version: "
            << query.lastError().text();

        return false;
    }

    const int version = query.value(0).toInt();

    qDebug() << "Database schema version: " << version;

    if (version > CURRENT_SCHEMA_VERSION)
    {
        qCritical() << "Database is newer than application supports.";
        return false;
    }

    if (version == CURRENT_SCHEMA_VERSION)
    {
        return true;
    }

    if (version != 0)
    {
        qCritical() << "Unsupported legacy schema version: " << version;

        return false;
    }

    if (!validateLegacySchema())
    {
        qCritical() << "Legacy database schema validation failed.";
        return false;
    }

    // Проверяем целостность перед миграцией.
    QSqlQuery integrityQuery(db);

    if (!integrityQuery.exec("PRAGMA integrity_check;") ||
        !integrityQuery.next() ||
        integrityQuery.value(0).toString() != "ok")
    {
        qCritical() << "Database integrity check failed.";

        return false;
    }

    QSqlQuery foreignKeyQuery(db);

    if (!foreignKeyQuery.exec("PRAGMA foreign_key_check;"))
    {
        qCritical() << "Foreign key check failed: " <<
            foreignKeyQuery.lastError().text();

        return false;
    }

    if (foreignKeyQuery.next())
    {
        qCritical() << "Foreign key violations detected.";

        return false;
    }

    // Завершаем активные SQL-запросы перед VACUUM INTO
    query.finish();
    integrityQuery.finish();
    foreignKeyQuery.finish();

    // Создаем страховочную копию
    const QFileInfo databaseInfo(databasePath());

    const QString backupDirectory =
        databaseInfo.dir().filePath("backups");

    DatabaseBackupManager backupManager(*this);

    QString backupPath;

    if (!backupManager.createBackup(
        backupDirectory,
        backupPath,
        DatabaseBackupManager::BackupType::Safety))
    {
        qCritical() << "Migration backup failed: " <<
            backupManager.lastError();

        return false;
    }

    if (!backupManager.validateBackup(backupPath))
    {
        qCritical() << "Migration backup validation failed: " <<
            backupManager.lastError();

        return false;
    }

    qDebug() << "Migration safety backup: " << backupPath;

    // Обновляем версию атомарно
    if (!db.transaction())
    {
        qCritical() << "Cannot start migration transaction: " <<
            db.lastError().text();

        return false;
    }

    QSqlQuery migrationQuery(db);

    if (!migrationQuery.exec("PRAGMA user_version = 1;"))
    {
        qCritical() << "Cannot update schema version: " <<
            migrationQuery.lastError().text();

        db.rollback();
        return false;
    }

    if (!db.commit())
    {
        qCritical() << "Cannot commit migration: " <<
            db.lastError().text();

        db.rollback();
        return false;
    }

    qDebug() << "Database schema migrated to version 1.";

    return true;
}



bool Database::validateLegacySchema()
{
    const QMap<QString, QStringList> requiredColumns = {
        {"areas", {"id", "uuid", "version_uuid", "name"}},
        {"substations", {"id", "uuid", "version_uuid",
                         "area_id", "name"}},
        {"connections", {"id", "uuid", "version_uuid",
                         "substation_id", "name"}},
        {"employees", {"id", "uuid", "version_uuid",
                       "short_name", "position", "is_active"}},
        {"meters", {"id", "uuid", "version_uuid",
                    "name", "serial_number"}},
        {"current_transformers", {"id", "uuid", "version_uuid",
                                  "name", "serial_number"}},
        {"acts", {"id", "uuid", "version_uuid",
                  "act_type_id", "connection_id"}},
        {"act_meters", {"id", "act_id", "meter_id"}},
        {"meter_readings", {"id", "act_meter_id", "reading_type_id"}},
        {"vector_diagrams", {"id", "act_id"}},
        {"external_representatives", {"id", "act_id"}},
        {"act_current_transformers",
            {"id", "act_id", "current_transformer_id"}},
        {"act_types", {"id", "name"}},
        {"reading_types", {"id", "name", "code"}}
    };

    for (auto it = requiredColumns.cbegin(); it != requiredColumns.cend(); ++it)
    {
        QSqlQuery query(db);

        if (!query.exec(
            QString("PRAGMA table_info(\"%1\");").arg(it.key())))
        {
            qCritical() << "Cannot inspect table: " << it.key()
                << query.lastError().text();
            return false;
        }

        QSet<QString> existingColumns;

        while (query.next())
        {
            existingColumns.insert((
                query.value(1).toString()));
        }

        for (const QString &column : it.value())
        {
            if (!existingColumns.contains(column))
            {
                qCritical() << "Missing column: " << it.key() << column;

                return false;
            }
        }
    }

    return true;
}









