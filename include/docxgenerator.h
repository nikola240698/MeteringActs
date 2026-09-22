#pragma once

#ifndef METERINGACTS_DOCXGENERATOR_H
#define METERINGACTS_DOCXGENERATOR_H

#include <QString>
#include <QStringList>
#include <QDomDocument>

#include "actdata.h"

class DocxGenerator
{
public:
    DocxGenerator();

    bool unpackTemplate(const QString &templatePath);

    QString lastError() const;
    QString documentXmlPath() const;

    QStringList placeholders();

    bool replacePlaceholder(const QString &name, const QString &value);

    bool saveDocument(const QString &outputPath);

    bool generate(
        const ActData &data,
        const QString &outputPath);

private:
    QString m_lastError;
    QString m_workDirectory;
    QString m_documentXmlPath;

    QString readDocumentText();

    // метод определения шаблона по типу акта
    QString templatePathForActType(int actTypeId) const;

    // метод генерации особых полей в акта согласно его типу
    bool generateActType1(const ActData &data);
    bool generateActType2(const ActData &data);
    bool generateActType3(const ActData &data);
    bool generateActType4(const ActData &data);
    bool generateActType5(const ActData &data);
    bool generateActType6(const ActData &data);

    bool replacePlaceholderInParagraph(
        QDomDocument &document,
        QDomElement &paragraph,
        const QString &placeholder,
        const QString &value);

    void setElementText(
        QDomDocument &document,
        QDomElement &element,
        const QString &text);

    bool replaceMeterData(
        const ActData &data,
        int meterRole,
        const QString &prefix = QString());

    // Методы замены особых полей связанных с ТТ
    bool replaceCurrentTransformers(
        const ActData &data,
        int role,
        const QString &blockName = "current_transformers");

    bool replaceCurrentTransformerRow(
        QDomDocument &document,
        QDomElement &row,
        const ActCurrentTransformerData &transformer,
        const QString &blockName);

    QString meterReadingValue(
        const ActMeterData &meter,
        const QString &code) const;

    bool replaceVectorDiagramData(const ActData &data);

    bool replaceActDate(const ActData &data);

    // метод замены сторонних представителей в верхней части документа
    bool replaceRepresentativesTop(
        QDomDocument &document,
        const QList<ExternalRepresentativeData> &representatives);
    // метод замены сторонних представителей в нижней части документа
    bool replaceRepresentativesSignatures(
        QDomDocument &document,
        const QList<ExternalRepresentativeData> &representatives);
    // метод замены текста в конкретном абзаце
    bool replacePlaceholderInParagraphAll(
        QDomDocument &document,
        QDomElement &paragraph,
        const QString &name,
        const QString &value);
    bool replaceRepresentatives(const ActData &data);

    // метод замены подписи представления представителя внизу акта
    bool replaceEmployeeSignature(const ActData &data);
    // метод создания подписей в представителей в верхней части документа
    bool processConditionalParagraphs(
        QDomDocument &document,
        const QString &blockName,
        bool keep);

    // методы чтения DOM
    bool loadDocument(QDomDocument &document);
    bool saveXmlDocument(const QDomDocument &document);

    // метод удаления при необходимости векторной диаграммы
    bool processConditionalBlock(
        QDomDocument &document,
        const QString &blockName,
        bool keep,
        const QString &replacementText = QString());

    bool processVectorDiagram(const ActData &data);

    // метод получения характера работ
    QString workScheduleTypeToString(int workScheduleType) const;

    // Метод расчета недоучтенной электроэнергии
    bool processReplacementDuration(const ActData &data);



};

#endif //METERINGACTS_DOCXGENERATOR_H