
#include <JlCompress.h>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QSaveFile>

#include "docxgenerator.h"
#include "actcalculator.h"

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

    const QString placeholder = "{{" + name + "}}";

    // 1. Открываем document.xml
    QFile file(m_documentXmlPath);

    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError =
            "Не удалось открыть document.xml: " + file.errorString();

        return false;
    }

    // 2. Загружаем XML
    // namespaceProcessing оставляем false,
    // потому что дальше работаем с именами: w:p w:t
    QDomDocument document;

    const auto parseResult = document.setContent(
        &file,
        QDomDocument::ParseOption::PreserveSpacingOnlyNodes);

    if (!parseResult)
    {
        m_lastError = QString("Ошибка XML в строке %1, столбце %2: %3")
            .arg(parseResult.errorLine)
            .arg(parseResult.errorColumn)
            .arg(parseResult.errorMessage);

        file.close();
        return false;
    }

    file.close();

    // 3. Получаем все абзацы Word
    QDomNodeList paragraphs = document.elementsByTagName("w:p");

    // сколько замен реально выполнили
    int replacementCount = 0;

    // 4. Проходим по каждому абзацу
    for (qsizetype i = 0; i < paragraphs.count(); ++i)
    {
        QDomElement paragraph = paragraphs.at(i).toElement();

        // в одном абзаце один и тот же placeholder тоже
        // теоретически может встретится несколько раз
        // поэтому повторяем замену до тех пор,
        // пока placeholder существует
        while (replacePlaceholderInParagraph(
            document, paragraph, placeholder, value))
        {
            ++replacementCount;
        }
    }

    // 5. Если вообще ничего не нашли - это ошибка
    if (replacementCount == 0)
    {
        m_lastError = "Плейсхолдер не найден: " + placeholder;
        return false;
    }

    // 6. Сохраняем измененный document.xml
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
        m_lastError = "Ошибка записи document.xml: "
            + outputFile.errorString();

        return false;
    }

    if (!outputFile.commit())
    {
        m_lastError = "Не удалось сохранить document.xml: "
            +outputFile.errorString();

        return false;
    }

    return true;
}

bool DocxGenerator::saveDocument(const QString &outputPath)
{
    m_lastError.clear();

    // 1. Проверяем, что рабочая директория существует
    QDir workDir(m_workDirectory);

    if (!workDir.exists())
    {
        m_lastError = "Рабочий каталог DOCX не существует: "
            + m_workDirectory;
        return false;
    }

    // 2. Проверяем путь выходного файла
    QFileInfo outputInfo(outputPath);

    // Создаем каталог для результата, если его еще нет
    const QString outputDirectory = outputInfo.absolutePath();

    if (!QDir().mkpath(outputDirectory))
    {
        m_lastError = "Не удалось создать каталог для DOCX: "
            + outputDirectory;
        return false;
    }

    // 3. Если такой файл существует, то удаляем его
    if (QFileInfo::exists(outputPath))
    {
        if (!QFile::remove(outputPath))
        {
            m_lastError = "Не удалось удалить старый DOCX: "
                + outputPath;
            return false;
        }
    }

    // 4. Упаковываем содержимое рабочей директории
    if (!JlCompress::compressDir(outputPath, m_workDirectory))
    {
        m_lastError = "Не удалось создать DOCX: "
            + outputPath;
        return false;
    }

    // 5. Проверяем, что файл действительно появился
    QFileInfo resultInfo(outputPath);

    if (!resultInfo.exists() ||
        !resultInfo.isFile() ||
        resultInfo.size() == 0)
    {
        m_lastError = "DOCX-файл не был создан или имеет нулевой размер: "
            + outputPath;
        return false;
    }

    return true;
}

