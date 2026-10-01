
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>

#include "directorieswidget.h"
#include "ui_directorieswidget.h"
#include "areadialog.h"
#include "substationdialog.h"
#include "connectiondialog.h"
#include "employeedialog.h"


DirectoriesWidget::DirectoriesWidget(Database &database, QWidget *parent)
    : QWidget(parent), ui(new Ui::DirectoriesWidget), m_database(database)
{
    ui->setupUi(this);

    // -------------------------------------------
    // Вкладка "Объекты"
    // -------------------------------------------

    // Скрываем по умолчания кнопку "Назад"
    ui->backButton->setVisible(false);

    // создаем модели объектов
    m_leftModel = new QSqlQueryModel(this);
    m_rightModel = new QSqlQueryModel(this);
    // вставляем их в виджеты
    ui->leftTableView->setModel(m_leftModel);
    ui->rightTableView->setModel(m_rightModel);

    loadAreas();

    // Слот одиночного нажатия на строку в таблице участка
    connect(ui->leftTableView, &QTableView::clicked, this,
        [this](const QModelIndex &index)
        {
           if (!index.isValid())
           {
               return;
           }

            const int id = m_leftModel->index(index.row(), 0).data().toInt();

            if (m_level == DirectoryLevel::Areas)
            {
                loadSubstations(id);
            }
            else if (m_level == DirectoryLevel::Substations)
            {
                loadConnections(id);
            }
        });

    clearSubstations();

    // Подключаем двойной клик на строке левой таблицы
    connect(ui->leftTableView, &QTableView::doubleClicked, this,
        [this](const QModelIndex &index)
        {
            if (m_level != DirectoryLevel::Areas || !index.isValid())
            {
                return;
            }

            const int areaId = m_leftModel->index(index.row(), 0)
                .data().toInt();

            showSubstationsLevel(areaId);
        });

    // Подключаем слот нажатия кнопки "Назад"
    connect(ui->backButton, &QPushButton::clicked, this,
        [this]()
        {
            showAreasLevel();
        });

    // Подключаем кнопку добавления слева
    connect(ui->addLeftButton, &QPushButton::clicked, this,
        [this]()
        {
            if (m_level == DirectoryLevel::Areas)
            {
                AreaDialog dialog(m_database, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    loadAreas();
                    emit objectChanged();
                }
            }
            else if (m_level == DirectoryLevel::Substations)
            {
                if (m_currentAreaId < 0)
                    return;

                SubstationDialog dialog(
                    m_database, m_currentAreaId, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    showSubstationsLevel(m_currentAreaId);
                }
            }
        });

    // Подключаем кнопку редактирования слева
    connect(ui->editLeftButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex index =
                ui->leftTableView->currentIndex();

            if (!index.isValid())
            {
                if (m_level == DirectoryLevel::Areas)
                {
                    QMessageBox::information(this, "Редактирование",
                        "Выберите участок для редактирования.");
                }
                else
                {
                    QMessageBox::information(this, "редактирование",
                        "Выберите подстанцию для редактирования.");
                }
                return;
            }

            const int id =
                m_leftModel->index(index.row(), 0).data().toInt();

            // Редактируем участок
            if (m_level == DirectoryLevel::Areas)
            {
                AreaDialog dialog(m_database, id, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    loadAreas();
                    clearSubstations();
                    emit objectChanged();
                }
            }
            // Редактируем подстанцию
            else if (m_level == DirectoryLevel::Substations)
            {
                if ( m_currentAreaId < 0)
                    return;

                editSubstation(m_currentAreaId, id);
            }
        });

    // Слот кнопки удаления слева
    connect(ui->deleteLeftButton, &QPushButton::clicked, this,
        [this]()
        {
            if (m_level == DirectoryLevel::Substations)
            {
                // Удаление подстанции

                const QModelIndex index =
                    ui->leftTableView->currentIndex();

                if (!index.isValid())
                {
                    QMessageBox::information(this, "Удаление",
                        "Выберите подстанцию для удаления.");

                    return;
                }

                if (m_currentAreaId < 0)
                    return;

                const int substationId =
                    m_leftModel->index(index.row(), 0).data().toInt();

                const QString substationName =
                    m_leftModel->index(index.row(), 1).data().toString();

                deleteSubstation(m_currentAreaId, substationId, substationName);

                return;
            }

            // Удаление участка

            const QModelIndex index = ui->leftTableView->currentIndex();

            if (!index.isValid())
            {
                QMessageBox::information(this, "Удаление",
                    "Выберите участок для удаления");

                return;
            }

            const int row = index.row();

            const int areaId =
                m_leftModel->index(row, 0).data().toInt();

            const QString areaName =
                m_leftModel->index(row, 1).data().toString();

            const auto answer = QMessageBox::question(
                this, "Удаление участка", QString(
                    "Удалить участок \"%1\"?\n\n"
                    "Это действие нельзя отменить.")
                    .arg(areaName),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (answer != QMessageBox::Yes)
                return;

            // Проверяем на наличие существующих ПС данного участка
            QSqlQuery checkQuery(m_database.getDatabase());

            checkQuery.prepare(
                "SELECT EXISTS("
                    "SELECT 1 "
                    "FROM substations "
                    "WHERE area_id = :areaId"
                ");");

            checkQuery.bindValue(":areaId", areaId);

            if (!checkQuery.exec() || !checkQuery.next())
            {
                QMessageBox::critical(this, "Ошибка",
                    "Не удалось проверить наличие подстанций.\n\n" +
                    checkQuery.lastError().text());

                return;
            }

            if (checkQuery.value(0).toBool())
            {
                QMessageBox::warning(this, "Удаление не возможно",
                    QString(
                        "Невозможно удалить участок \"%1\", "
                        "пока в нём имеются подстанции.")
                        .arg(areaName));

                return;
            }

            QSqlQuery query(m_database.getDatabase());

            query.prepare(
                "DELETE FROM areas "
                "WHERE id = :areaId;");

            query.bindValue(":areaId", areaId);

            if (!query.exec())
            {
                QMessageBox::critical(this, "Ошибка",
                    "Не удалось удалить участок.\n\n" +
                    query.lastError().text());


                return;
            }

            loadAreas();
            clearSubstations();
            emit objectChanged();
        });

    // Слот кнопки добавления справа
    connect(ui->addRightButton, &QPushButton::clicked, this,
        [this]()
        {
            // Добавляем подстанцию
            if (m_level == DirectoryLevel::Areas)
            {
                const QModelIndex index =
                    ui->leftTableView->currentIndex();

                if (!index.isValid())
                {
                    QMessageBox::information(this, "Добавление подстанции",
                        "Сначала выберите участок.");

                    return;
                }

                const int areaId = m_leftModel->index(index.row(), 0)
                    .data().toInt();

                SubstationDialog dialog(m_database, areaId, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    loadSubstations(areaId);
                    emit objectChanged();
                }
                    return;
            }

            // Добавляем присоединение
            if (m_level == DirectoryLevel::Substations)
            {
                const QModelIndex index =
                    ui->leftTableView->currentIndex();

                if (!index.isValid())
                {
                    QMessageBox::information(this, "Добавление присоединения",
                        "Сначала выберите подстацнию.");

                    return;
                }

                const int substationId =
                    m_leftModel->index(index.row(), 0).data().toInt();

                ConnectionDialog dialog(
                    m_database, substationId, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    loadConnections(substationId);
                    emit objectChanged();
                }
            }
        });

    // Подключаем кнопку редактирования справа
    connect(ui->editRightButton, &QPushButton::clicked, this,
        [this]()
        {
            // Редактирование подстанции
            if (m_level == DirectoryLevel::Areas)
            {
                const QModelIndex areaIndex =
                    ui->leftTableView->currentIndex();

                if (!areaIndex.isValid())
                {
                    QMessageBox::information(this, "Редактирование",
                        "Сначала выберите участок");

                    return;
                }

                const QModelIndex substationIndex =
                    ui->rightTableView->currentIndex();

                if (!substationIndex.isValid())
                {
                    QMessageBox::information(this, "Редактирование",
                        "Выберите подстанцию для редактирования.");

                    return;
                }

                const int areaId =
                    m_leftModel->index(areaIndex.row(), 0).data().toInt();

                const int substationId =
                    m_rightModel->index(substationIndex.row(), 0).data().toInt();

                editSubstation(areaId, substationId);

                return;
            }

            // Редактирование присоединения
            if (m_level == DirectoryLevel::Substations)
            {
                const QModelIndex substationIndex =
                    ui->leftTableView->currentIndex();

                if (!substationIndex.isValid())
                {
                    QMessageBox::information(this, "Редактирование",
                        "Сначала выберите подстанцию.");

                    return;
                }

                const QModelIndex connectionIndex =
                    ui->rightTableView->currentIndex();

                if (!connectionIndex.isValid())
                {
                    QMessageBox::information(this, "Редактирование",
                        "Выберите присоединение для редактирования.");

                    return;
                }

                const int substationId =
                    m_leftModel->index(substationIndex.row(), 0).data().toInt();

                const int connectionId =
                    m_rightModel->index(connectionIndex.row(), 0).data().toInt();

                ConnectionDialog dialog(
                    m_database, substationId, connectionId, this);

                if (dialog.exec() == QDialog::Accepted)
                {
                    loadConnections(substationId);
                    emit objectChanged();
                }
            }
        });

    // Подключаем кнопку удаления справа
    connect(ui->deleteRightButton, &QPushButton::clicked, this,
        [this]()
        {
            // Удаление подстанции
            if (m_level == DirectoryLevel::Areas)
            {
                const QModelIndex areaIndex =
                    ui->leftTableView->currentIndex();

                if (!areaIndex.isValid())
                {
                    QMessageBox::information(this, "Удаление",
                        "Сначала выберите участок.");

                    return;
                }

                const QModelIndex substationIndex =
                    ui->rightTableView->currentIndex();

                if (!substationIndex.isValid())
                {
                    QMessageBox::information(this, "Удаление",
                        "Выберите подстанцию для удаления.");

                    return;
                }

                const int areaId =
                    m_leftModel->index(areaIndex.row(), 0).data().toInt();

                const int substationId =
                    m_rightModel->index(substationIndex.row(), 0).data().toInt();

                const QString substationName =
                    m_rightModel->index(substationIndex.row(), 1).data().toString();

                deleteSubstation(areaId, substationId, substationName);

                return;
            }

            // Удаление присоединения
            if (m_level == DirectoryLevel::Substations)
            {
                const QModelIndex substationIndex =
                    ui->leftTableView->currentIndex();

                if (!substationIndex.isValid())
                {
                    QMessageBox::information(this, "Удаление",
                        "Сначала выберите подстанцию.");

                    return;
                }

                const QModelIndex connectionIndex =
                    ui->rightTableView->currentIndex();

                if (!connectionIndex.isValid())
                {
                    QMessageBox::information(this, "Удаление",
                        "Выберите присоединение для удаления.");

                    return;
                }

                const int substationId =
                    m_leftModel->index(substationIndex.row(), 0).data().toInt();

                const int connectionId =
                    m_rightModel->index(connectionIndex.row(), 0).data().toInt();

                const QString connectionName =
                    m_rightModel->index(connectionIndex.row(), 1).data().toString();

                deleteConnection(
                    substationId, connectionId, connectionName);
                emit objectChanged();
            }
        });

    // -------------------------------------------
    // Вкладка "Сотрудники"
    // -------------------------------------------

    // создаем модель представителей
    m_employeesModel = new QSqlQueryModel(this);
    ui->employeesTableView->setModel(m_employeesModel);

    loadEmployees();
    setupEmployeesTable();
    updateEmployeeButtons();

    // Подключаем кнопку "Добавить"
    connect(ui->addEmployeeButton, &QPushButton::clicked, this,
        [this]()
        {
            EmployeeDialog dialog(m_database, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                loadEmployees();
                emit employeesChanged();
            }
        });

    // Подключаем кнопку "Изменить"
    connect(ui->editEmployeeButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex currentIndex =
                ui->employeesTableView->currentIndex();

            if (!currentIndex.isValid())
            {
                QMessageBox::information(this, "Сотрудники",
                    "Выберите сотрудника для редактирования.");

                return;
            }

            const int row = currentIndex.row();

            const int employeeId =
                m_employeesModel->index(row, 0).data().toInt();

            EmployeeDialog dialog(m_database, employeeId, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                loadEmployees();
                emit employeesChanged();
            }
        });

    // подключаем клик по строке для активации кнопки изменения активности
    connect(ui->employeesTableView, &QTableView::clicked, this,
        [this](const QModelIndex &)
        {
            updateEmployeeButtons();
        });

    // Подключаем кнопку изменения активности персонала
    connect(ui->toggleEmployeeButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex index =
                ui->employeesTableView->currentIndex();

            if (!index.isValid())
            {
                QMessageBox::information(this, "Сотрудники",
                    "Выберите сотрудника.");

                return;
            }

            const int row = index.row();

            const int employeeId =
                m_employeesModel->index(row, 0).data().toInt();

            const QString employeeName =
                m_employeesModel->index(row, 1).data().toString();

            const bool isActive =
                m_employeesModel->index(row, 4).data().toBool();

            const QString question = isActive
                ? QString("Перевести сотрудника \"%1\" в неактивные?")
                    .arg(employeeName)
                : QString("Восстановить сотрудника \"%1\"?")
                    .arg(employeeName);

            const auto answer = QMessageBox::question(this,
                isActive ? "Увольнение сотрудника" : "Восстановление сотрудника",
                question,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (answer != QMessageBox::Yes)
                return;

            QSqlQuery query(m_database.getDatabase());

            query.prepare(
                "UPDATE employees "
                "SET is_active = :isActive "
                "WHERE id = :employeeId;");

            query.bindValue(":isActive", isActive ? 0 : 1);
            query.bindValue(":employeeId", employeeId);

            if (!query.exec())
            {
                QMessageBox::critical(this, "Ошибка",
                    "Не удалось изменить статус сотрудника.\n\n" +
                    query.lastError().text());

                return;
            }

            loadEmployees();
            updateEmployeeButtons();
            emit employeesChanged();
        });

    // Подключаем кнопку удаления сотрудника
    connect(ui->deleteEmployeeButton, &QPushButton::clicked, this,
        [this]()
        {
            const QModelIndex index = ui->employeesTableView->currentIndex();

            if (!index.isValid())
            {
                QMessageBox::information(this, "Удаление сотрудника",
                    "Выберите сотрудника для удаления.");

                return;
            }

            const int row = index.row();

            const int employeeId =
                m_employeesModel->index(row, 0).data().toInt();

            const QString employeeName =
                m_employeesModel->index(row, 1).data().toString();

            // Проверяем, использовался ли сотрудник в актах
            QSqlQuery checkQuery(m_database.getDatabase());

            checkQuery.prepare(
                "SELECT EXISTS("
                "SELECT 1 "
                "FROM acts "
                "WHERE employee_id = :employeeId);");

            checkQuery.bindValue(":employeeId", employeeId);

            if (!checkQuery.exec() || !checkQuery.next())
            {
                QMessageBox::critical(this, "Ошибка",
                    "Не удалось проверить использование сотрудника.\n\n" +
                    checkQuery.lastError().text());

                return;
            }

            if (checkQuery.value(0).toBool())
            {
                QMessageBox::warning(this, "Удаление невозможно",
                    QString(
                        "Невозможно удалить сотрудника \"%1\", "
                        "так как он используется в актах.\n\n"
                        "Если сотрудник больше не работает, "
                        "переведите его в неактивные.")
                        .arg(employeeName));

                return;
            }

            // Сотрудник нигде не используется - спрашиваем разрешение
            const auto answer = QMessageBox::question(this, "Удаление сотрудника",
                QString(
                    "Удалить сотрудника \"%1\"?\n\n"
                    "Это действие нельзя отменить.")
                    .arg(employeeName),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (answer != QMessageBox::Yes)
                return;

            QSqlQuery query(m_database.getDatabase());

            query.prepare(
                "DELETE FROM employees "
                "WHERE id = :employeeId;");

            query.bindValue(":employeeId", employeeId);

            if (!query.exec())
            {
                QMessageBox::critical(this, "Ошибка",
                    "Неудалось удалить сотрудника.\n\n" +
                    query.lastError().text());

                return;
            }

            loadEmployees();
            emit employeesChanged();
        });


}

