
#include <QSqlQuery>
#include <QSqlError>

#include "actrepository.h"

// конструктор класса
ActRepository::ActRepository(Database &database) : m_database(database)
{
}

// метод загрузки данных их БД
bool ActRepository::loadAct(int actId, ActData &data)
{
    // очищаем переменную для ошибок
    m_lastError.clear();

    // Полностью сбрасываем старые данные
    data = ActData{};
    // проверяем на корректность id акта
    if (actId <= 0)
    {
        m_lastError = "Некорректный id акта.";
        return false;
    }
    // вызываем по очереди методы загрузки данных областей
    // основные данные
    if (!loadMainData(actId, data))
        return false;
    // сторонние представители
    if (!loadExternalRepresentatives(actId, data))
        return false;
    // приборы учета
    if (!loadMeters(actId, data))
        return false;
    // ТТ
    if (!loadCurrentTransformers(actId, data))
        return false;
    // векторная диаграмма
    if (!loadVectorDiagram(actId, data))
        return false;

    return true;

}

// геттер ошибки выполнения
QString ActRepository::lastError() const
{
    return m_lastError;
}

// метод загрузки основных данных
bool ActRepository::loadMainData(int actId, ActData &data)
{
    // создаем запрос и пробуем подготовить его
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "a.id, "
        "a.act_type_id, "
        "at.name AS act_type_name, "
        "a.act_date, "

        "ar.id AS area_id, "
        "ar.name AS area_name, "

        "s.id AS substation_id, "
        "s.name AS substation_name,"

        "c.id AS connection_id, "
        "c.name AS connection_name, "
        "c.voltage_kv, "
        "c.ct_ratio, "

        "a.employee_id, "
        "a.employee_name, "
        "a.employee_position, "

        "a.reason,"
        "a.result, "
        "a.seal_number, "
        "a.replacement_duration_minutes,"
        "a.work_schedule_type "

        "FROM acts a "

        "JOIN act_types at "
        "ON at.id = a.act_type_id "

        "JOIN connections c "
        "ON c.id = a.connection_id "

        "JOIN substations s "
        "ON s.id = c.substation_id "

        "JOIN areas ar "
        "ON ar.id = s.area_id "

        "WHERE a.id = :actId;"
    ))
    {
        // выводим причину ошибки
        m_lastError = "Не удалось подготовить запрос данных акта: "
        + query.lastError().text();

        return false;
    }
    // заполняем данные
    query.bindValue(":actId", actId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // выводим ошибку
        m_lastError = "не удалось загрузить основные данные акта: "
        + query.lastError().text();

        return false;
    }
    // проверяем, что что-нибудь нашлось
    if (!query.next())
    {
        m_lastError = "Акт с id " + QString::number(actId) + " не найден.";

        return false;
    }

    // получаем значения из запроса
    data.id = query.value("id").toInt();
    data.actTypeId = query.value("act_type_id").toInt();
    data.actTypeName = query.value("act_type_name").toString();

    data.date = QDate::fromString(query.value("act_date").toString(), Qt::ISODate);

    data.areaId = query.value("area_id").toInt();
    data.areaName = query.value("area_name").toString();

    data.substationId = query.value("substation_id").toInt();
    data.substationName = query.value("substation_name").toString();

    data.connectionId = query.value("connection_id").toInt();
    data.connectionName = query.value("connection_name").toString();

    data.voltage = query.value("voltage_kv").toString();
    data.ctRatio = query.value("ct_ratio").toString();

    data.employeeId = query.value("employee_id").toInt();
    data.employeeName = query.value("employee_name").toString();
    data.employeePosition = query.value("employee_position").toString();

    data.reason = query.value("reason").toString();
    data.result = query.value("result").toString();
    data.sealNumber = query.value("seal_number").toString();
    // проверяем, было ли заполнено время замены
    if (!query.value("replacement_duration_minutes").isNull())
    {
        data.replacementDurationMinutes =
            query.value("replacement_duration_minutes").toInt();
    }
    // проверяем, указан ли характер работ
    if (!query.value("work_schedule_type").isNull())
    {
        data.workScheduleType =
            query.value("work_schedule_type").toInt();
    }

    return true;
}

