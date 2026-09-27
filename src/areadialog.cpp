
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "areadialog.h"
#include "ui_areadialog.h"


AreaDialog::AreaDialog(Database &database, QWidget *parent)
    : QDialog(parent), ui(new Ui::AreaDialog), m_database(database)
{
    ui->setupUi(this);

    setWindowTitle("Добавление участка");
    ui->titleLabel->setText("Новый участок");

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->saveButton, &QPushButton::clicked, this,
        [this]()
        {
            if (save())
                accept();
        });
}

AreaDialog::AreaDialog(Database &database, int areaId, QWidget *parent)
    : AreaDialog(database, parent)
{
    m_areaId = areaId;

    setWindowTitle("Редактирование участка");
    ui->titleLabel->setText("Редактирование участка");

    loadArea();
}

AreaDialog::~AreaDialog()
{
    delete ui;
}

bool AreaDialog::save()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Введите название участка");

        ui->nameLineEdit->setFocus();

        return false;
    }

    if (m_areaId < 0)
        return insertArea();

    return updateArea();
}

bool AreaDialog::insertArea()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "INSERT INTO areas (name) "
        "VALUES (:name);");

    query.bindValue(":name", name);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось добавить участок.\n\n" + query.lastError().text());

        return false;
    }

    return true;
}

bool AreaDialog::updateArea()
{
    const QString name = ui->nameLineEdit->text().trimmed();

    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "UPDATE areas "
        "SET name = :name "
        "WHERE id = :areaId;");

    query.bindValue(":name", name);
    query.bindValue(":areaId", m_areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось изменить участок.\n\n" + query.lastError().text());

        return false;
    }

    return true;
}

void AreaDialog::loadArea()
{
    QSqlQuery query(m_database.getDatabase());

    query.prepare(
        "SELECT name "
        "FROM areas "
        "WHERE id = :areaId;");

    query.bindValue(":areaId", m_areaId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка",
            "Не удалось загрузить данные участка.\n\n" + query.lastError().text());

        return;
    }

    if (!query.next())
    {
        QMessageBox::warning(this, "Ошибка", "Участок не найден");

        return;
    }

    ui->nameLineEdit->setText(query.value("name").toString());
}











