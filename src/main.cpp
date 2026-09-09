#include <QApplication>
#include <QPushButton>
#include <QMessageBox>


#include "database.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qDebug() << "Available SQL drivers:"
                 << QSqlDatabase::drivers();
    Database db;

    if (!db.open())
    {
        QMessageBox::critical(nullptr, "Error", "Failed to open Database.");

        return -1;
    }

    QPushButton button("Hello world!", nullptr);
    button.resize(200, 100);
    button.show();
    return QApplication::exec();
}