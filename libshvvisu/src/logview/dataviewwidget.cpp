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
	connect(ui->cbDataView, &QComboBox::currentIndexChanged, this, &DataViewWidget::onDataViewChanged);
}

DataViewWidget::~DataViewWidget()
{
	delete ui;
}

void DataViewWidget::init(const QString &site_path, timeline::Graph *graph)
{
	m_graph = graph;
	m_sitePath = site_path;

	reloadDataViewsComboboxAndSetlectItem();
}

void DataViewWidget::reloadDataViewsComboboxAndSetlectItem(const QString &text)
{
	ui->cbDataView->clear();
	ui->cbDataView->addItem(tr("All channels"));
	ui->cbDataView->addItems(m_graph->savedVisualSettingsNames(m_sitePath));

	if (!text.isEmpty()) {
		ui->cbDataView->setCurrentText(text);
	}
	else {
		ui->cbDataView->setCurrentIndex(INVALID_FILTER_INDEX);
	}
}

void DataViewWidget::onShowChannelFilterClicked()
{
	tl::ChannelFilterDialog *channel_filter_dialog = new tl::ChannelFilterDialog(this);
	channel_filter_dialog->init(m_sitePath, m_graph, ui->cbDataView->currentText());

	if (channel_filter_dialog->exec() == QDialog::Accepted) {
		shv::visu::timeline::ChannelFilter filter(channel_filter_dialog->selectedChannels());
		m_graph->setChannelFilter(filter);
	}

	channel_filter_dialog->deleteLater();

	disableOnDataViewChangedAction();
	reloadDataViewsComboboxAndSetlectItem(channel_filter_dialog->selectedFilterName());
	enableOnDataViewChangedAction();
}

void DataViewWidget::onDataViewChanged(int index)
{
	if (m_onDataViewChangedActionEnabled && (index >= 0)) { //ignore event for index = -1, currentIndexChanged(-1) is emmited from clear() method
		if (index == INVALID_FILTER_INDEX) {
			m_graph->resetVisualSettingsAndChannelFilter();
		}
		else{
			m_graph->loadVisualSettings(m_sitePath, ui->cbDataView->currentText());
		}
	}
}

}