DirectoriesWidget::~DirectoriesWidget()
{
    delete ui;
}

void DirectoriesWidget::loadAreas()
{
    QSqlQuery query(m_database.getDatabase());

    if (!query.exec(
        "SELECT id, name "
        "FROM areas "
        "ORDER BY name;"))
    {
        qDebug() << "loadAreas error: " << query.lastError().text();

        return;
    }
    // Вставляем результат запроса в модель
    m_leftModel->setQuery(std::move(query));
    // Выставляем название столбца
    m_leftModel->setHeaderData(1, Qt::Horizontal, "Название участка");
    // Скрываем столбец с ID
    ui->leftTableView->hideColumn(0);
    // Растягиваем последний столбец согласно содержимому
    ui->leftTableView->horizontalHeader()->setStretchLastSection(true);


}

void DirectoriesWidget::loadSubstations(int areaId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name "
        "FROM substations "
        "WHERE area_id = :areaId "
        "ORDER BY name;");

    query.bindValue(":areaId", areaId);

    if (!query.exec())
    {
        qDebug() << "loadSubstations error: " << query.lastError().text();

        return;
    }
    // Вставляем результат запроса
    m_rightModel->setQuery(std::move(query));
    // Меняем название столбца
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");
    // Выводим данные в модель
    ui->rightTableView->setModel(m_rightModel);
    // Прячем столбец с ID
    ui->rightTableView->hideColumn(0);
    // Растягиваем последний столбец согласно ширине содержимого
    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);
}

