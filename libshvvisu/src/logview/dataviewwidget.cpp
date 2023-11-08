#include "dataviewwidget.h"
#include "ui_dataviewwidget.h"

#include "shv/visu/timeline/graph.h"
#include "shv/visu/timeline/channelfilterdialog.h"

#include "shv/core/log.h"

namespace tl = shv::visu::timeline;

namespace shv::visu::logview {

DataViewWidget::DataViewWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DataViewWidget)
{
	ui->setupUi(this);

	connect(ui->pbShowChannelFilterDialog, &QToolButton::clicked, this, &DataViewWidget::onShowChannelFilterClicked);
}

DataViewWidget::~DataViewWidget()
{
	delete ui;
}

void DataViewWidget::init(const QString &site_path, timeline::Graph *graph)
{
	if (m_graph != nullptr) {
		shvWarning() << "Dialog is allready initialized.";
		return;
	}

	m_graph = graph;
	m_sitePath = site_path;

	connect(graph, &timeline::Graph::channelFilterChanged, this, [this](){
		bool is_filter_enabled = (m_graph->channelFilter() != std::nullopt);
		ui->pbShowChannelFilterDialog->setIcon(is_filter_enabled? QIcon(QStringLiteral(":/shv/visu/images/filter.svg")): QIcon(QStringLiteral(":/shv/visu/images/filter-off.svg")));
	});
}

void DataViewWidget::onShowChannelFilterClicked()
{
	tl::ChannelFilterDialog *channel_filter_dialog = new tl::ChannelFilterDialog(this);
	channel_filter_dialog->init(m_sitePath, m_currentVisualSettingsId, m_graph);

	if (channel_filter_dialog->exec() == QDialog::Accepted) {
		m_graph->setChannelFilter(channel_filter_dialog->filter());
		m_currentVisualSettingsId = channel_filter_dialog->currentVisualSettingsId();
	}

	channel_filter_dialog->deleteLater();
}

}
