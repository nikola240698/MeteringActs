
#include "docxservice.h"
#include "docxgenerator.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>


bool DocxService::generate(const ActData &data, QWidget *parent)
{
    const QString suggestedFileName =
        QString("Акт_%1_%2.docx")
            .arg(data.id)
            .arg(data.date.toString("dd-MM-yyyy"));

    QSettings settings;

    QString docxDirectory =
        settings.value("documents/docxDirectory").toString();

    if (docxDirectory.isEmpty() || !QDir(docxDirectory).exists())
    {
        docxDirectory =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    const QString suggestedPath =
        QDir(docxDirectory).filePath(suggestedFileName);

    QString outputPath =
        QFileDialog::getSaveFileName(
            parent, "Сохранить акт", suggestedPath, "Документ Word (*.docx)");

    if (outputPath.isEmpty())
        return false;

    if (!outputPath.endsWith(
        ".docx", Qt::CaseInsensitive))
    {
        outputPath += ".docx";
    }

    DocxGenerator generator;

    if (!generator.generate(data, outputPath))
    {
        QMessageBox::critical(
            parent, "Ошибка формирования DOCX", generator.lastError());

        return false;
    }

    QMessageBox::information(
        parent, "Готово", "Документ успешно сформирован:\n" + outputPath);

    const bool openDirectory =
        settings.value("documents/openDirectoryAfterGeneration", true).toBool();

    if (openDirectory)
    {
        const QFileInfo fileInfo(outputPath);

        QDesktopServices::openUrl(
            QUrl::fromLocalFile(fileInfo.absolutePath()));
    }

    return true;
}







