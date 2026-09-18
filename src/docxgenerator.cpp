
#include <JlCompress.h>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>

#include "docxgenerator.h"

DocxGenerator::DocxGenerator()
{
}

bool DocxGenerator::unpackTemplate(const QString &templatePath)
{
    // очищаем всё от старого
    m_lastError.clear();
    m_workDirectory.clear();
    m_documentXmlPath.clear();

    // Проверяем существование шаблона
    QFileInfo templateInfo(templatePath);

    if (!templateInfo.exists())
    {
        // формируем ошибку
        m_lastError = "Файл шаблона не найден: " + templatePath;
        return false;
    }
    // проверяем, что указали на файл, а не папку
    if (!templateInfo.isFile())
    {
        m_lastError = "Указанный путь не является файлом: " + templatePath;
        return false;
    }

    // Пока используем обычную тестовую папку
    m_workDirectory =
        QDir(QStringLiteral(PROJECT_ROOT)).filePath("docx_test");
    // создаем рабочую директорию
    QDir workDir(m_workDirectory);

    // Удаляем содержимое предыдущего текста
    if (workDir.exists())
    {
        // пробуем удалить старый каталог
        if (!workDir.removeRecursively())
        {
            // формируем ошибку
            m_lastError =
                "Не удается удалить предыдущий временный каталог: "
                    + m_workDirectory;
            return false;
        }
    }
    // пробуем создать временную папку
    if (!QDir().mkpath(m_workDirectory))
    {
        // формируем ошибку
        m_lastError =
            "Не удается создать временный каталог: " + m_workDirectory;
        return false;
    }

    // DOCX является ZIP архивом
    const QStringList extractedFiles =
        JlCompress::extractDir(templatePath, m_workDirectory);
    // пробуем распаковать файлы
    if (extractedFiles.isEmpty())
    {
        // формируем ошибку
        m_lastError = "Не удалось распаковать DOCX-шаблон.";
        return false;
    }

    // Основное содержимое docx документа
    m_documentXmlPath =
        QDir(m_workDirectory).filePath("word/document.xml");
    // проверяем содержимое нашей папки
    if (!QFileInfo::exists(m_documentXmlPath))
    {
        // формируем ошибку
        m_lastError = "В DOCX не найден файл word/document.xml";
        return false;
    }
    return true;
}

QString DocxGenerator::lastError() const
{
    return m_lastError;
}

QString DocxGenerator::documentXmlPath() const
{
    return m_documentXmlPath;
}

QStringList DocxGenerator::placeholders()
{
    // список всех найденных placeholders
    QStringList result;
    // читаем документ отбрасывая всё ненужное
    const QString text = readDocumentText();
    // если нашли ошибку, то прерываем результат
    if (!m_lastError.isEmpty())
        return result;
    // позиция для указания поиска следующего placeholder
    qsizetype position = 0;

    while (true)
    {
        // начинаем чтение с указанной позиции
        const qsizetype start = text.indexOf("{{", position);
        // если больше нет {{ то значит всё нашли и прерываем поиск
        if (start == -1)
            break;
        // теперь поиск }} и начинаем со смещение +2
        const qsizetype end = text.indexOf("}}", start + 2);
        // если есть начало, но нет конца, то такой не принимаем и пропускаем
        if (end == -1)
            break;
        // вырезаем из текста весь placeholder учитывая скобки, поэтому длина +2
        const QString placeholder = text.mid(start, end - start + 2);
        // добавляем найденное к результату
        result.append(placeholder);
        // следующий поиск уже смещаем немного
        position = end + 2;
    }

    return result;
}