// метод загрузки сторонних представителей
bool ActRepository::loadExternalRepresentatives(int actId, ActData &data)
{
    // создаем запрос и пробуем подготовить его
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "organization, "
        "short_name, "
        "position "
        "FROM external_representatives "
        "WHERE act_id = :actId "
        "ORDER BY id;"
    ))
    {
        // формируем ошибку
        m_lastError =
            "Не удалось подготовить запрос сторонних представителей: "
            + query.lastError().text();

        return false;
    }
    // заполняем значения в запросе
    query.bindValue(":actId", actId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // формируем ошибку
        m_lastError = "Не удалось загрузить сторонних представителей: " +
            query.lastError().text();

        return false;
    }
    // очищаем от предыдущих данных
    data.externalRepresentatives.clear();
    // проверяем, что что-то нашлось
    while (query.next())
    {
        // создаем структуру представителей
        ExternalRepresentativeData representative;
        // получаем значения из запроса
        representative.organization = query.value("organization").toString();
        representative.shortName = query.value("short_name").toString();
        representative.position = query.value("position").toString();
        // вставляем из в нашу структуру
        data.externalRepresentatives.append(representative);
    }

    return true;
}

// метод получения данных приборов учета
bool ActRepository::loadMeters(int actId, ActData &data)
{
    // создаем и пробуем подготовить запрос
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "id, "
        "meter_id, "
        "role, "
        "meter_name, "
        "serial_number, "
        "accuracy_class, "
        "verification_year "
        "FROM act_meters "
        "WHERE act_id = :actId "
        "ORDER BY id;"
    ))
    {
        // формируем ошибку
        m_lastError = "Не удалось подготовить запрос приборов учета: "
            + query.lastError().text();

        return false;
    }
    // вставляем значения
    query.bindValue(":actId", actId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // формируем ошибку
        m_lastError = "не удалось загрузить приборы учета: "
            + query.lastError().text();

        return false;
    }
    // очищаем от старых данных
    data.meters.clear();
    // заполняем данные по очереди
    while (query.next())
    {
        // создаем структуру
        ActMeterData meter;
        // получаем данные из запроса
        const int actMeterId = query.value("id").toInt();
        meter.meterId = query.value("meter_id").toInt();
        meter.role = query.value("role").toInt();
        meter.name = query.value("meter_name").toString();
        meter.serialNumber = query.value("serial_number").toString();
        meter.accuracyClass = query.value("accuracy_class").toString();
        // проверяем, что есть год поверки
        if (!query.value("verification_year").isNull())
        {
            meter.verificationYear = query.value("verification_year").toInt();
        }
        // пробуем загрузить данные показаний прибора учета
        if (!loadMeterReadings(actMeterId, meter))
            return false;
        // добавляем данные в нашу структуру
        data.meters.append(meter);
    }
    return true;
}

// метод загрузки показаний
bool ActRepository::loadMeterReadings(int actMeterId, ActMeterData &meter)
{
    // создаем и пробуем подготовить запрос
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "mr.reading_type_id, "
        "rt.code, "
        "mr.value "
        "FROM meter_readings mr "
        "JOIN reading_types rt "
        "ON rt.id = mr.reading_type_id "
        "WHERE mr.act_meter_id = :actMeterId "
        "ORDER BY mr.reading_type_id;"
    ))
    {
        // формируем ошибку
        m_lastError = "Не удалось подготовить запрос показаний прибора учета: "
            + query.lastError().text();

        return false;
    }
    // вставляем значения
    query.bindValue(":actMeterId", actMeterId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // формируем ошибеку
        m_lastError = "Не удалось загрузить показания прибора учета: "
            + query.lastError().text();

        return false;
    }

    // очищаем от старых данных
    meter.readings.clear();
    // получаем результаты поочередно
    while (query.next())
    {
        // Создаем нашу структуру
        ActMeterReadingData reading;
        // получаем данные из запроса
        reading.typeId = query.value("reading_type_id").toInt();
        reading.code = query.value("code").toString();
        reading.value = query.value("value").toDouble();
        // добавляем в структуру все данные
        meter.readings.append(reading);
    }

    return true;
}

