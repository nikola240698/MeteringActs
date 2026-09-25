#pragma once

#ifndef METERINGACTS_ARCHIVEWIDGET_H
#define METERINGACTS_ARCHIVEWIDGET_H

#include <QWidget>
#include <QDate>


class Database;
class QSqlQueryModel;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class ArchiveWidget;
}

QT_END_NAMESPACE

class ArchiveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ArchiveWidget(
        Database &database, QWidget *parent = nullptr);

    ~ArchiveWidget() override;

private:
    Ui::ArchiveWidget *ui;

    Database &m_database;

    QSqlQueryModel* m_model = nullptr;

    void loadActs(
        const QString &searchText = QString(),
        int actTypeId = -1,
        bool useDateFilter = false,
        const QDate &dateFrom = QDate(),
        const QDate &dateTo = QDate());

    void loadActTypes();

    // метод генерации DOCX файла
    void generateSelectedAct();

    //  метод для сокращения всех connect
    void applyFilters();

    // метод открытия окна показа акта
    void openSelectedAct();

    // Метод открытия редактирования выбранного акта
    void editSelectedAct();

    // Метод удаления акта
    void deleteSelectedAct();


};


#endif //METERINGACTS_ARCHIVEWIDGET_H










