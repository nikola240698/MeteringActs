
#include <QSqlQuery>

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
        qDebug() << "Database file not found: " << path;
        return false;
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

// метод получения пути к БД
QString Database::getDatabasePath() const
{
    return QDir(QStringLiteral(PROJECT_ROOT)).filePath("db/meteringacts.db");
}









