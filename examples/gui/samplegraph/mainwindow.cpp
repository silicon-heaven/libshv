#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>

#include <random>

using namespace std;
using namespace shv::chainpack;
namespace tl = shv::visu::timeline;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->samplesCount, &QSpinBox::editingFinished, this, &MainWindow::on_actionGenerate_sample_data_triggered);

	ui->btGenerateSamples->setDefaultAction(ui->actionGenerate_sample_data);

	m_graphModel = new tl::GraphModel(this);
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	m_graph = new tl::Graph(this);
	tl::Graph::Style graph_style = m_graph->style();
	// graph_style.setYAxisVisible(false);
	m_graph->setStyle(graph_style);
	tl::GraphChannel::Style channel_style = m_graph->defaultChannelStyle();
	//channel_style.setColorGrid(QColor());
	m_graph->setDefaultChannelStyle(channel_style);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);
	generateSampleData();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::generateSampleData()
{
	int sample_cnt = ui->samplesCount->value();
	static constexpr int64_t YEAR = 1000LL * 60 * 60 * 24 * 365;
	int64_t min_time = RpcValue::DateTime::now().msecsSinceEpoch();
	int64_t max_time = min_time + YEAR;
	double min_val = -3;
	double max_val = 5;

	m_graphModel->clear();
	enum Channel {Stepped = 0, Line, Isolated, Discrete, WithMissinData, SparseData, CHANNEL_COUNT};
	//const auto CHANNEL_COUNT = Channel::Sparse + 1;
	auto channel_to_string = [](Channel ch) {
		switch(ch) {
		case Isolated: return "Isolated";
		case Stepped: return "Stepped";
		case Line: return "Line";
		case WithMissinData: return "WithMissinData";
		case SparseData: return "Sparse";
		case Discrete: return "Discrete";
		case CHANNEL_COUNT: return "CHANNEL_COUNT";
		break;
		}
		return "???";
	};
	for (int i = 0; i < CHANNEL_COUNT; ++i) {
		if (i == Channel::Discrete) {
			using shv::core::utils::ShvTypeDescr;
			ShvTypeDescr td(ShvTypeDescr::Type::Map, ShvTypeDescr::SampleType::Discrete);
			m_graphModel->appendChannel(channel_to_string(static_cast<Channel>(i)), {}, td);
		}
		else {
			m_graphModel->appendChannel(channel_to_string(static_cast<Channel>(i)), {}, {});
		}
	}

	m_graphModel->beginAppendValues();

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<int64_t> time_distrib(min_time, max_time);
	std::uniform_real_distribution<> val_distrib(min_val, max_val);

	vector<int64_t> times;
	for (int n=0; n<sample_cnt+2; ++n)
		times.push_back(time_distrib(gen));
	sort(times.begin(), times.end());

	m_graphModel->appendValue(Channel::Isolated, tl::Sample{times[0], val_distrib(gen)});
	for(size_t j=1; j<times.size()-1; ++j) {
		auto val = val_distrib(gen);
		for (int i = Channel::Stepped; i <= Channel::Isolated; i++) {
			if(j == 3) {
				// generate multiple same pixel values
				for(int k=0; k<5; k++)
					m_graphModel->appendValue(i, tl::Sample{times[j]+k, val_distrib(gen)});
			}
			else {
				m_graphModel->appendValue(i, tl::Sample{times[j], val});
			}
		}
	}
	m_graphModel->appendValue(Channel::Isolated, tl::Sample{times[times.size()-1], val_distrib(gen)});
	{
		vector<int64_t> discr_times;
		for (int n = 0; n < sample_cnt / 5; ++n)
			discr_times.push_back(time_distrib(gen));
		sort(discr_times.begin(), discr_times.end());
		for(auto discr_time : discr_times) {
			auto val = val_distrib(gen);
			QVariantMap map{{"name", "Discrete sample"}, {"value", val}};
			if (val > 0) {
				map["positive"] = true;
			}
			if (val > 5) {
				map["big"] = true;
			}
			m_graphModel->appendValue(Channel::Discrete, tl::Sample{discr_time, map});
		}
	}
	{
		// generate data with not available part in center third
		bool na_added = false;
		for(size_t j=1; j<times.size()-1; ++j) {
			if(j > times.size()/3 && j < times.size()*2/3) {
				if(!na_added) {
					na_added = true;
					m_graphModel->appendValue(Channel::WithMissinData, tl::Sample{times[j], QVariant()});
				}
			}
			else {
				m_graphModel->appendValue(Channel::WithMissinData, tl::Sample{times[j], val_distrib(gen)});
			}
		}
	}
	{
		//generate sparse data
		const auto step = times.size() / 5;
		for(size_t j=step; j<times.size() - 3; j+=step) {
			m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 00, val_distrib(gen)});
			m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 10, val_distrib(gen)});
			m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 20, val_distrib(gen)});
			m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 30, val_distrib(gen)});
		}
	}
	m_graphModel->endAppendValues();

	m_graph->createChannelsFromModel(shv::visu::timeline::Graph::SortChannels::No);
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		shv::visu::timeline::GraphChannel *ch = m_graph->channelAt(i);
		shv::visu::timeline::GraphChannel::Style style = ch->style();
		if(ch->shvPath() == "Line") {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::Line);
			style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
			style.setColor(Qt::yellow);
		}
		else if(ch->shvPath() == "Isolated") {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::None);
			style.setColor(Qt::magenta);
		}
		else if(ch->shvPath() == "Discrete") {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::None);
			// style.setHideDiscreteValuesInfo(false);
			style.setColor(Qt::green);
		}
		else {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::Stepped);
			style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
		}
		//style.setInterpolation(tl::GraphChannel::Style::Interpolation::None);
		ch->setStyle(style);
	}
	ui->graphView->makeLayout();
}


void MainWindow::on_actionGenerate_sample_data_triggered()
{
	generateSampleData();
}