bool DocxGenerator::generate(const ActData &data, const QString &outputPath)
{
    m_lastError.clear();

    // 1. Проверяем данные акта
    if (data.id <= 0)
    {
        m_lastError = "Некорректный id акта.";
        return false;
    }

    const QString templatePath = templatePathForActType(data.actTypeId);

    if (templatePath.isEmpty())
    {
        m_lastError =
            QString("Для типа акта %1 не задан DOCX-шаблон.").arg(data.actTypeId);

        return false;
    }

    // 2. Распаковываем DOCX-шаблон
    if (!unpackTemplate(templatePath))
    {
        return false;
    }

    // 3. Заменяем простые поля акта
    if (!replacePlaceholder(
        "substation_name", data.substationName))
    {
        return false;
    }
    if (!replacePlaceholder(
        "area_name", data.areaName))
    {
        return false;
    }
    if (!replacePlaceholder(
        "employee_name", data.employeeName))
    {
        return false;
    }
    if (!replacePlaceholder(
        "employee_position", data.employeePosition))
    {
        return false;
    }
    if (!replacePlaceholder(
        "connection_name", data.connectionName))
    {
        return false;
    }
    if (!replacePlaceholder(
        "voltage", data.voltage))
    {
        return false;
    }
    if (!replacePlaceholder(
        "ct_ratio", data.ctRatio))
    {
        return false;
    }
    if (!replacePlaceholder(
        "seal_number", data.sealNumber))
    {
        return false;
    }
    if (!replacePlaceholder(
        "reason", data.reason))
    {
        return false;
    }
    if (!replacePlaceholder(
        "result", data.result))
    {
        return false;
    }


    if (!replacePlaceholder(
        "work_schedule_type", workScheduleTypeToString(data.workScheduleType)))
    {
        return false;
    }

    // 4. Заполняем дату
    if (!replaceActDate(data))
    {
        return false;
    }

    // 5. Заполняем особенности акта в зависимости от его типа
    switch (data.actTypeId)
    {
        case 1:
            if (!generateActType1(data))
            {
                return false;
            }
            break;
        case 2:
            if (!generateActType2(data))
            {
                return false;
            }
            break;
        default:
            m_lastError =
                "Генерация данного акта пока не реализована.";
            return false;
    }

    // 5. Формируем подпись сотрудника предприятия
    if (!replaceEmployeeSignature(data))
    {
        return false;
    }

    // 7. Заполняем представителей сторонних организаций
    if (!replaceRepresentatives(data))
    {
        return false;
    }

    // 8. Сохраняем готовый DOCX
    if (!saveDocument(outputPath))
    {
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

QString DocxGenerator::templatePathForActType(int actTypeId) const
{
    QString fileName;

    switch (actTypeId)
    {
        case 1:
            fileName = "act_check.docx";
            break;
        case 2:
            fileName = "act_replace_meter.docx";
            break;
        default:
            return {};
    }

    return QDir(QStringLiteral(PROJECT_ROOT))
        .filePath("templates/" + fileName);
}

bool DocxGenerator::generateActType1(const ActData &data)
{
    constexpr int CheckedMeterRole = 1;

    if (!replaceMeterData(data, CheckedMeterRole))
    {
        return false;
    }

    if (!processVectorDiagram(data))
    {
        return false;
    }

    return true;
}

bool DocxGenerator::generateActType2(const ActData &data)
{
    constexpr int RemovedMeterRole = 2;
    constexpr int InstalledMeterRole = 3;

    // Снятый прибор
    if (!replaceMeterData(data, RemovedMeterRole, "removed_"))
    {
        return false;
    }

    // Установленный прибор
    if (!replaceMeterData(data, InstalledMeterRole, "installed_"))
    {
        return false;
    }

    // Векторная диаграмма
    if (!processVectorDiagram(data))
    {
        return false;
    }

    if (!processReplacementDuration(data))
    {
        return false;
    }

    return true;
}

bool DocxGenerator::replacePlaceholderInParagraph(QDomDocument &document, QDomElement &paragraph,
                                                  const QString &placeholder, const QString &value)
{
    // 1, Получаем все <w:t> текущего абзаца
    QDomNodeList textNodes = paragraph.elementsByTagName("w:t");

    if (textNodes.isEmpty())
        return false;

    // 2. Строим логический текст только этого абзаца
    QString logicalText;

    for (qsizetype i = 0; i < textNodes.count(); ++i)
    {
        logicalText += textNodes.at(i).toElement().text();
    }

    // 3. Ищем placeholder
    const qsizetype placeholderStart = logicalText.indexOf(placeholder);

    // Если в этом абзаце нет такого placeholder
    // это не ошибка
    if (placeholderStart == -1)
        return false;

    const qsizetype placeholderEnd = placeholderStart + placeholder.length();

    // 4. Определяем первый и последний <w:t>
    qsizetype currentPosition = 0;

    int firstNodeIndex = -1;
    int lastNodeIndex = -1;

    qsizetype startOffset = -1;
    qsizetype endOffset = -1;

    for (qsizetype i = 0; i < textNodes.count(); ++i)
    {
        const QString nodeText = textNodes.at(i).toElement().text();

        const qsizetype nodeStart = currentPosition;

        const qsizetype nodeEnd = nodeStart + nodeText.length();

        // Здесь начинается placeholder
        if (firstNodeIndex == -1 &&
            placeholderStart >= nodeStart &&
            placeholderStart < nodeEnd)
        {
            firstNodeIndex = static_cast<int>(i);

            startOffset = placeholderStart - nodeStart;
        }

        // здесь заканчивается Placeholder
        if (placeholderEnd > nodeStart && placeholderEnd <= nodeEnd)
        {
            lastNodeIndex = static_cast<int>(i);

            endOffset = placeholderEnd - nodeStart;

            break;
        }

        currentPosition = nodeEnd;
    }

    if (firstNodeIndex == -1 ||
        lastNodeIndex == -1 ||
        startOffset == -1 ||
        endOffset == -1)
    {
        return false;
    }

    // 5. Получаем первый и последний элементы
    QDomElement firstElement =
        textNodes.at(firstNodeIndex).toElement();

    QDomElement lastElement =
        textNodes.at(lastNodeIndex).toElement();

    const QString firstText = firstElement.text();

    const QString lastText = lastElement.text();

    // 6. placeholder находится в одном <w:t>
    if (firstNodeIndex == lastNodeIndex)
    {
        const QString prefix = firstText.left(startOffset);

        const QString suffix = firstText.mid(endOffset);

        setElementText(document, firstElement, prefix + value + suffix);

        return true;
    }

    // 7. Placeholder разбит между несколькими <w:t>

    const QString prefix = firstText.left(startOffset);

    const QString suffix = lastText.mid(endOffset);

    // в первый узел записываем
    // текст до Placeholder + новое значение
    setElementText(document, firstElement, prefix + value);

    // Промежуточные узлы очищаем
    for (int i = firstNodeIndex + 1; i < lastNodeIndex; ++i)
    {
        QDomElement element = textNodes.at(i).toElement();

        setElementText(document, element, QString());
    }

    // В последнем узле оставляем только текст,
    // находящийся после placeholder
    setElementText(document, lastElement, suffix);

    return true;

}

void DocxGenerator::setElementText(QDomDocument &document, QDomElement &element, const QString &text)
{
    // Удаляем старое содержимое <w:t>
    while (!element.firstChild().isNull())
    {
        element.removeChild(element.firstChild());
    }

    // Если строка начинается или заканчивается пробелом,
    // сообщаем Word, что эти пробелы нужно сохранить
    if (text.startsWith(' ') ||
        text.endsWith(' '))
    {
        element.setAttribute("xml:space", "preserve");
    }
    else
    {
        // Если раньше xml:space="preserve" был,
        // а теперь он уже не нужен - убираем
        element.removeAttribute("xml:space");
    }

    // Пустому <w:t> текстовый дочерний узел не нужен
    if (!text.isEmpty())
    {
        element.appendChild(document.createTextNode(text));
    }
}


bool DocxGenerator::replaceMeterData(const ActData &data, int meterRole, const QString &prefix)
{
    const ActMeterData *meter = nullptr;

    // Ищем прибор с нужной ролью
    for (const ActMeterData &item : data.meters)
    {
        if (item.role == meterRole)
        {
            meter = &item;
            break;
        }
    }

    // Для акта, где прибор обязателен, его отсутствие
    // является ошибкой
    if (meter == nullptr)
    {
        m_lastError = QString(
            "В данных акта не найден прибор учета с ролью %1.")
            .arg(meterRole);
        return false;
    }

    // Основные данные прибора
    if (!replacePlaceholder(
        prefix + "meter_name", meter->name))
    {
        return false;
    }
    if (!replacePlaceholder(
        prefix + "meter_serial", meter->serialNumber))
    {
        return false;
    }
    if (!replacePlaceholder(
        prefix + "accuracy", meter->accuracyClass))
    {
        return false;
    }

    QString verificationYear;
    if (meter->verificationYear > 0)
    {
        verificationYear = QString::number(meter->verificationYear);
    }
    if (!replacePlaceholder(
        prefix + "verify", verificationYear))
    {
        return false;
    }

    // Показания
    if (!replacePlaceholder(
        prefix + "reading_a_plus", meterReadingValue(*meter, "A+")))
    {
        return false;
    }
    if (!replacePlaceholder(
        prefix + "reading_a_minus", meterReadingValue(*meter, "A-")))
    {
        return false;
    }
    if (!replacePlaceholder(
        prefix + "reading_r_plus", meterReadingValue(*meter, "R+")))
    {
        return false;
    }
    if (!replacePlaceholder(
        prefix + "reading_r_minus", meterReadingValue(*meter, "R-")))
    {
        return false;
    }

    return true;
}

QString DocxGenerator::meterReadingValue(const ActMeterData &meter, const QString &code) const
{
    for (const ActMeterReadingData &reading : meter.readings)
    {
        if (reading.code == code)
        {
            return QString::number(reading.value, 'f', 2);
        }
    }

    // Если такого показания в БД нет, то в акте
    // оставляем пустое место
    return {};
}

bool DocxGenerator::replaceVectorDiagramData(const ActData &data)
{
    const VectorDiagramData &vector = data.vectorDiagram;

    // Токи
    if (!replacePlaceholder(
        "ia", QString::number(vector.ia, 'f', 0)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "ib", QString::number(vector.ib, 'f', 0)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "ic", QString::number(vector.ic, 'f', 0)))
    {
        return false;
    }

    // Углы
    if (!replacePlaceholder(
        "angle_a", QString::number(vector.angleA, 'f', 0)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "angle_b", QString::number(vector.angleB, 'f', 0)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "angle_c", QString::number(vector.angleC, 'f', 0)))
    {
        return false;
    }

    // Тип угла L/C
    if (!replacePlaceholder(
        "type_a", vector.angleAType))
    {
        return false;
    }
    if (!replacePlaceholder(
        "type_b", vector.angleBType))
    {
        return false;
    }
    if (!replacePlaceholder(
        "type_c", vector.angleCType))
    {
        return false;
    }

    // Напряжения
    if (!replacePlaceholder(
        "uab", QString::number(vector.uab, 'f', 1)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "ubc", QString::number(vector.ubc, 'f', 1)))
    {
        return false;
    }
    if (!replacePlaceholder(
        "uca", QString::number(vector.uca, 'f', 1)))
    {
        return false;
    }

    return true;
}

bool DocxGenerator::replaceActDate(const ActData &data)
{
    if (!data.date.isValid())
    {
        m_lastError = "В данных акта указана неверная дата.";
        return false;
    }

    static const QStringList months =
    {
        "января",
        "февраля",
        "марта",
        "апреля",
        "мая",
        "июня",
        "июля",
        "августа",
        "сентября",
        "октября",
        "ноября",
        "декабря"
    };

    const QString day = QString::number(data.date.day());

    const QString month = months.at(data.date.month() - 1);

    const QString year = QString::number(data.date.year());

    if (!replacePlaceholder(
        "day", day))
    {
        return false;
    }
    if (!replacePlaceholder(
        "month", month))
    {
        return false;
    }
    if (!replacePlaceholder(
        "year", year))
    {
        return false;
    }

    return true;
}

// метод замены сторонних представителей в верхней части документа
bool DocxGenerator::replaceRepresentativesTop(QDomDocument &document,
    const QList<ExternalRepresentativeData> &representatives)
{
    QDomNodeList paragraphs = document.elementsByTagName("w:p");

    QDomElement templateParagraph;

    // 1. Ищем абзац-шаблон
    for (qsizetype i = 0; i < paragraphs.count(); ++i)
    {
        QDomElement paragraph = paragraphs.at(i).toElement();

        QString text;

        QDomNodeList textNodes = paragraph.elementsByTagName("w:t");

        for (qsizetype j = 0; j < textNodes.count(); ++j)
        {
            text += textNodes.at(j).toElement().text();
        }

        if (text.contains("{{#representatives_top}}") &&
            text.contains("{{/representatives_top}}"))
        {
            templateParagraph = paragraph;
            break;
        }
    }

    // 2. Проверяем, нашли ли блок

    if (templateParagraph.isNull())
    {
        m_lastError = "В DOCX не найден блок representatives_top.";
        return false;
    }

    // 3. Получаем родителя абзаца
    QDomNode parent = templateParagraph.parentNode();

    if (parent.isNull())
    {
        m_lastError = "Не удалось определить родительский узел "
                      "блока representatives_top.";
        return false;
    }

    // 4. Создаем копию для каждого представителя
    for (const ExternalRepresentativeData &representative : representatives)
    {
        QDomNode clonedNode = templateParagraph.cloneNode(true);

        QDomElement clonedParagraph = clonedNode.toElement();

        // Удаляем маркеры блока
        replacePlaceholderInParagraphAll(
            document, clonedParagraph, "#representatives_top", "");

        replacePlaceholderInParagraphAll(
            document, clonedParagraph, "/representatives_top", "");

        // Подставляем данные
        replacePlaceholderInParagraphAll(
            document, clonedParagraph,
            "representative_organization", representative.organization);

        replacePlaceholderInParagraphAll(
            document, clonedParagraph,
            "representative_position", representative.position);

        replacePlaceholderInParagraphAll(
            document, clonedParagraph,
            "representative_name", representative.shortName);

        // Вставляем созданный абзац перед шаблоном.
        parent.insertBefore(clonedNode, templateParagraph);
    }

    // 5. Исходный абзац-шаблон больше не нужен
    parent.removeChild(templateParagraph);

    return true;

}

bool DocxGenerator::replaceRepresentativesSignatures(QDomDocument &document,
    const QList<ExternalRepresentativeData> &representatives)
{
     QDomNodeList paragraphs = document.elementsByTagName("w:p");

    QDomElement templateParagraph;

    // 1. Ищем абзац-шаблон подписей
    for (qsizetype i = 0; i < paragraphs.count(); ++i)
    {
        QDomElement paragraph = paragraphs.at(i).toElement();

        QString text;

        QDomNodeList textNodes = paragraph.elementsByTagName("w:t");

        for (qsizetype j = 0; j < textNodes.count(); ++j)
        {
            text += textNodes.at(j).toElement().text();
        }

        if (text.contains("{{#representative_signature}}") &&
            text.contains("{{/representative_signature}}"))
        {
            templateParagraph = paragraph;
            break;
        }
    }

    // 2. Проверяем, нашли ли блок

    if (templateParagraph.isNull())
    {
        m_lastError = "В DOCX не найден блок representative_signature.";
        return false;
    }

    // 3. Получаем родительский узел
    QDomNode parent = templateParagraph.parentNode();

    if (parent.isNull())
    {
        m_lastError = "Не удалось определить родительский узел "
                      "блока representative_signature.";
        return false;
    }

    // 4. Создаем строку подписей для каждого представителя
    for (const ExternalRepresentativeData &representative : representatives)
    {
        // Полностью копируем абзац Word вместе со всем форматированием
        QDomNode clonedNode = templateParagraph.cloneNode(true);

        QDomElement clonedParagraph = clonedNode.toElement();

        // Удаляем служебные маркеры блока
        replacePlaceholderInParagraphAll(
            document, clonedParagraph, "#representative_signature", "");

        replacePlaceholderInParagraphAll(
            document, clonedParagraph, "/representative_signature", "");

        // Подставляем данные
        replacePlaceholderInParagraphAll(
            document, clonedParagraph,
            "representative_organization", representative.organization);

        replacePlaceholderInParagraphAll(
            document, clonedParagraph,
            "representative_name", representative.shortName);

        // Вставляем созданный абзац перед шаблоном.
        parent.insertBefore(clonedNode, templateParagraph);
    }

    // 5. Исходный абзац-шаблон больше не нужен
    parent.removeChild(templateParagraph);

    return true;

}

// метод замены текста в конкретном абзаце
bool DocxGenerator::replacePlaceholderInParagraphAll(QDomDocument &document, QDomElement &paragraph,
    const QString &name, const QString &value)
{
    // реализация проста, по причине уже имеющегося
    // replacePlaceholderInParagraph
    const QString placeholder = "{{" + name + "}}";

    bool replaced = false;

    while (replacePlaceholderInParagraph(document, paragraph, placeholder, value))
    {
        replaced = true;
    }

    return replaced;
}

bool DocxGenerator::replaceRepresentatives(const ActData &data)
{
    QDomDocument document;

    if (!loadDocument(document))
    {
        return false;
    }

    const bool hasRepresentatives = !data.externalRepresentatives.isEmpty();

    // Верхние условные строки
    if (!processConditionalParagraphs(
        document,
        "with_representatives",
        hasRepresentatives))
    {
        return false;
    }

    // Верхний список сторонних представителей
    if (!replaceRepresentativesTop(
        document,
        data.externalRepresentatives))
    {
        return false;
    }

    // Нижние подписи сторонних представителей
    if (!replaceRepresentativesSignatures(
        document,
        data.externalRepresentatives))
    {
        return false;
    }

    if (!saveXmlDocument(document))
    {
        return false;
    }

    return true;
}

bool DocxGenerator::replaceEmployeeSignature(const ActData &data)
{
    QString signatureTitle;

    if (data.externalRepresentatives.isEmpty())
    {
        // Если сторонних представителей нет
        signatureTitle = data.employeePosition;
    }
    else
    {
        signatureTitle =
            "Представитель ТОО \"Межрегионэнерготранзит\"";
    }

    if (!replacePlaceholder("employee_signature_title", signatureTitle))
    {
        return false;
    }

    return true;
}

bool DocxGenerator::processConditionalParagraphs(QDomDocument &document, const QString &blockName, bool keep)
{
    const QString startMarker =
        "{{#" + blockName + "}}";

    const QString endMarker =
        "{{/" + blockName + "}}";

    QDomNodeList paragraphs =
        document.elementsByTagName("w:p");

    bool found = false;

    // Идем с конца, потому что при keep = false
    // будем удалять элементы из DOM.
    for (qsizetype i = paragraphs.count() - 1; i >= 0; --i)
    {
        QDomElement paragraph = paragraphs.at(i).toElement();

        QString text;

        QDomNodeList textNodes = paragraph.elementsByTagName("w:t");

        for (qsizetype j = 0; j < textNodes.count(); ++j)
        {
            text += textNodes.at(j).toElement().text();
        }

        // Только те абзацы, содержащие оба маркера
        if (!text.contains(startMarker) ||
            !text.contains(endMarker))
        {
            continue;
        }

        found = true;

        if (keep)
        {
            // Данные нужны
            replacePlaceholderInParagraphAll(
                document,
                paragraph,
                "#" + blockName,
                "");

            replacePlaceholderInParagraphAll(
                document,
                paragraph,
                "/" + blockName,
                "");
        }
        else
        {
            // Данные не нужны. Удаляем весь абзац Word
            QDomNode parent = paragraph.parentNode();

            if (!parent.isNull())
            {
                parent.removeChild(paragraph);
            }
        }
    }

    if (!found)
    {
        m_lastError = "В DOCX не найден условный блок: " + blockName;

        return false;
    }

    return true;

}

bool DocxGenerator::loadDocument(QDomDocument &document)
{
    QFile file(m_documentXmlPath);

    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError =
            "Не удалось открыть document.xml: " + file.errorString();
        return false;
    }

    const auto parseResult = document.setContent(
        &file, QDomDocument::ParseOption::PreserveSpacingOnlyNodes);

    file.close();

    if (!parseResult)
    {
        m_lastError = QString(
            "Ошибка XML в строке %1, столбце %2: %3")
            .arg(parseResult.errorLine)
            .arg(parseResult.errorColumn)
            .arg(parseResult.errorMessage);
        return false;
    }

    return true;
}

bool DocxGenerator::saveXmlDocument(const QDomDocument &document)
{
    QSaveFile file(m_documentXmlPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_lastError = "Не удалось открыть document.xml для записи: "
            + file.errorString();
        return false;
    }

    const QByteArray xmlData = document.toByteArray();

    if (file.write(xmlData) == -1)
    {
        m_lastError = "Ошибка записи document.xml: "
            + file.errorString();
        return false;
    }

    if (!file.commit())
    {
        m_lastError = "не удалось сохранить document.xml: "
            + file.errorString();

        return false;
    }

    return true;
}

bool DocxGenerator::processConditionalBlock(QDomDocument &document, const QString &blockName, bool keep,
    const QString &replacementText)
{
    const QString startMarker =
        "{{#" + blockName + "}}";

    const QString endMarker =
        "{{/" + blockName + "}}";

    QDomNodeList paragraphs = document.elementsByTagName("w:p");

    QDomElement startParagraph;
    QDomElement endParagraph;

    // 1. Ищем начало и конец блока
    for (qsizetype i = 0; i < paragraphs.count(); ++i)
    {
        QDomElement paragraph = paragraphs.at(i).toElement();

        QString text;

        QDomNodeList textNodes = paragraph.elementsByTagName("w:t");

        for (qsizetype j = 0; j < textNodes.count(); ++j)
        {
            text += textNodes.at(j).toElement().text();
        }

        if (text.contains(startMarker))
        {
            startParagraph = paragraph;
        }

        if (text.contains(endMarker))
        {
            endParagraph = paragraph;
            break;
        }
    }

    // 2. Проверяем наличие обоих маркеров
    if (startParagraph.isNull() ||
        endParagraph.isNull())
    {
        m_lastError = "В DOCX найден условный блок: " + blockName;

        return false;
    }

    // 3. Оба маркера должны иметь одного родителя
    QDomNode parent = startParagraph.parentNode();

    if (parent.isNull() ||
        parent != endParagraph.parentNode())
    {
        m_lastError =
            "Маркеры условного блока " + blockName +
            " находятся в разных элементах DOCX.";

        return false;
    }

    // Если блок нужно оставить
    if (keep)
    {
        // Удаляем только служебные абзацы (маркеры)
        parent.removeChild(startParagraph);
        parent.removeChild(endParagraph);

        return true;
    }

    // Если блок надо заменить
    // Сначала используем начальный абзац для текста-замены
    replacePlaceholderInParagraphAll(
        document,
        startParagraph,
        "#" + blockName,
        replacementText);

    // теперь удаляем все после начального абзаца до конечного маркера включительно
    QDomNode current = startParagraph.nextSibling();

    while (!current.isNull())
    {
        QDomNode next = current.nextSibling();

        const bool isEnd = current == endParagraph;

        parent.removeChild(current);

        if (isEnd)
        {
            break;
        }

        current = next;
    }

    return true;
}

bool DocxGenerator::processVectorDiagram(const ActData &data)
{
    QDomDocument document;

    if (!loadDocument(document))
    {
        return false;
    }

    const bool hasVEctorDiagram = data.vectorDiagram.exists;

    if (!processConditionalBlock(
        document,
        "vector_diagram",
        hasVEctorDiagram,
        "Без нагрузки."))
    {
        return false;
    }

    // Сохраняем структурные изменения DOCX
    if (!saveXmlDocument(document))
    {
        return false;
    }

    // Если диаграммы нет, то вместо нее записываем "Без нагрузки."
    if (!hasVEctorDiagram)
    {
        return true;
    }

    // Диаграмма существует
    return replaceVectorDiagramData(data);
}

QString DocxGenerator::workScheduleTypeToString(int workScheduleType) const
{
    switch (workScheduleType)
    {
        case PlannedWork:
            return "плановая";
        case UnplannedWork:
            return "внеплановая";
        default:
            return {};
    }
}

bool DocxGenerator::processReplacementDuration(const ActData &data)
{
    // Нет нагрузки
    if (!data.vectorDiagram.exists)
    {
        QDomDocument document;

        if (!loadDocument(document))
        {
            return false;
        }

        if (!processConditionalParagraphs(
            document,
            "replacement_duration",
            false))
        {
            m_lastError =
                "Не удалось удалить блок времени замены.";

            return false;
        }

        return saveXmlDocument(document);
    }

    // Есть нагрузка - сразу выполняем расчет

    bool voltageOk = false;

    const double voltageKv = data.voltage.toDouble(&voltageOk);

    if (!voltageOk || voltageKv <= 0.0)
    {
        m_lastError =
            "Некорректное напряжение присоединения: " + data.voltage;

        return false;
    }

    double powerMW = 0.0;

    if (!ActCalculator::calculatePrimaryActivePowerMW(
        data.vectorDiagram,
        data.ctRatio,
        voltageKv,
        powerMW))
    {
        m_lastError =
            "не удалось рассчитать мощность присоединения. "
            "Проверьте Ктт и напряжение.";

        return false;
    }

    const double energyKWh =
        ActCalculator::calculateUnmeteredEnergyKWh(
            powerMW,
            data.replacementDurationMinutes);

    // Снчала обычные данные
    if (!replacePlaceholder(
        "replacement_duration_minutes",
        QString::number(data.replacementDurationMinutes)))
    {
        return false;
    }

    if (!replacePlaceholder(
        "connection_power",
        QString::number(powerMW, 'f', 3)))
    {
        return false;
    }

    if (!replacePlaceholder(
        "connection_energy",
        QString::number(energyKWh, 'f', 3)))
    {
        return false;
    }

    // Теперь убираем маркеры условного блока
    if (!replacePlaceholder("#replacement_duration", ""))
    {
        return false;
    }

    if (!replacePlaceholder("/replacement_duration", ""))
    {
        return false;
    }
    return true;
}
















