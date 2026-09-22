#pragma once

#ifndef METERINGACTS_ARCHIVEWIDGET_H
#define METERINGACTS_ARCHIVEWIDGET_H

#include <QWidget>
#include <QDate>

#include "database.h"


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


};


#endif //METERINGACTS_ARCHIVEWIDGET_H