void DirectoriesWidget::clearSubstations()
{
    // Сразу выполняем и передаем запрос в модель
    // таким запросом "очищаем" таблицу
    m_rightModel->setQuery(
        "SELECT id, name "
        "FROM substations "
        "WHERE 0;",
        m_database.getDatabase());
    // меняем заголовок столбца
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");
    // скрываем столбец id
    ui->rightTableView->hideColumn(0);
    // Растягиваем последний столбец согласно его содержимого
    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);

}

void DirectoriesWidget::showAreasLevel()
{
    m_level = DirectoryLevel::Areas;
    m_currentAreaId = -1;

    ui->backButton->setVisible(false);

    ui->leftTitleLabel->setText("Участки");
    ui->rightTitleLabel->setText("Подстанции");

    loadAreas();
    clearSubstations();
}

void DirectoriesWidget::showSubstationsLevel(int areaId)
{
    m_level = DirectoryLevel::Substations;
    m_currentAreaId = areaId;

    ui->backButton->setVisible(true);

    ui->leftTitleLabel->setText("Подстанции");
    ui->rightTitleLabel->setText("Присоединения");

    // Загружаем ПС теперь в левую таблицу
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name "
        "FROM substations "
        "WHERE area_id = :areaId "
        "ORDER BY name;");

    query.bindValue(":areaId", areaId);

    if (!query.exec())
    {
        qDebug() << "showSubstationsLevel error: " << query.lastError().text();

        return;
    }

    m_leftModel->setQuery(std::move(query));

    m_leftModel->setHeaderData(1, Qt::Horizontal, "Название подстанции");

    ui->leftTableView->hideColumn(0);

    clearConnections();
}



