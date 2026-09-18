#include <QApplication>
#include <QPushButton>
#include <QMessageBox>
#include <ui_MainWindow.h>


#include "database.h"
#include "mainwindow.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Database db;

    if (!db.open())
    {
        QMessageBox::critical(nullptr, "Error", "Failed to open Database.");

        return -1;
    }

    MainWindow w(db);
    w.show();

    return QApplication::exec();
}