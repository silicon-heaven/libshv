#include "ui_dataviewfilterselector.h"

#include <shv/visu/logview/dataviewfilterselector.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/channelfilterdialog.h>

#include <shv/core/log.h>

namespace tl = shv::visu::timeline;

namespace shv::visu::logview {

DataViewFilterSelector::DataViewFilterSelector(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DataViewFilterSelector)
{
	ui->setupUi(this);

	connect(ui->pbShowChannelFilterDialog, &QToolButton::clicked, this, &DataViewFilterSelector::onShowChannelFilterClicked);
	connect(ui->pbShowRawData, &QToolButton::clicked, this, &DataViewFilterSelector::onShowRawDataClicked);
}

DataViewFilterSelector::~DataViewFilterSelector()
{
	delete ui;
}

void DataViewFilterSelector::init(const QString &site_path, timeline::Graph *graph)
{
	if (m_graph != nullptr) {
		shvWarning() << "Dialog is allready initialized.";
		return;
	}

	m_graph = graph;
	m_sitePath = site_path;

	connect(graph, &timeline::Graph::channelFilterChanged, this, [this](){
		ui->pbShowChannelFilterDialog->setIcon(m_graph->channelFilter()? QIcon(QStringLiteral(":/shv/visu/images/filter.svg")): QIcon(QStringLiteral(":/shv/visu/images/filter-off.svg")));
	});

	ui->pbShowRawData->setIcon(m_graph->style().isRawDataVisible()? QIcon(QStringLiteral(":/shv/visu/images/raw.svg")): QIcon(QStringLiteral(":/shv/visu/images/raw-off.svg")));
}

void DataViewFilterSelector::setPredefinedViews(const QVector<timeline::Graph::VisualSettings> &views)
{
	m_predefinedViews = views;
}

void DataViewFilterSelector::applyPredefinedView(const QString &name)
{
	for (const auto &v: m_predefinedViews) {
		if (v.name == name) {
			m_graph->setVisualSettingsAndChannelFilter(v);
			return;
		}
	}
}

void DataViewFilterSelector::onShowChannelFilterClicked()
{
	auto *channel_filter_dialog = new tl::ChannelFilterDialog(this, m_sitePath, m_graph, m_predefinedViews);

	if (channel_filter_dialog->exec() == QDialog::Accepted) {
		m_graph->setChannelFilter(channel_filter_dialog->channelFilter());
	}

	channel_filter_dialog->deleteLater();
}

void DataViewFilterSelector::onShowRawDataClicked()
{
	tl::Graph::Style graph_style = m_graph->style();
	graph_style.setRawDataVisible(!m_graph->style().isRawDataVisible());
	m_graph->setStyle(graph_style);

	ui->pbShowRawData->setIcon(m_graph->style().isRawDataVisible()? QIcon(QStringLiteral(":/shv/visu/images/raw.svg")): QIcon(QStringLiteral(":/shv/visu/images/raw-off.svg")));
}

}
