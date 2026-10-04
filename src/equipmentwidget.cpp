


#include "equipmentwidget.h"
#include "ui_equipmentwidget.h"
#include "meterswidget.h"
#include "currenttransformerswidget.h"


EquipmentWidget::EquipmentWidget(
    Database &database, QWidget *parent) :
        QWidget(parent), ui(new Ui::EquipmentWidget), m_database(database)
{
    ui->setupUi(this);

    // Создаем вкладку приборов учета
    m_metersWidget = new MetersWidget(m_database, ui->metersTab);
    ui->metersTab->layout()->addWidget(m_metersWidget);

    // Создаем вкладку ТТ
    m_currentTransformerWidget = new CurrentTransformersWidget(
        m_database, ui->currentTransformersTab);
    ui->currentTransformersTab->layout()->addWidget(
        m_currentTransformerWidget);
}

EquipmentWidget::~EquipmentWidget()
{
    delete ui;
}

MetersWidget * EquipmentWidget::metersWidget() const
{
    return m_metersWidget;
}

CurrentTransformersWidget * EquipmentWidget::currentTransformerWidget() const
{
    return m_currentTransformerWidget;
}