void DirectoriesWidget::loadConnections(int substationId)
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT id, name, voltage_kv, ct_ratio "
        "FROM connections "
        "WHERE substation_id = :substationId "
        "ORDER BY name;");

    query.bindValue(":substationId", substationId);

    if (!query.exec())
    {
        qDebug() << "loadConnections error: " << query.lastError().text();

        return;
    }

    m_rightModel->setQuery(std::move(query));

    setupConnectionsTable();
}

void DirectoriesWidget::clearConnections()
{
    m_rightModel->setQuery(
        "SELECT id, name, voltage_kv, ct_ratio "
        "FROM connections "
        "WHERE 0;",
        m_database.getDatabase());

   setupConnectionsTable();
}

void DirectoriesWidget::setupConnectionsTable()
{
    m_rightModel->setHeaderData(1, Qt::Horizontal, "Присоединение");
    m_rightModel->setHeaderData(2, Qt::Horizontal, "Напряжение, кВ");
    m_rightModel->setHeaderData(3, Qt::Horizontal, "Ктт");

    ui->rightTableView->hideColumn(0);

    ui->rightTableView->horizontalHeader()->setStretchLastSection(true);
}

void DirectoriesWidget::editSubstation(int areaId, int substationId)
{
    SubstationDialog dialog(m_database, areaId, substationId, this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    if (m_level == DirectoryLevel::Areas)
    {
        // На первом уровне ПС находятся справа
        loadSubstations(areaId);
    }
    else if (m_level == DirectoryLevel::Substations)
    {
        // На втором уровне ПС находятся слева
        showSubstationsLevel(areaId);
    }

}

void DirectoriesWidget::deleteSubstation(int areaId, int substationId, const QString &substationName)
{
    const auto answer = QMessageBox::question(this, "Удаление подстанции",
        QString(
            "Удалить подстанцию \"%1\"?\n\n"
            "Это действие нельзя отменить.")
            .arg(substationName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    // Проверяем, есть ли у ПС присоединения
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM connections "
            "WHERE substation_id = :substationId"
        ");");

    checkQuery.bindValue(":substationId", substationId);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить наличие присоединений.\n\n" +
            checkQuery.lastError().text());

        return;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Удаление невозможно",
            QString(
                "Невозможно удалить подстанцию \"%1\", "
                "пока в ней имеются присоединения.")
                .arg(substationName));

        return;
    }

    // Если нет зависимых присоединений - удаляем ПС
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "DELETE FROM substations "
        "WHERE id = :substationId "
        "AND area_id = :areaId;");

    query.bindValue(":substationId", substationId);

    query.bindValue(":areaId", areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось удалить подстанцию.\n\n" +
            query.lastError().text());

        return;
    }

    // Обновляем текущий интерфейс
    if (m_level == DirectoryLevel::Areas)
    {
        loadSubstations(areaId);
    }
    else if (m_level == DirectoryLevel::Substations)
    {
        showSubstationsLevel(areaId);
    }
}

