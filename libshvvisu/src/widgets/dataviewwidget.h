#pragma once

#include <QWidget>

namespace Ui {
class DataViewWidget;
}

class DataViewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DataViewWidget(QWidget *parent = nullptr);
	~DataViewWidget();

private:
	Ui::DataViewWidget *ui;
};
