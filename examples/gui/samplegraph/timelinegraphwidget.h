#pragma once

#include <QWidget>

namespace shv::visu::logview { class LogModel; class LogSortFilterProxyModel;}
namespace shv::visu::timeline { class GraphModel; class GraphWidget; class Graph; class ChannelFilterDialog;}

QT_BEGIN_NAMESPACE
namespace Ui { class TimelineGraphWidget; }
QT_END_NAMESPACE

class TimelineGraphWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TimelineGraphWidget(QWidget *parent = nullptr);
	~TimelineGraphWidget() override;

	void generateSampleData(int count);

private:
	Ui::TimelineGraphWidget *ui;

	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
};
