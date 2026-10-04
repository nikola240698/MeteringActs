PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE areas
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL UNIQUE
);
INSERT INTO areas VALUES(1,'Костанайский ЛПУ'),
  (2,'Федеровский ЛПУ'),
  (3,'Костанайский ЛПУ Алтынсаринский участок'),
  (4,'Сарыкольский ЛПУ');
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
INSERT INTO substations VALUES(1,1,'110/10кВ "Городская"'),
  (2,3,'110/35/10кВ "Большая Чураковка"'),
  (3,2,'110/35/10кВ "Чеховка"'),
  (4,4,'110/35/10кВ "Урицк"'),
  (6,4,'35/10кВ "Маяк"'),
  (7,3,'35/10кВ "Димитрово"'),
  (8,3,'35/10кВ "Новоселовка"');
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
INSERT INTO connections VALUES(1,1,'ЦРП-2',10.0,'200/5'),
  (2,1,'Ввод-10 Т-1',10.0,'400/5'),
  (3,2,'ЦУ-2',10.0,NULL),
  (7,6,'Ввод 10кВ Т-1',10.0,'200/5'),
  (8,3,'ЦУ',10.0,'75/5');
CREATE TABLE employees
(
    id              INTEGER PRIMARY KEY AUTOINCREMENT,

    short_name      TEXT NOT NULL,       -- Иванов И.И.
    position        TEXT NOT NULL,       -- текущая должность

    is_active       INTEGER NOT NULL DEFAULT 1
                    CHECK (is_active IN (0, 1))
);
INSERT INTO employees VALUES(1,'Алиферец Н.А.','Вед.инженер ССРЗА',1),
  (2,'Мустафин Т.Д.','Вед.инженер ССРЗА',1),
  (3,'Нестеров Л.А.','Вед.инженер ССРЗА',1),
  (5,'Федотов А.С.','Вед.инженер ПС 220/110/35/10кВ "Заречная"',0),
  (6,'Абашов Н.М.','Эл.монтер ССРЗА',1);
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
INSERT INTO meters VALUES(1,'Для проверки','12345','0,5',2026,'ХЗ',2026,''),
  (2,'VBG','123','0.002',2026,'',NULL,''),
  (3,'МИР С-07','49215224063674','0,5S',2024,'НПО "МИР"',2024,''),
  (4,'МИР С-07','49214525137153','0,5S',2026,'НПО "МИР"',2024,''),
  (5,'Меркурий 234 ART2-04 PR','49400993-23г.','0.5S',2023,'ООО "НПК "ИНКОТЕКС"',2023,''),
  (6,'VBh','234234234','0.5',2000,'1213123',2000,''),
  (8,'ДАЛА СА4У-Э720 Т1','NK001107','1',2018,'Saiman',2018,'');
