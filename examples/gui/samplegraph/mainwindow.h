#pragma once

#include <QMainWindow>

namespace shv::visu::logview { class LogModel; class LogSortFilterProxyModel;}
namespace shv::visu::timeline { class GraphModel; class GraphWidget; class Graph; class ChannelFilterDialog;}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private:
	void onGenerateSamplesClicked();

	Ui::MainWindow *ui;
};
