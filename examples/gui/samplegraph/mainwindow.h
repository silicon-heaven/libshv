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
	~MainWindow();
private slots:
	void on_actionGenerate_sample_data_triggered();

private:
	void generateSampleData();
private:
	Ui::MainWindow *ui;
	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
};
