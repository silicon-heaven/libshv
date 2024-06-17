#include "histogramgraphwidget.h"
#include "ui_histogramgraphwidget.h"

#include <shv/coreqt/log.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>

#include <random>

using namespace std;
using namespace shv::chainpack;
namespace tl = shv::visu::timeline;

HistogramGraphWidget::HistogramGraphWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::HistogramGraphWidget)
{
	ui->setupUi(this);

	m_graphModel = new tl::GraphModel(this);
	m_graphModel->setXAxisType(shv::visu::timeline::GraphModel::XAxisType::Histogram);
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	m_graph = new tl::Graph(this);
	tl::Graph::Style graph_style = m_graph->style();
	m_graph->setStyle(graph_style);
	tl::GraphChannel::Style channel_style = m_graph->defaultChannelStyle();
	m_graph->setDefaultChannelStyle(channel_style);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);
}

HistogramGraphWidget::~HistogramGraphWidget()
{
	delete ui;
}

void HistogramGraphWidget::generateSampleData(int count)
{
	enum Channel {Histogram = 0, CHANNEL_COUNT};

	m_graphModel->clear();
	m_graphModel->appendChannel("Histogram", {}, {});
	m_graphModel->beginAppendValues();

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> val_distrib(0, 20);

	for (int n = 0; n < count; n++) {
		if (n == 2) { // skip value to test missing data
			continue;
		}
		m_graphModel->appendValue(Channel::Histogram, shv::visu::timeline::Sample{n, val_distrib(gen)});
	}

	m_graphModel->endAppendValues();
	m_graph->createChannelsFromModel(shv::visu::timeline::Graph::SortChannels::No);

	shv::visu::timeline::GraphChannel *ch = m_graph->channelAt(Channel::Histogram);
	shv::visu::timeline::GraphChannel::Style style = ch->style();
	style.setColor(Qt::blue);
	ch->setStyle(style);

	ui->graphView->makeLayout();
}

