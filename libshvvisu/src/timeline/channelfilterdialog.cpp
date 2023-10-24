#include "channelfilterdialog.h"
#include "ui_channelfilterdialog.h"

#include "channelfiltermodel.h"
#include "channelfiltersortfilterproxymodel.h"

#include <shv/core/log.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

namespace shv::visu::timeline {

static const int INVALID_FILTER_INDEX = 0;
static QString RECENT_SETTINGS_DIR;
static const QString FLATLINE_VIEW_SETTINGS_FILE_TYPE = QStringLiteral("FlatlineViewSettings");
static const QString FLATLINE_VIEW_SETTINGS_FILE_EXTENSION = QStringLiteral(".fvs");

ChannelFilterDialog::ChannelFilterDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ChannelFilterDialog)
{
	ui->setupUi(this);

	m_channelsFilterModel = new ChannelFilterModel(this);

	m_channelsFilterProxyModel = new ChannelFilterSortFilterProxyModel(this);
	m_channelsFilterProxyModel->setSourceModel(m_channelsFilterModel);
	m_channelsFilterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	m_channelsFilterProxyModel->setRecursiveFilteringEnabled(true);

	ui->tvChannelsFilter->setModel(m_channelsFilterProxyModel);
	ui->tvChannelsFilter->header()->hide();
	ui->tvChannelsFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	QMenu *view_menu = new QMenu(this);
	m_resetViewAction = view_menu->addAction(tr("Discard changes"), this, &ChannelFilterDialog::discardUserChanges);
	m_saveViewAction = view_menu->addAction(tr("Save"), this, &ChannelFilterDialog::saveView);
	m_saveViewAsAction = view_menu->addAction(tr("Save as"), this, &ChannelFilterDialog::saveViewAs);
	m_deleteViewAction = view_menu->addAction(tr("Delete"), this, &ChannelFilterDialog::deleteView);
	m_exportViewAction = view_menu->addAction(tr("Export"), this, &ChannelFilterDialog::exportView);
	m_importViewAction = view_menu->addAction(tr("Import"), this, &ChannelFilterDialog::importView);
	ui->pbActions->setMenu(view_menu);

	connect(ui->tvChannelsFilter, &QTreeView::customContextMenuRequested, this, &ChannelFilterDialog::onCustomContextMenuRequested);
	connect(ui->leMatchingFilterText, &QLineEdit::textChanged, this, &ChannelFilterDialog::onLeMatchingFilterTextChanged);
	connect(ui->pbClearMatchingText, &QPushButton::clicked, this, &ChannelFilterDialog::onPbClearMatchingTextClicked);
	connect(ui->pbCheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbCheckItemsClicked);
	connect(ui->pbUncheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsClicked);
	connect(ui->cbDataView, &QComboBox::currentIndexChanged, this, &ChannelFilterDialog::onDataViewCurrentIndexChanged);
}

ChannelFilterDialog::~ChannelFilterDialog()
{
	delete ui;
}

void ChannelFilterDialog::init(const QString &site_path, Graph *graph, const QString &filter_name)
{
	disableOnDataViewChangedAction();
	m_sitePath = site_path;
	m_graph = graph;
	m_channelsFilterModel->createNodes(graph->channelPaths());

	applyPermittedChannelsFromGraph();
	reloadDataViewsComboboxAndSetlect(filter_name);
	enableOnDataViewChangedAction();
}

QSet<QString> ChannelFilterDialog::selectedChannels()
{
	return m_channelsFilterModel->selectedChannels();
}

QString ChannelFilterDialog::selectedFilterName()
{
	return ui->cbDataView->currentText();
}

void ChannelFilterDialog::applyTextFilter()
{
	m_channelsFilterProxyModel->setFilterString(ui->leMatchingFilterText->text());

	if (m_channelsFilterProxyModel->rowCount() == 1) {
		ui->tvChannelsFilter->setCurrentIndex(m_channelsFilterProxyModel->index(0, 0));
		ui->tvChannelsFilter->expandRecursively(ui->tvChannelsFilter->currentIndex());
	}
}

void ChannelFilterDialog::reloadDataViewsComboboxAndSetlect(const QString &text)
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

void ChannelFilterDialog::deleteView()
{
	m_graph->deleteVisualSettings(m_sitePath, ui->cbDataView->currentText());
	disableOnDataViewChangedAction();
	reloadDataViewsComboboxAndSetlect();
	enableOnDataViewChangedAction();
}

void ChannelFilterDialog::exportView()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Input file name"), RECENT_SETTINGS_DIR, "*" + FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
	if (!file_name.isEmpty()) {
		if (!file_name.endsWith(FLATLINE_VIEW_SETTINGS_FILE_EXTENSION)) {
			file_name.append(FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
		}

		QSettings settings(file_name, QSettings::Format::IniFormat);
		settings.setValue("fileType", FLATLINE_VIEW_SETTINGS_FILE_TYPE);
		settings.setValue("settings", m_graph->visualSettings().toJson());
		RECENT_SETTINGS_DIR = QFileInfo(file_name).path();
	}
}

