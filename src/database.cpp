
#include <QSqlQuery>
#include <QSettings>
#include <QStandardPaths>

#include "database.h"

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









