PRAGMA foreign_keys = ON;


-- ============================================================
-- 1. УЧАСТКИ
-- ============================================================

CREATE TABLE areas
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL UNIQUE
);


-- ============================================================
-- 2. ПОДСТАНЦИИ
-- ============================================================

CREATE TABLE substations
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    area_id     INTEGER NOT NULL,
    name        TEXT NOT NULL,

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
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    substation_id       INTEGER NOT NULL,

    name                TEXT NOT NULL,
    voltage_kv          REAL,
    ct_ratio            TEXT,

    FOREIGN KEY (substation_id)
        REFERENCES substations(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    UNIQUE(substation_id, name)
);


-- ============================================================
-- 4. ПРЕДСТАВИТЕЛИ ПРЕДПРИЯТИЯ
-- ============================================================

CREATE TABLE employees
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

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

    act_number              TEXT,

    act_type_id             INTEGER NOT NULL,
    act_date                TEXT NOT NULL,       -- YYYY-MM-DD

    connection_id           INTEGER NOT NULL,

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
-- показания    -> один прибор
-- демонтаж     -> один прибор
-- установка    -> один прибор
-- замена       -> два прибора:
--                 снятый + установленный
--

CREATE TABLE act_meters
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    act_id          INTEGER NOT NULL,
    meter_id        INTEGER NOT NULL,

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