void DirectoriesWidget::deleteConnection(int substationId, int connectionId, const QString &connectionName)
{
    const auto answer = QMessageBox::question(this, "Удаление присоединения",
        QString(
            "Удалить присоединение \"%1\"?\n\n"
            "Это действие нельзя отменить.")
            .arg(connectionName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    // Проверяем, используется ли присоединение в актах
    QSqlQuery checkQuery(m_database.getDatabase());

    checkQuery.prepare(
        "SELECT EXISTS("
            "SELECT 1 "
            "FROM acts "
        "WHERE connection_id = :connectionId);");

    checkQuery.bindValue(":connectionId", connectionId);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось проверить использование присоединения.\n\n" +
            checkQuery.lastError().text());

        return;
    }

    if (checkQuery.value(0).toBool())
    {
        QMessageBox::warning(this, "Удаление невозможно",
            QString(
                "Невозможно удалить присоединение \"%1\", "
                "так как оно используется в актах.")
                .arg(connectionName));

        return;
    }

    // Удаляем, если нигде не используется
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "DELETE FROM connections "
        "WHERE id = :connectionId "
        "AND substation_id = :substationId;");

    query.bindValue(":connectionId", connectionId);

    query.bindValue(":substationId", substationId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось удалить присоединение.\n\n" +
            query.lastError().text());

        return;
    }

    loadConnections(substationId);
}