CREATE TABLE act_types
(
    id      INTEGER PRIMARY KEY,
    name    TEXT NOT NULL UNIQUE
);
INSERT INTO act_types VALUES(1,'Проверка прибора учета'),
  (2,'Замена прибора учета'),
  (3,'Снятие показаний'),
  (4,'Демонтаж прибора учета'),
  (5,'Установка прибора учета'),
  (6,'Установка прибора учета и ТТ'),
  (7,'Замена ТТ');
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
                            
        replacement_duration_minutes INTEGER
                                                        CHECK (
                                                            replacement_duration_minutes IS NULL
                                                            OR replacement_duration_minutes >= 0
                                                            ), work_schedule_type INTEGER
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
INSERT INTO acts VALUES(1,NULL,1,'2026-09-10',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Произведена плановая проверка прибора учета.','Замечаний нет.','МРЭТ/22-03',NULL,'2026-09-19 20:31:50',NULL,1),
  (2,NULL,1,'2026-09-21',2,1,'Алиферец Н.А.','вед.инженер ССРЗА','Установили.','И забыли','МРЭТ',NULL,'2026-09-21 11:29:18',NULL,NULL),
  (24,NULL,1,'2026-09-21',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','23','23','13',NULL,'2026-09-21 16:00:45',NULL,1),
  (25,NULL,4,'2026-09-21',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','123123','123123',NULL,NULL,'2026-09-21 16:06:22',NULL,NULL),
  (26,NULL,2,'2026-09-21',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Произведена внеплановая замена прибора учета по причине выхода из строя установленного прибора.','ОБЭЭ сделать переасчет э/э.','МРЭТ-22/03, ЭЧ-16',NULL,'2026-09-21 17:04:19',16,1),
  (27,NULL,2,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Поменяли','Всё хорошо','МРЭТ',NULL,'2026-09-22 02:42:29',16,2),
  (28,NULL,2,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Сняли','Поменяли','МРЭТ',NULL,'2026-09-22 03:01:37',NULL,2),
  (29,NULL,1,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА',unistr('Ghjcnj\u0009q'),'ewqeqwe','qeqe',NULL,'2026-09-22 03:56:00',NULL,1),
  (30,NULL,3,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','safsfsdf','sdfsdfsd','2123123',NULL,'2026-09-22 04:44:09',NULL,NULL),
  (32,NULL,4,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','123123','123123',NULL,NULL,'2026-09-22 05:23:26',NULL,NULL),
  (34,NULL,5,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','asfsafasfd','141233124','fsdf21',NULL,'2026-09-22 05:34:07',NULL,NULL),
  (35,NULL,6,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','123123','12313','VH"N',NULL,'2026-09-22 09:24:38',NULL,NULL),
  (36,NULL,6,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','123123','12313','VH"N',NULL,'2026-09-22 09:24:53',NULL,NULL),
  (37,NULL,7,'2026-09-15',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Replaced Поменяли','Just One Только один','MRET/22-03',NULL,'2026-09-22 10:50:25',NULL,2),
  (38,NULL,7,'2026-09-22',2,1,'Алиферец Н.А.','Вед.инженер ССРЗА','Replaced','Just one','MRET',NULL,'2026-09-22 10:50:38',NULL,1),
  (39,NULL,1,'2026-09-22',1,1,'Алиферец Н.А.','Вед.инженер ССРЗА','3123','123','123123',NULL,'2026-09-22 18:00:02',NULL,2),
  (40,NULL,3,'2026-10-02',7,2,'Мустафин Т.Д.','Вед.инженер ССРЗА','Захотелось так','Пожтому поменяли','132',NULL,'2026-10-02 17:35:31',NULL,NULL);
INSERT INTO acts VALUES(41,NULL,3,'2026-10-02',8,3,'Нестеров Л.А.','Вед.инженер ССРЗА','аыва','ыфваыав','а23',NULL,'2026-10-02 17:37:04',NULL,NULL);
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
INSERT INTO act_meters VALUES(29,2,4,1,'МИР С-07','49214525137153','0,5S',2026),
  (30,24,2,1,'VBG','123','',2026),
  (31,25,2,2,'VBG','123','',2026),
  (32,26,3,2,'МИР С-07','49215224063674','0,5S',2024),
  (33,26,4,3,'МИР С-07','49214525137153','0,5S',2026),
  (34,27,3,2,'МИР С-07','49215224063674','0,5S',2024),
  (35,27,4,3,'МИР С-07','49214525137153','0,5S',2026),
  (36,28,3,2,'МИР С-07','49215224063674','0,5S',2024),
  (37,28,4,3,'МИР С-07','49214525137153','0,5S',2026),
  (38,29,3,1,'МИР С-07','49215224063674','0,5S',2026),
  (39,30,2,4,'VBG','123','',2026),
  (41,32,1,2,'Для проверки','12345','0,5',2026),
  (43,34,2,3,'VBG','123','',2026),
  (44,35,1,3,'Для проверки','12345','0,5',2026),
  (45,36,1,3,'Для проверки','12345','0,5',2026),
  (47,38,1,5,'Для проверки','12345','0,5',2026),
  (48,39,1,1,'Для проверки','12345','0,5',2026),
  (51,37,2,5,'VBG','123','',2026),
  (52,1,3,1,'МИР С-07','49215224063674','0,5S',2024),
  (53,40,2,4,'VBG','123','0.002',2026),
  (54,41,1,4,'Для проверки','12345','0,5',2026);
CREATE TABLE reading_types
(
    id          INTEGER PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,

    -- Условное обозначение удобно для интерфейса / DOCX
    code        TEXT NOT NULL UNIQUE
);
INSERT INTO reading_types VALUES(1,'Активная энергия, приём','A+'),
  (2,'Активная энергия, отдача','A-'),
  (3,'Реактивная энергия, приём','R+'),
  (4,'Реактивная энергия, отдача','R-');
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
INSERT INTO meter_readings VALUES(59,29,1,123.35),
  (60,29,2,152.2),
  (61,30,1,132.0),
  (62,31,1,123.0),
  (63,32,1,111.0),
  (64,32,2,222.0),
  (65,32,3,333.0),
  (66,32,4,444.0),
  (67,33,1,5555.0),
  (68,33,2,666.0),
  (69,33,3,777.0),
  (70,33,4,888.0),
  (71,34,1,1233.0),
  (72,34,2,13212.0),
  (73,34,3,12312.0),
  (74,34,4,123123.0),
  (75,35,1,123321.0),
  (76,35,2,12312.0),
  (77,35,3,13123.0),
  (78,35,4,123132.0),
  (79,36,1,12312.0),
  (80,36,3,21313.0),
  (81,37,1,1231.0),
  (82,37,3,12313.0),
  (83,38,1,134424.0),
  (84,39,1,12313.0),
  (85,39,2,123123.0),
  (86,39,3,12312.0),
  (87,39,4,13123.0),
  (92,41,1,211.0),
  (93,41,3,12312.0),
  (98,43,1,123123.0),
  (99,43,2,123123.0),
  (100,43,3,123123.0),
  (101,43,4,123123.0),
  (102,44,1,12312.0),
  (103,44,2,132132.0),
  (104,44,3,123123.0),
  (105,44,4,12313.0),
  (106,45,1,12312.0),
  (107,45,2,132132.0),
  (108,45,3,123123.0),
  (109,45,4,12313.0),
  (114,47,1,3123.0),
  (115,47,3,1231.0),
  (116,48,1,213123.0),
  (117,48,2,213123.0),
  (118,48,3,13123.0),
  (119,48,4,123123.0),
  (128,51,1,111111.0),
  (129,51,2,222222.0),
  (130,51,3,33333.0),
  (131,51,4,44444.0),
  (132,52,1,1215.12),
  (133,52,2,652.25),
  (134,52,3,15432.0),
  (135,52,4,1651.21),
  (136,53,1,65465.564),
  (137,54,1,114.0);
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
INSERT INTO vector_diagrams VALUES(6,25,123.0,123.0,'L',2123.0,123.0,'L',123.0,12.0,'C',123.0,123.0,123.0),
  (7,26,200.0,50.0,'L',202.0,160.0,'L',203.0,60.0,'C',100.5,100.3,100.8),
  (8,27,208.4,60.0,'L',231.4,168.0,'L',257.0,60.0,'C',104.62,103.35,103.31),
  (10,35,123.0,123.0,'L',123.0,123.0,'L',123.0,123.0,'C',123.0,123.0,132.0),
  (12,39,123.0,123.0,'L',123.0,123.0,'L',213.0,123.0,'C',123.0,123.0,213.0),
  (15,37,111.0,222.0,'L',333.0,443.0,'L',555.0,666.0,'C',777.0,888.0,999.0),
  (16,1,215.0,45.0,'L',210.0,160.0,'L',206.0,85.0,'C',101.3,101.2,101.2);
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
INSERT INTO external_representatives VALUES(9,26,'"ЭПК-Forfait"','Корниенко А.Г.','руководитель ГУиК'),
  (10,26,'АО "КТЖ"','Коломиец Е.И.','инженер-механик'),
  (11,28,'Форфайт','Зюганов РИ','Инженер'),
  (12,28,'Кегок','Иванчук ДЕ','Рабочий'),
  (13,30,'Ajhafq','slkjfhsjkdf','Dfjsdhf'),
  (14,30,'afdasfd','sdfsf','sdfsdf'),
  (15,30,'sdfsdf','sdfsdf','sdfsdf'),
  (20,35,'123123','123123','12313'),
  (21,35,'qdasd','asd','asdad'),
  (24,39,'Forfait','Тымченко','заместитель'),
  (29,37,'Forfait','Остапенко','инженер'),
  (30,37,'KEGOC','Иванов','метролог'),
  (31,1,'ТОО "ЭПК-Forfait"','Куникеев Е.Н.','инженер по ПС'),
  (32,1,'АО "KEGOC"','Колесник Д.С.','инженер МСУ');
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
INSERT INTO current_transformers VALUES(1,'ТОЛ-10','123456','200/5','0,5','',1990,''),
  (2,'ТОЛ','123','200/5','0,5/10Р','СВЭЛ',1990,'Просто так'),
  (3,'jgjhgj','1234','ghjg','ghj','',1990,''),
  (4,'iuo','12345','ouoiu','uio','',1990,''),
  (5,'uuiyiu2312','456','yuiyui','yuiy','',1990,'');
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
INSERT INTO act_current_transformers VALUES(10,35,2,2,'A','ТОЛ','123','200/5','0,5/10Р'),
  (11,35,4,2,'B','iuo','12345','ouoiu','uio'),
  (12,35,5,2,'C','uuiyiu','456','yuiyui','yuiy'),
  (13,36,2,2,'A','ТОЛ','123','200/5','0,5/10Р'),
  (14,36,5,2,'C','uuiyiu','456','yuiyui','yuiy'),
  (19,38,2,1,'A','ТОЛ','123','200/5','0,5/10Р'),
  (20,38,4,1,'C','iuo','12345','ouoiu','uio'),
  (21,38,5,2,'A','uuiyiu','456','yuiyui','yuiy'),
  (22,38,1,2,'C','ТОЛ-10','123456','200/5','0,5'),
  (31,37,5,1,'B','uuiyiu','456','yuiyui','yuiy'),
  (32,37,4,1,'C','iuo','12345','ouoiu','uio'),
  (33,37,2,2,'B','ТОЛ','123','200/5','0,5/10Р'),
  (34,37,1,2,'C','ТОЛ-10','123456','200/5','0,5');
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('areas',6),
  ('substations',9),
  ('connections',8),
  ('employees',6),
  ('meters',8),
  ('current_transformers',8),
  ('acts',41),
  ('act_meters',54),
  ('meter_readings',137),
  ('act_current_transformers',34),
  ('vector_diagrams',16),
  ('external_representatives',32);
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
PRAGMA writable_schema=OFF;
COMMIT;
