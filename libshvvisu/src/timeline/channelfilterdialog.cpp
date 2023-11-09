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
	ui->gbFilterSetings->setEnabled(ui->chbFileterEnabled->isChecked());

	connect(ui->chbFileterEnabled, &QCheckBox::stateChanged, this, &ChannelFilterDialog::onChbFilterEnabledClicked);
	connect(ui->tvChannelsFilter, &QTreeView::customContextMenuRequested, this, &ChannelFilterDialog::onCustomContextMenuRequested);
	connect(ui->leMatchingFilterText, &QLineEdit::textChanged, this, &ChannelFilterDialog::onLeMatchingFilterTextChanged);
	connect(ui->pbClearMatchingText, &QPushButton::clicked, this, &ChannelFilterDialog::onPbClearMatchingTextClicked);
	connect(ui->pbCheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbCheckItemsClicked);
	connect(ui->pbUncheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsClicked);
}

ChannelFilterDialog::~ChannelFilterDialog()
{
	delete ui;
}

void ChannelFilterDialog::init(const QString &site_path, Graph *graph)
{
	if (m_graph != nullptr) {
		shvWarning() << "Dialog is allready initialized.";
		return;
	}

	m_sitePath = site_path;
	m_graph = graph;
	m_channelsFilterModel->createNodes(graph->channelPaths());

	ui->chbFileterEnabled->setChecked(m_graph->isFilteringEnabled());

	setPermittedChannelsFromGraph();
	reloadDataViewsCombobox();

	std::optional<ChannelFilter> chf = m_graph->channelFilter();

	if ((chf != std::nullopt) && (!chf->name().isEmpty()))
		ui->cbDataView->setCurrentText(chf->name());
	else
		ui->cbDataView->setCurrentIndex(-1);

	updateContextMenuActionsAvailability();
	connect(ui->cbDataView, &QComboBox::currentIndexChanged, this, &ChannelFilterDialog::onDataViewChanged);
}

std::optional<ChannelFilter> ChannelFilterDialog::filter()
{
	if (ui->chbFileterEnabled->isChecked()) {
		return ChannelFilter(m_channelsFilterModel->permittedChannels(), ui->cbDataView->currentText());
	}

	return std::nullopt;
}

void ChannelFilterDialog::applyTextFilter()
{
	m_channelsFilterProxyModel->setFilterString(ui->leMatchingFilterText->text());

	if (m_channelsFilterProxyModel->rowCount() == 1) {
		ui->tvChannelsFilter->setCurrentIndex(m_channelsFilterProxyModel->index(0, 0));
		ui->tvChannelsFilter->expandRecursively(ui->tvChannelsFilter->currentIndex());
	}
}

void ChannelFilterDialog::reloadDataViewsCombobox()
{
	ui->cbDataView->clear();
	ui->cbDataView->addItems(m_graph->savedVisualSettingsNames(m_sitePath));
}

void ChannelFilterDialog::deleteView()
{
	m_graph->deleteVisualSettings(m_sitePath, ui->cbDataView->currentText());
	reloadDataViewsCombobox();
	ui->cbDataView->setCurrentIndex(ui->cbDataView->count() - 1);
}

void ChannelFilterDialog::exportView()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Input file name"), RECENT_SETTINGS_DIR, "*" + FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
	if (!file_name.isEmpty()) {
		if (!file_name.endsWith(FLATLINE_VIEW_SETTINGS_FILE_EXTENSION)) {
			file_name.append(FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
		}

		m_graph->setChannelFilter(filter());

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
			visual_settings.name = view_name;
			m_graph->setVisualSettingsAndChannelFilter(visual_settings);
			m_graph->saveVisualSettings(m_sitePath, view_name);
			reloadDataViewsCombobox();
			ui->cbDataView->setCurrentText(view_name);

			RECENT_SETTINGS_DIR = QFileInfo(file_name).path();
		}
	}
}

void ChannelFilterDialog::updateContextMenuActionsAvailability()
{
	m_saveViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
	m_deleteViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
	m_resetViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
}

void ChannelFilterDialog::saveView()
{
	if(ui->cbDataView->currentText().isEmpty()) {
		shvWarning() << "Failed to save empty filter name.";
		return;
	}

	m_graph->setChannelFilter(filter());
	m_graph->saveVisualSettings(m_sitePath, ui->cbDataView->currentText());
}

void ChannelFilterDialog::saveViewAs()
{
	QString view_name = QInputDialog::getText(this, tr("Save as"), tr("Input view name"));

	if (view_name.isEmpty()) {
		QMessageBox::warning(this, tr("Error"), tr("Failed to save view: name is empty."));
	}
	else {
		m_graph->setChannelFilter(filter());
		m_graph->saveVisualSettings(m_sitePath, view_name);
		reloadDataViewsCombobox();
		ui->cbDataView->setCurrentText(view_name);
	}
}

void ChannelFilterDialog::discardUserChanges()
{
	onDataViewChanged(ui->cbDataView->currentIndex());
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

void ChannelFilterDialog::onChbFilterEnabledClicked(int state)
{
	ui->gbFilterSetings->setEnabled(state == Qt::CheckState::Checked);
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

void ChannelFilterDialog::setPermittedChannelsFromGraph()
{
	std::optional<ChannelFilter> f = m_graph->channelFilter();
	m_channelsFilterModel->setPermittedChannels((f == std::nullopt) ? QSet<QString>{} : f->permittedPaths());
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

void ChannelFilterDialog::onDataViewChanged(int index)
{
	if (index >= 0) { //ignore event with index = -1, which is emmited from clear() method
		m_graph->loadVisualSettings(m_sitePath, ui->cbDataView->currentText());
		setPermittedChannelsFromGraph();
	}
	updateContextMenuActionsAvailability();
}

}
