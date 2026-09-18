#pragma once

#ifndef METERINGACTS_DOCXGENERATOR_H
#define METERINGACTS_DOCXGENERATOR_H

#include <QString>
#include <QStringList>

class DocxGenerator
{
public:
    DocxGenerator();

    bool unpackTemplate(const QString &templatePath);

    QString lastError() const;
    QString documentXmlPath() const;

    QStringList placeholders();

    bool replacePlaceholder(const QString &name, const QString &value);

private:
    QString m_lastError;
    QString m_workDirectory;
    QString m_documentXmlPath;

    QString readDocumentText();
};

#endif //METERINGACTS_DOCXGENERATOR_H