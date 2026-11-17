PRAGMA foreign_keys = ON;


-- ============================================================
-- 1. УЧАСТКИ
-- ============================================================

CREATE TABLE areas
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid            TEXT NOT NULL UNIQUE,
    version_uuid    TEXT NOT NULL,

    name            TEXT NOT NULL UNIQUE
);


-- ============================================================
-- 2. ПОДСТАНЦИИ
-- ============================================================

CREATE TABLE substations
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid            TEXT NOT NULL UNIQUE,
    version_uuid    TEXT NOT NULL,

    area_id         INTEGER NOT NULL,
    name            TEXT NOT NULL,

    FOREIGN KEY (area_id)
        REFERENCES areas(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    UNIQUE(area_id, name)
);


-- ============================================================
-- 3. ПРИСОЕДИНЕНИЯ
-- ============================================================

CREATE TABLE connections
(
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid                 TEXT NOT NULL UNIQUE,
    version_uuid         TEXT NOT NULL,

    substation_id        INTEGER NOT NULL,
    connection_type      INTEGER NOT NULL
                         CHECK (connection_type IN (1, 2, 3)),
    cell_number          INTEGER,
    name                 TEXT NOT NULL CHECK (length(trim(name)) > 0),
    voltage_kv           REAL NOT NULL CHECK (voltage_kv > 0),

    ct_primary_current   INTEGER,
    ct_secondary_current INTEGER NOT NULL DEFAULT 5
                         CHECK (ct_secondary_current IN (0, 1, 5)),

    FOREIGN KEY (substation_id)
        REFERENCES substations(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    CHECK (
        (connection_type = 1 AND cell_number IS NOT NULL AND cell_number > 0)
        OR (connection_type IN (2, 3) AND cell_number IS NULL)
    ),
    CHECK (
        (ct_secondary_current = 0 AND ct_primary_current IS NULL)
        OR (ct_secondary_current IN (1, 5) AND
            ct_primary_current IS NOT NULL AND ct_primary_current > 0)
    )
);

-- Номер ячейки уникален в пределах всей ПС независимо от напряжения.
CREATE UNIQUE INDEX ux_connections_cell_number
    ON connections(substation_id, cell_number)
    WHERE connection_type = 1;

-- Уникальность названий ВЛ и прочих присоединений
-- в пределах одной подстанции и одного типа.
-- Для ячеек допускаются одинаковые названия:
-- их уникальность определяется номером ячейки.
CREATE UNIQUE INDEX ux_connections_type_name
    ON connections(substation_id, connection_type, name COLLATE NOCASE)
    WHERE connection_type IN (2, 3);



-- ============================================================
-- 4. ПРЕДСТАВИТЕЛИ ПРЕДПРИЯТИЯ
-- ============================================================

CREATE TABLE employees
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid            TEXT NOT NULL UNIQUE,
    version_uuid    TEXT NOT NULL,

    short_name      TEXT NOT NULL,       -- Иванов И.И.
    position        TEXT NOT NULL,       -- текущая должность

    is_active       INTEGER NOT NULL DEFAULT 1
                    CHECK (is_active IN (0, 1))
);


-- ============================================================
-- 5. ПРИБОРЫ УЧЁТА
-- ============================================================

CREATE TABLE meters
(
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid                TEXT NOT NULL UNIQUE,
    version_uuid        TEXT NOT NULL,

    name                TEXT NOT NULL,       -- СЭТ-4ТМ.03М
    serial_number       TEXT NOT NULL UNIQUE,

    accuracy_class      TEXT,                -- 0.5S
    verification_year   INTEGER,             -- год поверки

    manufacturer        TEXT,                -- можно оставить на будущее
    manufacture_year    INTEGER,             -- можно оставить на будущее

    note                TEXT
);


-- ============================================================
-- 6. ТИПЫ АКТОВ
-- ============================================================
--
-- Вместо хранения строк вроде "Замена", "Проверка" непосредственно
-- в каждом акте используем справочник.
--

CREATE TABLE act_types
(
    id      INTEGER PRIMARY KEY,
    name    TEXT NOT NULL UNIQUE
);


