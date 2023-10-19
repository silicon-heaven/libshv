#include "dataviewwidget.h"
#include "ui_dataviewwidget.h"

DataViewWidget::DataViewWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DataViewWidget)
{
	ui->setupUi(this);
}

DataViewWidget::~DataViewWidget()
{
	delete ui;
}
