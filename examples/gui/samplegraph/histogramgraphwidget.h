#pragma once

#include <QWidget>

namespace shv::visu::logview { class LogModel; class LogSortFilterProxyModel;}
namespace shv::visu::timeline { class GraphModel; class GraphWidget; class Graph; class ChannelFilterDialog;}

QT_BEGIN_NAMESPACE
namespace Ui { class HistogramGraphWidget;}
QT_END_NAMESPACE

class HistogramGraphWidget : public QWidget
{
	Q_OBJECT

public:
	explicit HistogramGraphWidget(QWidget *parent = nullptr);
	~HistogramGraphWidget();

	void generateSampleData(int count);

private:
	Ui::HistogramGraphWidget *ui;
	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;

};