void ChannelFilterDialog::importView()
{
	QString file_name = QFileDialog::getOpenFileName(this, tr("Input file name"), RECENT_SETTINGS_DIR, "*" + FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
	if (!file_name.isEmpty()) {
		QString view_name = QInputDialog::getText(this, tr("Import as"), tr("Input view name"));

		if (!view_name.isEmpty()) {
			QSettings settings_file(file_name, QSettings::Format::IniFormat);
			if (settings_file.value("fileType").toString() != FLATLINE_VIEW_SETTINGS_FILE_TYPE) {
				QMessageBox::warning(this, tr("Error"), tr("This file is not flatline view setting file"));
				return;
			}

			Graph::VisualSettings visual_settings = Graph::VisualSettings::fromJson(settings_file.value("settings").toString());
			QSet<QString> graph_channels = m_graph->channelPaths();

			for (int i = 0; i < visual_settings.channels.count(); ++i) {
				if (!graph_channels.contains(visual_settings.channels[i].shvPath)) {
					visual_settings.channels.removeAt(i--);
				}
			}

			m_graph->saveVisualSettings(m_sitePath, view_name);
			reloadDataViewsComboboxAndSetlect(view_name);

			RECENT_SETTINGS_DIR = QFileInfo(file_name).path();
		}
	}
}

void ChannelFilterDialog::updateContextMenuActionsAvailability()
{
	m_saveViewAction->setEnabled(ui->cbDataView->currentIndex() != INVALID_FILTER_INDEX);
	m_deleteViewAction->setEnabled(ui->cbDataView->currentIndex() != INVALID_FILTER_INDEX);
}

void ChannelFilterDialog::saveView()
{
	m_graph->setChannelFilter(selectedChannels());
	m_graph->saveVisualSettings(m_sitePath, ui->cbDataView->currentText());
}

void ChannelFilterDialog::saveViewAs()
{
	QString view_name = QInputDialog::getText(this, tr("Save as"), tr("Input view name"));
	if (!view_name.isEmpty()) {
		m_graph->setChannelFilter(selectedChannels());
		m_graph->saveVisualSettings(m_sitePath, view_name);
		reloadDataViewsComboboxAndSetlect(view_name);
	}
}

void ChannelFilterDialog::discardUserChanges()
{
	onDataViewCurrentIndexChanged(ui->cbDataView->currentIndex());
}

void ChannelFilterDialog::onCustomContextMenuRequested(QPoint pos)
{
	QModelIndex ix = ui->tvChannelsFilter->indexAt(pos);

	if (ix.isValid()) {
		QMenu menu(this);

		menu.addAction(tr("Expand"), this, [this, ix]() {
			ui->tvChannelsFilter->expandRecursively(ix);
		});
		menu.addAction(tr("Collapse"), this, [this, ix]() {
			ui->tvChannelsFilter->collapse(ix);
		});

		menu.addSeparator();

		menu.addAction(tr("Expand all nodes"), this, [this]() {
			ui->tvChannelsFilter->expandAll();
		});

		menu.exec(ui->tvChannelsFilter->mapToGlobal(pos));
	}
}

void ChannelFilterDialog::onPbCheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Checked);
}

void ChannelFilterDialog::onPbUncheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Unchecked);
}

void ChannelFilterDialog::onPbClearMatchingTextClicked()
{
	ui->leMatchingFilterText->setText(QString());
}

void ChannelFilterDialog::onLeMatchingFilterTextChanged(const QString &text)
{
	Q_UNUSED(text);
	applyTextFilter();
}

void ChannelFilterDialog::onChbFindRegexChanged(int state)
{
	Q_UNUSED(state);
	applyTextFilter();
}

void ChannelFilterDialog::applyPermittedChannelsFromGraph()
{
	shvInfo() << "setPermitted channels" << m_graph->channelFilter().isValid();
	m_channelsFilterModel->setSelectedChannels((m_graph->channelFilter().isValid()) ? m_graph->channelFilter().permittedPaths() : m_graph->channelPaths());
}

void ChannelFilterDialog::setVisibleItemsCheckState(Qt::CheckState state)
{
	for (int row = 0; row < m_channelsFilterProxyModel->rowCount(); row++) {
		setVisibleItemsCheckState_helper(m_channelsFilterProxyModel->index(row, 0), state);
	}

	m_channelsFilterModel->fixCheckBoxesIntegrity();
}

void ChannelFilterDialog::setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state)
{
	if (!mi.isValid()) {
		return;
	}

	m_channelsFilterModel->setItemCheckState(m_channelsFilterProxyModel->mapToSource(mi), state);

	for (int row = 0; row < m_channelsFilterProxyModel->rowCount(mi); row++) {
		setVisibleItemsCheckState_helper(m_channelsFilterProxyModel->index(row, 0, mi), state);
	}
}

void ChannelFilterDialog::onDataViewCurrentIndexChanged(int index)
{
	if (m_onDataViewChangedActionEnabled && (index >= 0)) {
		if (index == INVALID_FILTER_INDEX) {
			m_graph->reset();
			shvInfo() << "chf valid" << m_graph->channelFilter().isValid();
		}
		else{
			m_graph->loadVisualSettings(m_sitePath, ui->cbDataView->currentText());
		}

		shvInfo() << " load" << ui->cbDataView->currentText().toStdString() << index;
		applyPermittedChannelsFromGraph();
		updateContextMenuActionsAvailability();
	}
}

}
