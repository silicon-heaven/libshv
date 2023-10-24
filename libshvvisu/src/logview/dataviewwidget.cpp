#include "dataviewwidget.h"
#include "ui_dataviewwidget.h"

#include "shv/visu/timeline/graph.h"
#include "shv/visu/timeline/channelfilterdialog.h"

#include "shv/core/log.h"

namespace tl = shv::visu::timeline;

namespace shv::visu::logview {

static const int INVALID_FILTER_INDEX = 0;

DataViewWidget::DataViewWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DataViewWidget)
{
	ui->setupUi(this);

	connect(ui->pbShowChannelFilterDialog, &QToolButton::clicked, this, &DataViewWidget::onShowChannelFilterClicked);
	connect(ui->cbDataViews, &QComboBox::currentIndexChanged, this, &DataViewWidget::onDataViewChanged);
}

DataViewWidget::~DataViewWidget()
{
	delete ui;
}

void DataViewWidget::init(const QString &site_path, timeline::Graph *graph)
{
	m_graph = graph;
	m_sitePath = site_path;

	disableOnDataViewChangedAction();
	reloadDataViewsCombobox();
	enableOnDataViewChangedAction();
}

void DataViewWidget::reloadDataViewsCombobox()
{
	ui->cbDataViews->clear();
	ui->cbDataViews->addItem(tr("All channels"));

	for (const QString &view_name : m_graph->savedVisualSettingsNames(m_sitePath)) {
		ui->cbDataViews->addItem(view_name);
	}
}

void DataViewWidget::onShowChannelFilterClicked()
{
	tl::ChannelFilterDialog *channel_filter_dialog = new tl::ChannelFilterDialog(this);
	channel_filter_dialog->init(m_sitePath, m_graph, ui->cbDataViews->currentText());

	if (channel_filter_dialog->exec() == QDialog::Accepted) {
		shv::visu::timeline::ChannelFilter filter(channel_filter_dialog->selectedChannels());
		m_graph->setChannelFilter(filter);
	}

	channel_filter_dialog->deleteLater();

	disableOnDataViewChangedAction();
	reloadDataViewsCombobox();
	ui->cbDataViews->setCurrentText(channel_filter_dialog->selectedFilterName());
	enableOnDataViewChangedAction();
}

void DataViewWidget::onDataViewChanged(int index)
{
	if (m_onDataViewChangedActionEnabled) {
		if (index > INVALID_FILTER_INDEX)
			m_graph->loadVisualSettings(m_sitePath, ui->cbDataViews->currentText());
		else {
			m_graph->setChannelFilter(timeline::ChannelFilter());
		}
	}
}

}
