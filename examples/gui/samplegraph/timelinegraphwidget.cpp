#include "timelinegraphwidget.h"
#include "ui_timelinegraphwidget.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/rpc.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>

#include <QFile>
#include <QMessageBox>

#include <random>

using namespace std;
using namespace shv::chainpack;
namespace tl = shv::visu::timeline;

TimelineGraphWidget::TimelineGraphWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::TimelineGraphWidget)
{
	ui->setupUi(this);

	m_graphModel = new tl::GraphModel(this);
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

TimelineGraphWidget::~TimelineGraphWidget()
{
	delete ui;
}

void TimelineGraphWidget::generateSampleData(int count)
{
	int sample_cnt = count;
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
	for (int n = 0; n < sample_cnt; ++n)
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
		if (step > 0) {
			for(size_t j=step; j<times.size() - 3; j+=step) {
				m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 00, val_distrib(gen)});
				m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 10, val_distrib(gen)});
				m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 20, val_distrib(gen)});
				m_graphModel->appendValue(Channel::SparseData, tl::Sample{times[j] + 30, val_distrib(gen)});
			}
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

void TimelineGraphWidget::loadCsvFile(const QString &file_name)
{
	m_graphModel->clear();

	QFile file(file_name);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		shvError() << "Failed to open CSV file:" << file_name;
		QMessageBox::critical(this, "Error", "Failed to open CSV file: " + file_name);
		return;
	}

	auto read_line = [](QTextStream &in) {
		while (!in.atEnd()) {
			auto line = in.readLine();
			if (!line.isEmpty() && !line.startsWith('#')) {
				return line;
			}
		}
		return QString();
	};

	auto split_line = [](const QString &line) {
		auto fields = line.split(',');
		return fields;
	};

	QTextStream in(&file);

	static const std::array colors = {
		Qt::magenta,
		Qt::cyan,
		Qt::yellow,
		Qt::green,
		Qt::red,
		Qt::blue
	};

	QStringList columns;
	QMap<QString, shv::visu::timeline::GraphChannel::Style> styles;
	shv::visu::timeline::GraphChannel::Style default_style;
	default_style.setInterpolation(tl::GraphChannel::Style::Interpolation::Stepped);
	default_style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
	while (!in.atEnd()) {
		auto line = in.readLine();
		if (line.isEmpty()) {
			continue;
		}
		if (line.startsWith('#')) {
			line = line.mid(1).trimmed();
			static auto CHANNEL_STYLE = QStringLiteral("style:");
			if (line.startsWith(CHANNEL_STYLE)) {
				line = line.mid(CHANNEL_STYLE.size()).trimmed();
				std::string err;
				auto rv = shv::chainpack::RpcValue::fromCpon(line.toStdString(), &err);
				if (!err.empty()) {
					shvError() << "Failed to parse channel style from line:" << line << "error:" << err;
					continue;
				}
				const auto &m = rv.asMap();
				for (const auto &[chname, chstyle] : m) {
					auto &style = styles[QString::fromStdString(chname)];
					style = default_style;
					for (const auto &[key, val] : chstyle.asMap()) {
						if (key == "color") {
							style.setColor(QColor(val.to<QString>()));
							continue;
						}
						if (key == "interpolation") {
							auto ip = tl::GraphChannel::Style::Interpolation::None;
							if (val == "line") {
								ip = tl::GraphChannel::Style::Interpolation::Line;
							}
							else if (val == "stepped") {
								ip = tl::GraphChannel::Style::Interpolation::Stepped;
							}
							style.setInterpolation(ip);
							continue;
						}
					}
				}
			}
			continue;
		}
		auto csv_header = split_line(line);
		for (auto i = 1; i < csv_header.size(); ++i) {
			auto column = csv_header[i].trimmed();
			columns << column;
			m_graphModel->appendChannel(column, {}, {});
		}
		break;
	}

	m_graphModel->beginAppendValues();
	qsizetype n = 0;
	while (!in.atEnd()) {
		++n;
		auto values = split_line(read_line(in));
		auto tss = values.value(0).trimmed();
		int64_t ts;
		if (tss.contains('T')) {
			// try to parse ISO datetime
			auto dt = RpcValue::DateTime::fromIsoString(tss.toStdString());
			ts = dt.msecsSinceEpoch();
			if (ts == 0) {
				shvError() << "Invalid timestamp:" << tss;
				continue;
			}
		}
		else {
			bool ok = false;
			ts = values.value(0).trimmed().toLongLong(&ok);
			if (!ok) {
				shvError() << "Invalid timestamp:" << tss;
				continue;
			}
		}
		for (auto i = 0; i < columns.size(); ++i) {
			if (auto valstr = values.value(i+1).trimmed(); !valstr.isEmpty()) {
				bool ok = false;
				double val = valstr.toDouble(&ok);
				if (ok) {
					m_graphModel->appendValue(i, tl::Sample{ts, val});
				} else {
					shvError() << "Invalid value:" << valstr << "in line:" << values.join(',');
				}
			}
		}
	}
	m_graphModel->endAppendValues();
	shvInfo() << n << "CSV lines read";

	m_graph->createChannelsFromModel(shv::visu::timeline::Graph::SortChannels::No);
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		shv::visu::timeline::GraphChannel *ch = m_graph->channelAt(i);
		auto style = styles.value(columns.value(i), default_style);
		if (!style.color_isset()) {
			style.setColor(QColor(colors[i % colors.size()]));
		}
		ch->setStyle(style);
	}
	ui->graphView->makeLayout();
}

