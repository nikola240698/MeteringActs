#include <QApplication>
#include <QMessageBox>
#include <ui_MainWindow.h>
#include <QString>
#include <QDebug>
#include <QDir>

#include "database.h"
#include "mainwindow.h"
#include "docxgenerator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DocxGenerator generator;

    const QString templatePath =
        QDir(QStringLiteral(PROJECT_ROOT)).filePath("templates/act_check.docx");

    if (!generator.unpackTemplate(templatePath))
    {
        qDebug() << "DOCX ERROR: " << generator.lastError();
    } else
    {
        qDebug() << "DOCX unpacked successfully";

        if (!generator.replacePlaceholder("meter_serial", "12345678"))
        {
            qDebug() << "Replace ERROR: " << generator.lastError();
        } else
        {
            qDebug() << "Placeholder replaced successfully";
        }

    }




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