INSERT INTO act_types (id, name) VALUES
    (1, 'Проверка прибора учета'),
    (2, 'Замена прибора учета'),
    (3, 'Снятие показаний'),
    (4, 'Демонтаж прибора учета'),
    (5, 'Установка прибора учета'),
    (6, 'Установка прибора учета и ТТ'),
    (7, 'Замена ТТ');


-- ============================================================
-- 7. АКТЫ
-- ============================================================

CREATE TABLE acts
(
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid                    TEXT NOT NULL UNIQUE,
    version_uuid            TEXT NOT NULL,

    act_number              TEXT,

    act_type_id             INTEGER NOT NULL,
    act_date                TEXT NOT NULL,       -- YYYY-MM-DD
    substation_id           INTEGER NOT NULL,

    -- Для актов снятия показаний NULL.
    -- Присоединения таких актов хранятся в act_meters.
    connection_id           INTEGER,

    -- Исторический снимок места проведеня работ.
    -- Значения фиксируются на момент сохдания/редактирвоания акта
    -- и используются при просмотре архива и повторной генерации DOCX
    area_name               TEXT NOT NULL,
    substation_name         TEXT NOT NULL,
    connection_name         TEXT,
    connection_voltage_kv   REAL,
    connection_ct_ratio     TEXT,

    -- Представитель предприятия.
    -- employee_id указывает на текущий справочник сотрудников.
    employee_id             INTEGER,

    -- А эти поля являются историческим снимком.
    employee_name           TEXT NOT NULL,
    employee_position       TEXT NOT NULL,

    -- Причина проведения работ
    reason                  TEXT,

    -- Результат / заключение
    result                  TEXT,
    
    -- Пломба
    seal_number                         TEXT,

    -- Путь к сформированному DOCX
    file_path               TEXT,

    -- Дата создания записи
    created_at              TEXT NOT NULL
                            DEFAULT CURRENT_TIMESTAMP,
                            
 	-- Продолжительность замены приборы учета
	replacement_duration_minutes INTEGER
    	CHECK (
    		replacement_duration_minutes IS NULL
        	OR replacement_duration_minutes >= 0
        	),
        
    -- Выбор плановой/внеплановой работы    	
    work_schedule_type INTEGER
		CHECK (
    		work_schedule_type IS NULL
    		OR work_schedule_type IN (1, 2)
			),

    FOREIGN KEY (act_type_id)
        REFERENCES act_types(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    FOREIGN KEY (substation_id)
        REFERENCES substations(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    FOREIGN KEY (connection_id)
        REFERENCES connections(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    FOREIGN KEY (employee_id)
        REFERENCES employees(id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
);


-- ============================================================
-- 8. ПРИБОРЫ, УЧАСТВУЮЩИЕ В АКТЕ
-- ============================================================
--
-- Один акт может содержать:
--
-- проверка     -> один прибор
-- показания    -> по одному прибору на каждое присоединение одной ПС
-- демонтаж     -> один прибор
-- установка    -> один прибор
-- замена       -> два прибора:
--                 снятый + установленный
--

CREATE TABLE act_meters
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    uuid            TEXT NOT NULL UNIQUE,

    act_id          INTEGER NOT NULL,
    meter_id        INTEGER NOT NULL,
    connection_id   INTEGER NOT NULL,

    -- Снимок присоединения на момент оформления акта.
    connection_name        TEXT NOT NULL,
    connection_voltage_kv  REAL,
    connection_ct_ratio    TEXT,

    -- Роль прибора именно в данном акте:
    --
    -- 1 = проверяемый
    -- 2 = снятый
    -- 3 = установленный
    -- 4 = прибор, с которого сняты показания
    --
    role            INTEGER NOT NULL
                    CHECK (role IN (1, 2, 3, 4, 5)),

    -- Исторический снимок параметров прибора.
    -- Это позволит через несколько лет заново сформировать старый акт,
    -- даже если параметры в справочнике meters были изменены.
    meter_name              TEXT NOT NULL,
    serial_number           TEXT NOT NULL,
    accuracy_class          TEXT,
    verification_year       INTEGER,

   

    FOREIGN KEY (act_id)
        REFERENCES acts(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,

    FOREIGN KEY (meter_id)
        REFERENCES meters(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    FOREIGN KEY (connection_id)
        REFERENCES connections(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
);


-- ============================================================
-- 9. ТИПЫ ПОКАЗАНИЙ
-- ============================================================

CREATE TABLE reading_types
(
    id          INTEGER PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,

    -- Условное обозначение удобно для интерфейса / DOCX
    code        TEXT NOT NULL UNIQUE
);


INSERT INTO reading_types (id, name, code) VALUES
    (1, 'Активная энергия, приём',   'A+'),
    (2, 'Активная энергия, отдача',  'A-'),
    (3, 'Реактивная энергия, приём', 'R+'),
    (4, 'Реактивная энергия, отдача','R-');


-- ============================================================
-- 10. ПОКАЗАНИЯ ПРИБОРА В АКТЕ
-- ============================================================

CREATE TABLE meter_readings
(
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,

    act_meter_id        INTEGER NOT NULL,
    reading_type_id     INTEGER NOT NULL,

    value               REAL NOT NULL,

    FOREIGN KEY (act_meter_id)
        REFERENCES act_meters(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,

    FOREIGN KEY (reading_type_id)
        REFERENCES reading_types(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    -- У одного прибора в одном акте не должно быть двух A+,
    -- двух A- и т.д.
    UNIQUE(act_meter_id, reading_type_id)
);


-- ============================================================
-- 11. ВЕКТОРНАЯ ДИАГРАММА
-- ============================================================
--
-- Заполняется только для тех актов, где она нужна.
--
-- Пока поля базовые. После того как определимся с точным
-- составом измерений, эту таблицу можно расширить.
--

CREATE TABLE vector_diagrams
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    act_id          INTEGER NOT NULL UNIQUE,

    ia               REAL NOT NULL,
    angle_a          REAL NOT NULL,
    angle_a_type     TEXT NOT NULL
                     CHECK (angle_a_type IN ('L', 'C')),

    ib               REAL NOT NULL,
    angle_b          REAL NOT NULL,
    angle_b_type     TEXT NOT NULL
                     CHECK (angle_b_type IN ('L', 'C')),

    ic               REAL NOT NULL,
    angle_c          REAL NOT NULL,
    angle_c_type     TEXT NOT NULL
                     CHECK (angle_c_type IN ('L', 'C')),

    uab              REAL NOT NULL,
    ubc              REAL NOT NULL,
    uca              REAL NOT NULL,

    FOREIGN KEY (act_id)
        REFERENCES acts(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
);


-- ============================================================
-- 12. СТОРОННИЕ ПРЕДСТАВИТЕЛИ
-- ============================================================
--
-- Их не используем как постоянный справочник.
-- Но если нужно вписывать их в акт и иметь возможность
-- повторно сформировать DOCX, сведения всё же желательно
-- сохранить вместе с конкретным актом.
--
-- Таблица необязательная: у акта может не быть ни одной записи.
--

CREATE TABLE external_representatives
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    act_id          INTEGER NOT NULL,

    organization    TEXT,
    short_name      TEXT,
    position        TEXT,

    FOREIGN KEY (act_id)
        REFERENCES acts(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
);


-- ============================================================
-- 13. ТРАНСФОРМАТОРЫ ТОКА
-- ============================================================

CREATE TABLE current_transformers
(
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,

    uuid                    TEXT NOT NULL UNIQUE,
    version_uuid            TEXT NOT NULL,

    name                    TEXT NOT NULL,
    serial_number           TEXT NOT NULL UNIQUE,
    transformation_ratio    TEXT NOT NULL,
    accuracy_class          TEXT,

    manufacturer            TEXT,
    manufacture_year        INTEGER,
    note                    TEXT
);

CREATE TABLE act_current_transformers
(
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,

    act_id                  INTEGER NOT NULL,
    current_transformer_id  INTEGER NOT NULL,

    role                    INTEGER NOT NULL
                            CHECK (role IN (1, 2)),

    phase                   TEXT NOT NULL
                            CHECK (phase IN ('A', 'B', 'C')),

    name                    TEXT NOT NULL,
    serial_number           TEXT NOT NULL,
    transformation_ratio    TEXT NOT NULL,
    accuracy_class          TEXT,

    FOREIGN KEY (act_id)
        REFERENCES acts(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,

    FOREIGN KEY (current_transformer_id)
        REFERENCES current_transformers(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
);


-- ============================================================
-- 13. ИНДЕКСЫ ДЛЯ ПОИСКА
-- ============================================================

CREATE INDEX idx_substations_area
    ON substations(area_id);

CREATE INDEX idx_connections_substation
    ON connections(substation_id);

CREATE INDEX idx_meters_serial
    ON meters(serial_number);

CREATE INDEX idx_acts_date
    ON acts(act_date);

CREATE INDEX idx_acts_type
    ON acts(act_type_id);

CREATE INDEX idx_acts_connection
    ON acts(connection_id);

CREATE INDEX idx_act_meters_act
    ON act_meters(act_id);

CREATE INDEX idx_act_meters_meter
    ON act_meters(meter_id);

CREATE INDEX idx_readings_act_meter
    ON meter_readings(act_meter_id);

CREATE INDEX idx_act_ct_act
        ON act_current_transformers(act_id);

CREATE INDEX idx_act_ct_transformer
        ON act_current_transformers(current_transformer_id);

CREATE INDEX idx_current_transformers_serial
        ON current_transformers(serial_number);










-- ============================================================
-- 14. СИНХРОНИЗАЦИЯ (ОФЛАЙН)
-- ============================================================
-- database_uuid общий для копий одной исходной базы.
-- Первоначальные INSERT для areas/substations/connections должны содержать
-- заранее зафиксированные UUID и version_uuid (одинаковые для всех ПК).
-- После начала эксплуатации справочники изменяются через приложение,
-- которое добавляет соответствующие события в sync_changes.
-- node_uuid конкретного ПК хранится вне распространяемой БД.
-- Строка sync_metadata (id=1) создаётся приложением при создании базы.
-- События из первоначальных INSERT не требуются: они являются общей основой.

CREATE TABLE sync_metadata
(
    id               INTEGER PRIMARY KEY CHECK (id = 1),
    database_uuid    TEXT NOT NULL UNIQUE,
    protocol_version INTEGER NOT NULL DEFAULT 1
                     CHECK (protocol_version >= 1)
);

-- Неизменяемые события. UUID события используется для дедупликации.
-- payload хранит данные, необходимые для воспроизведения операции.
-- Для acts это полный снимок акта с дочерними строками.
-- Ссылки внутри payload передаются по UUID, а не по локальным id.
CREATE TABLE sync_changes
(
    uuid              TEXT PRIMARY KEY,
    node_uuid         TEXT NOT NULL,
    transaction_uuid  TEXT NOT NULL,

    entity_type       TEXT NOT NULL
                      CHECK (entity_type IN
                      ('areas', 'substations', 'connections',
                       'employees', 'meters', 'current_transformers', 'acts')),
    entity_uuid       TEXT NOT NULL,

    operation         TEXT NOT NULL
                      CHECK (operation IN ('insert', 'update', 'delete')),
    old_version_uuid  TEXT,
    new_version_uuid  TEXT,

    payload           TEXT NOT NULL CHECK (json_valid(payload)),
    created_at        TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Эталонное содержимое акта после ПЕРВОЙ успешной синхронизации.
-- Дальнейшие локальные правки акта не заменяют этот снимок.
-- snapshot содержит весь акт, кроме машинно-зависимых путей к DOCX.
CREATE TABLE sync_act_baselines
(
    act_uuid          TEXT PRIMARY KEY,
    version_uuid      TEXT NOT NULL,
    snapshot          TEXT NOT NULL CHECK (json_valid(snapshot)),
    published_at      TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_sync_changes_entity
    ON sync_changes(entity_type, entity_uuid);

CREATE INDEX idx_sync_changes_transaction
    ON sync_changes(transaction_uuid);

CREATE INDEX idx_act_meters_connection
    ON act_meters(connection_id);

-- Рекомендуемая сортировка в справочнике (используется в SELECT приложения):
-- ORDER BY connection_type ASC, voltage_kv DESC, cell_number ASC, name COLLATE NOCASE ASC;
