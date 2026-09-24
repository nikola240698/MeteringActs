#pragma once

#ifndef METERINGACTS_ACTVIEWDIALOG_H
#define METERINGACTS_ACTVIEWDIALOG_H

#include <QDialog>
#include <QLabel>

#include "actdata.h"

class Database;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class ActViewDialog;
}

QT_END_NAMESPACE

class ActViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActViewDialog(Database &database, int actId, QWidget *parent = nullptr);

    ~ActViewDialog() override;

private:
    Ui::ActViewDialog *ui;

    Database &m_database;
    int m_actId;

    ActData m_data;

    void loadAct();
    void fillActData();



    QWidget* createMeterTable(
        const ActMeterData &meter,
        const QString &title);

    // метод получения показаний из структуры данных акта
    QString meterReadingValue(
        const ActMeterData &meter,
        const QString &code) const;
    // Метод получения текстового представления показаний
    QString meterReadingsText(const ActMeterData &meter) const;

    void fillMeters();

    QLabel* createTableCell(
        const QString &text,
        bool header = false) const;

    const ActMeterData* findMeterByRole(int role) const;

    QString actTitle() const;
    QString workDescription() const;
    QString formatActDate() const;
    // получение характера работ из целочисленного
    QString workScheduleText() const;

    // Методы для создания таблиц ТТ
    void fillCurrentTransformers();

    QWidget* createCurrentTransformerTable(
        const QList<ActCurrentTransformerData> &transformers,
        const QString &title);

    QList<ActCurrentTransformerData> currentTransformerByRole(int role) const;

    // Методы заполнения векторной диаграммы
    void fillVectorDiagram();

    QWidget* createVectorDiagramTable(
        const VectorDiagramData &diagram);

    // Заполняем поле времени замены
    void fillReplacementInfo();

    // метод заполнения представителей верхнего блока
    void fillRepresentatives();

    // метод заполнения подписей всех представителей
    void fillSignature();
    void addSignatureRow(
        const QString &title,
        const QString &name,
        int row);

    // Метод нажатия на кнопку формирования DOCX
    void generateDocx();






};


#endif //METERINGACTS_ACTVIEWDIALOG_H















