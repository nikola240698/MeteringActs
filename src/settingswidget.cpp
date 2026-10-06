
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QSettings>

#include "settingswidget.h"
#include "ui_settingswidget.h"


SettingsWidget::SettingsWidget(
    Database &database, QWidget *parent)
        : QWidget(parent),
          ui(new Ui::SettingsWidget),
          m_database(database),
          m_backupManager(database)
{
    ui->setupUi(this);

    // Загружаем настройки
    loadSettings();

    // Подключаем чекбокс выбора автоматического сохранения
    connect(ui->automaticBackupCheckBox, &QCheckBox::toggled, this,
        [this](bool checked)
        {
            ui->backupIntervalSpinBox->setEnabled(checked);
            ui->backupKeepCountSpinBox->setEnabled(checked);
            saveSettings();
        });

    // Подключаем СпинБокс выбора частоты сохранения
    connect(ui->backupIntervalSpinBox, &QSpinBox::valueChanged, this,
        [this]()
        {
            saveSettings();
        });

    // Подключаем СпинБокс выбора количества сохранений
    connect(ui->backupKeepCountSpinBox, &QSpinBox::valueChanged, this,
        [this]()
        {
            saveSettings();
        });

    ui->databasePathLineEdit->setText(m_database.databasePath());

    ui->backupPathLineEdit->setText(backupDirectory());

    ui->lastbackupLabel->setText("-");

    connect(ui->createBackupButton, &QPushButton::clicked,
        this, &SettingsWidget::createBackup);

    // Подключаем кнопку восстановления БД
    connect(ui->restoreBackupButton, &QPushButton::clicked,
        this, &SettingsWidget::restoreBackup);
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

QString SettingsWidget::backupDirectory() const
{
    const QFileInfo databaseInfo(m_database.databasePath());

    QDir projectDirectory = databaseInfo.dir();

    projectDirectory.cdUp();

    return projectDirectory.filePath("backups");
}


void SettingsWidget::createBackup()
{
    QString createdBackupPath;

    if (!m_backupManager.createBackup(backupDirectory(), createdBackupPath))
    {
        QMessageBox::critical(this, "Ошибка резервного копирования",
            m_backupManager.lastError());

        return;
    }

    const QFileInfo backupInfo(createdBackupPath);

    ui->lastbackupLabel->setText(backupInfo.fileName());

    QMessageBox::information(this, "Резервная копия создана",
        "Резервная копия базы данных успешно создана:\n\n"
        + createdBackupPath);
}

void SettingsWidget::restoreBackup()
{
    const QString filePath =
        QFileDialog::getOpenFileName(this, "Выберите резервную копию",
            backupDirectory(),
            "SQLite database (*.db);; Все файлы (*.*)");

    // При нажатии отмены
    if (filePath.isEmpty())
        return;

    // Только проверяем выбранный файл
    if (!m_backupManager.validateBackup(filePath))
    {
        QMessageBox::critical(this, "Некорректная резервная копия",
            m_backupManager.lastError());

        return;
    }

    const QFileInfo backupInfo(filePath);

    const auto answer = QMessageBox::warning(this, "Востановление базы данных",
        "Вы действительно хотите восстановить базу данных из резервной копии?\n\n"
        "Файл:\n"
        + backupInfo.fileName()
        +"\n\n"
         "Текущая база данных будет автоматически сохранена в отдельную"
         "страховочную копию.\n\n"
         "После восстановления программа будет закрыта.",
         QMessageBox::Yes | QMessageBox::No,
         QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    QString safetyBackupPath;

    if (!m_backupManager.restoreBackup(filePath, safetyBackupPath))
    {
        QMessageBox::critical(this, "Ошибка восстановления",
            m_backupManager.lastError());

        return;
    }

    QMessageBox::information(this, "Восстановление завершено",
        "База данных успешно восстановлена.\n\n"
        "Страховочная копия предыдущей базы сохранена:\n"
        + safetyBackupPath
        + "\n\n"
          "Программа сейчас будет закрыта.\n"
          "Запустите её снова, для работы с восстановленной базой.");

    QApplication::quit();
}

void SettingsWidget::loadSettings()
{
    QSettings settings;

    const bool automaticBackupEnabled = settings.value(
        "backup/automaticEnabled", true).toBool();

    const bool enabled = ui->automaticBackupCheckBox->isChecked();

    ui->backupIntervalSpinBox->setEnabled(enabled);
    ui->backupKeepCountSpinBox->setEnabled(enabled);

    const int intervalHour = settings.value(
        "backup/intervalHours", 24).toInt();

    const int keepCount =
        settings.value(
            "backup/keepCount", 10).toInt();

    ui->automaticBackupCheckBox->setChecked(automaticBackupEnabled);

    ui->backupIntervalSpinBox->setValue(intervalHour);

    ui->backupKeepCountSpinBox->setValue(keepCount);
}

void SettingsWidget::saveSettings()
{
    QSettings settings;

    settings.setValue(
        "backup/automaticEnabled",
        ui->automaticBackupCheckBox->isChecked());

    settings.setValue(
        "backup/intervalhours",
        ui->backupIntervalSpinBox->value());

    settings.setValue(
        "backup/keepCount",
        ui->backupKeepCountSpinBox->value());
}