void DirectoriesWidget::loadEmployees()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT "
            "id, "
            "short_name,"
            "position, "
            "CASE "
                "WHEN is_active = 1 THEN 'Работает' "
                "ELSE 'Не работает' "
            "END AS status, "
            "is_active "
        "FROM employees "
        "ORDER BY is_active DESC, short_name;");
    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить сотрудников:\n" +
            query.lastError().text());

        return;
    }

    m_employeesModel->setQuery(std::move(query));

    m_employeesModel->setHeaderData(1, Qt::Horizontal, "Ф.И.О.");
    m_employeesModel->setHeaderData(2, Qt::Horizontal, "Должность");
    m_employeesModel->setHeaderData(3, Qt::Horizontal, "Статус");

    ui->employeesTableView->hideColumn(0);
    ui->employeesTableView->hideColumn(4);

    ui->employeesTableView->clearSelection();
    ui->employeesTableView->setCurrentIndex(QModelIndex());

    setupEmployeesTable();
    updateEmployeeButtons();
}

void DirectoriesWidget::setupEmployeesTable()
{
    ui->employeesTableView->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    ui->employeesTableView->setSelectionMode(
        QAbstractItemView::SingleSelection);

    ui->employeesTableView->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    QHeaderView* header =
        ui->employeesTableView->horizontalHeader();

    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    header->setSectionResizeMode(2, QHeaderView::Stretch);

    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
}

void DirectoriesWidget::updateEmployeeButtons()
{
    const QModelIndex index =
        ui->employeesTableView->currentIndex();

    if (!index.isValid())
    {
        ui->editEmployeeButton->setEnabled(false);
        ui->toggleEmployeeButton->setEnabled(false);
        ui->deleteEmployeeButton->setEnabled(false);

        ui->toggleEmployeeButton->setText("Уволить");
        return;
    }

    const int row = index.row();

    const bool isActive =
        m_employeesModel->index(row, 4).data().toBool();

    ui->editEmployeeButton->setEnabled(true);
    ui->toggleEmployeeButton->setEnabled(true);
    ui->deleteEmployeeButton->setEnabled(true);

    if (isActive)
        ui->toggleEmployeeButton->setText("Уволить");
    else
        ui->toggleEmployeeButton->setText("Восстановить");
}




