bool DocxGenerator::replacePlaceholder(const QString &name, const QString &value)
{
    m_lastError.clear();

    // полная запись того, что ищем
    const QString placeholder = "{{" + name + "}}";

    // 1, Открываем document.xml
    QFile file(m_documentXmlPath);

    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError =
            "Не удалось открыть document.xml: " + file.errorString();
        return false;
    }

    // 2. Загружаем XML в DOM-дерево
    QDomDocument document;

    QString errorMessage;
    int errorLine = 0;
    int errorColumn = 0;

    if (!document.setContent(&file, true, &errorMessage, &errorLine, &errorColumn))
    {
        m_lastError =
            QString("Ошибка XML в строке %1, столбце %2: %3")
                .arg(errorLine)
                .arg(errorColumn)
                .arg(errorMessage);

        file.close();
        return false;
    }

    file.close();

    // 3. Получаем все <w:t>
    const QDomNodeList textNodes =
        document.elementsByTagName("w:t");

    qDebug() << "w:t nodes:" << textNodes.count();

    // 4. Строим единый логический текст
    QString logicalText;

    for (qsizetype i = 0; i < textNodes.count(); ++i)
    {
        const QDomNode node = textNodes.at(i);

        logicalText += node.toElement().text();
    }

    qDebug() << "meter_serial position:"
         << logicalText.indexOf("{{meter_serial}}");

    // 5. Ищем placeholder
    const qsizetype placeholderStart = logicalText.indexOf(placeholder);

    if (placeholderStart == -1)
    {
        m_lastError = "Плейсхолдер не найден: " + placeholder;

        return false;
    }

    const qsizetype placeholderEnd = placeholderStart + placeholder.length();

    // 6. Определяем, какие <w:t> содержит placeholder
    qsizetype currentPosition = 0;

    int firstNodeIndex = -1;
    int lastNodeIndex = -1;

    for (qsizetype i = 0; i < textNodes.count(); ++i)
    {
        const QString nodeText = textNodes.at(i).toElement().text();

        const qsizetype nodeStart = currentPosition;

        const qsizetype nodeEnd = currentPosition + nodeText.length();

        // проверяем пересечение диапазона текущего <w:t>
        // с диапазоном нашего placeholder
        if (nodeEnd > placeholderStart && nodeStart < placeholderEnd)
        {
            if (firstNodeIndex == -1)
                firstNodeIndex = static_cast<int>(i);

            lastNodeIndex = static_cast<int>(i);
        }

        currentPosition = nodeEnd;
    }

    if (firstNodeIndex == -1 || lastNodeIndex == -1)
    {
        m_lastError =
            "Не удалось определить XML-узлы плейсхолдера: " + placeholder;
        return false;
    }

    // 7. Выполняем замену
    for (int i = firstNodeIndex; i <= lastNodeIndex; ++i)
    {
        QDomElement element = textNodes.at(i).toElement();

        // удаляем старое текстовое содержимое <w:t>
        while (!element.firstChild().isNull())
        {
            element.removeChild(element.firstChild());
        }

        // в первый узел помещаем новое значение
        if (i == firstNodeIndex)
        {
            element.appendChild(document.createTextNode(value));
        }
    }

    // 8. Сохраняем измененный document.xml
    QSaveFile outputFile(m_documentXmlPath);

    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_lastError =
            "Не удалось открыть document.xml для записи: "
            + outputFile.errorString();
        return false;
    }

    const QByteArray xmlData = document.toByteArray();

    if (outputFile.write(xmlData) == -1)
    {
        m_lastError = "ошибка записи document.xml: " + outputFile.errorString();
        return false;
    }

    if (!outputFile.commit())
    {
        m_lastError =
            "Не удалось сохранить document.xml: " + outputFile.errorString();

        return false;
    }

    return true;

}

// метод чтения документа сквозь не нужные знаки и теги
QString DocxGenerator::readDocumentText()
{
    // очищаем прошлые ошибки
    m_lastError.clear();
    // создаем файл чтения
    QFile file(m_documentXmlPath);
    // пробуем открыть файл только для чтения
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // формируем ошибку
        m_lastError =
            "Не удалось открыть document.xml: " + file.errorString();
        return {};
    }
    // создаем поток чтения xml
    QXmlStreamReader xml(&file);
    // создаем строку приема нужного текста
    QString documentText;
    // читаем документ
    while (!xml.atEnd())
    {
        // переходим дальше
        xml.readNext();
        // читаем только содержимое тегов <w:t>
        if (xml.isStartElement() && xml.name() == QStringLiteral("t"))
        {
            // склеиваем наши найденные фрагменты в строку
            documentText += xml.readElementText();
        }
    }
    // проверяем на ошибки
    if (xml.hasError())
    {
        // формируем ошибку
        m_lastError = "Ошибка чтения document.xml: " + xml.errorString();
    }

    return documentText;
}
















