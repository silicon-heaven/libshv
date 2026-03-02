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
	MainWindow(const QString &csv_file, QWidget *parent = nullptr);
	~MainWindow() override;

private:
	void onGenerateSamplesClicked();

	void openFile();
	void loadCsvFile(const QString &csv_file);
	void reloadFile();
private:
	Ui::MainWindow *ui;
	QString m_currentFile;
};