// метод загрузки данных ТТ
bool ActRepository::loadCurrentTransformers(int actId, ActData &data)
{
    // создаем и пробуем подготовить запрос
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "current_transformer_id, "
        "role, "
        "phase, "
        "name, "
        "serial_number, "
        "transformation_ratio,"
        "accuracy_class "
        "FROM act_current_transformers "
        "WHERE act_id = :actId "
        "ORDER BY role, id;"
    ))
    {
        // формируем ошибку
        m_lastError = "Не удалось подготовить запрос трансформатор тока: "
            + query.lastError().text();

        return false;
    }
    // вставляем значения в запрос
    query.bindValue(":actId", actId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // формируем ошибку
        m_lastError = "Не удалось загрузить трансформаторы тока: "
            + query.lastError().text();

        return false;
    }
    // очищаем данные от прежних актов
    data.currentTransformers.clear();
    // поочередно заполняем данные
    while (query.next())
    {
        // создаем структуру
        ActCurrentTransformerData transformer;
        // получаем данные из запроса
        transformer.currentTransformerId =
            query.value("current_transformer_id").toInt();
        transformer.role = query.value("role").toInt();
        transformer.phase = query.value("phase").toString();
        transformer.name = query.value("name").toString();
        transformer.serialNumber = query.value("serial_number").toString();
        transformer.transformationRatio =
            query.value("transformation_ratio").toString();
        transformer.accuracyClass = query.value("accuracy_class").toString();
        // вносим данные в нашу структуру
        data.currentTransformers.append(transformer);
    }

    return true;
}

// метод загрузки векторной диаграммы
bool ActRepository::loadVectorDiagram(int actId, ActData &data)
{
    // создаем и пробуем подготовить запрос
    QSqlQuery query(m_database.getDatabase());
    if (!query.prepare(
        "SELECT "
        "ia, "
        "angle_a, "
        "angle_a_type, "
        "ib, "
        "angle_b, "
        "angle_b_type, "
        "ic, "
        "angle_c, "
        "angle_c_type, "
        "uab, "
        "ubc, "
        "uca "
        "FROM vector_diagrams "
        "WHERE act_id = :actId;"
    ))
    {
        // формируем ошибку
        m_lastError = "Не удалось подготовить запрос векторной диаграммы: "
            + query.lastError().text();

        return false;
    }
    // вставляем данные в запрос
    query.bindValue(":actId", actId);
    // пробуем выполнить запрос
    if (!query.exec())
    {
        // формируем ошибку
        m_lastError = "Не удалось загрузить векторную диаграмму: "
            + query.lastError().text();

        return false;
    }
    // очищаем от старых данных
    data.vectorDiagram = VectorDiagramData{};
    // проверяем, что результат есть
    if (!query.next())
    {
        // Векторной не найдено, значит её не сняли
        return true;
    }
    // указываем, что векторная существует
    data.vectorDiagram.exists = true;
    // получаем значения из запроса
    data.vectorDiagram.ia = query.value("ia").toDouble();
    data.vectorDiagram.angleA = query.value("angle_a").toDouble();
    data.vectorDiagram.angleAType = query.value("angle_a_type").toString();

    data.vectorDiagram.ib = query.value("ib").toDouble();
    data.vectorDiagram.angleB = query.value("angle_b").toDouble();
    data.vectorDiagram.angleBType = query.value("angle_b_type").toString();

    data.vectorDiagram.ic = query.value("ic").toDouble();
    data.vectorDiagram.angleC = query.value("angle_c").toDouble();
    data.vectorDiagram.angleCType = query.value("angle_c_type").toString();

    data.vectorDiagram.uab = query.value("uab").toDouble();
    data.vectorDiagram.ubc = query.value("ubc").toDouble();
    data.vectorDiagram.uca = query.value("uca").toDouble();

    return true;
}














