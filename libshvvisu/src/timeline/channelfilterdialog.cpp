#include "ui_channelfilterdialog.h"
#include "channelfiltersortfilterproxymodel.h"

#include <shv/visu/timeline/channelfilterdialog.h>
#include <shv/visu/timeline/channelfiltermodel.h>

#include <shv/core/log.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

namespace shv::visu::timeline {

static const QString FLATLINE_VIEW_SETTINGS_FILE_EXTENSION = QStringLiteral(".fvs");

ChannelFilterDialog::ChannelFilterDialog(QWidget *parent, const QString &site_path, Graph *graph) :
	QDialog(parent),
	ui(new Ui::ChannelFilterDialog)
{
	ui->setupUi(this);

	m_sitePath = site_path;
	m_graph = graph;

	m_channelsFilterModel = new ChannelFilterModel(this);

	m_channelsFilterProxyModel = new ChannelFilterSortFilterProxyModel(this);
	m_channelsFilterProxyModel->setSourceModel(m_channelsFilterModel);
	m_channelsFilterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	m_channelsFilterProxyModel->setRecursiveFilteringEnabled(true);

	ui->tvChannelsFilter->setModel(m_channelsFilterProxyModel);
	ui->tvChannelsFilter->header()->hide();
	ui->tvChannelsFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

	auto *view_menu = new QMenu(this);
	m_resetViewAction = view_menu->addAction(tr("Discard changes"), this, &ChannelFilterDialog::discardUserChanges);
	m_saveViewAction = view_menu->addAction(tr("Save"), this, &ChannelFilterDialog::saveDataView);
	m_saveViewAsAction = view_menu->addAction(tr("Save as"), this, &ChannelFilterDialog::saveDataViewAs);
	m_deleteViewAction = view_menu->addAction(tr("Delete"), this, &ChannelFilterDialog::deleteDataView);
	m_exportViewAction = view_menu->addAction(tr("Export"), this, &ChannelFilterDialog::exportDataView);
	m_importViewAction = view_menu->addAction(tr("Import"), this, &ChannelFilterDialog::importDataView);
	ui->pbActions->setMenu(view_menu);
	ui->gbFilterSettings->setChecked(true);

	m_channelsFilterModel->createNodes(graph->channelPaths());

	loadChannelFilterFomGraph();
	reloadDataViewsCombobox();

	std::optional<ChannelFilter> chf = m_graph->channelFilter();

	if (chf && !chf.value().name().isEmpty())
		ui->cbDataView->setCurrentText(chf.value().name());
	else
		ui->cbDataView->setCurrentIndex(-1);

	refreshActions();

	connect(ui->cbDataView, qOverload<int>(&QComboBox::currentIndexChanged), this, &ChannelFilterDialog::onDataViewComboboxChanged);
	connect(ui->tvChannelsFilter, &QTreeView::customContextMenuRequested, this, &ChannelFilterDialog::onCustomContextMenuRequested);
	connect(ui->leMatchingFilterText, &QLineEdit::textChanged, this, &ChannelFilterDialog::onLeMatchingFilterTextChanged);
	connect(ui->pbClearMatchingText, &QPushButton::clicked, this, &ChannelFilterDialog::onPbClearMatchingTextClicked);
	connect(ui->pbCheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbCheckItemsClicked);
	connect(ui->pbUncheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsClicked);
	connect(ui->pbUncheckItemsWithoutChanges, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsWithoutChangesClicked);
}

ChannelFilterDialog::~ChannelFilterDialog()
{
	delete ui;
}

std::optional<ChannelFilter> ChannelFilterDialog::channelFilter()
{
	if (ui->gbFilterSettings->isChecked()) {
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

void ChannelFilterDialog::deleteDataView()
{
	m_graph->deleteVisualSettings(m_sitePath, ui->cbDataView->currentText());
	reloadDataViewsCombobox();
	ui->cbDataView->setCurrentIndex(ui->cbDataView->count() - 1);
}

void ChannelFilterDialog::exportDataView()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Input file name"), m_recentSettingsDir, "*" + FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
	if (!file_name.isEmpty()) {
		QFile settings_file(file_name);
		if (!settings_file.open(QFile::WriteOnly | QFile::Truncate)) {
			QMessageBox::warning(this, tr("Error"), tr("Cannot open graph setting file."));
			return;
		}

		m_graph->setChannelFilter(channelFilter());
		settings_file.write(m_graph->visualSettings().toJson().toUtf8());
		m_recentSettingsDir = QFileInfo(file_name).path();
	}
}

void ChannelFilterDialog::importDataView()
{
	QString file_name = QFileDialog::getOpenFileName(this, tr("Input file name"), m_recentSettingsDir, "*" + FLATLINE_VIEW_SETTINGS_FILE_EXTENSION);
	if (!file_name.isEmpty()) {
		QString view_name = QInputDialog::getText(this, tr("Import as"), tr("Input view name"));

		if (!view_name.isEmpty()) {
			QFile settings_file(file_name);
			if (!settings_file.open(QFile::ReadOnly)) {
				QMessageBox::warning(this, tr("Error"), tr("Cannot open graph setting file."));
				return;
			}

			Graph::VisualSettings visual_settings = Graph::VisualSettings::fromJson(QString::fromUtf8(settings_file.readAll()));
			visual_settings.name = view_name;
			m_graph->setVisualSettingsAndChannelFilter(visual_settings);
			m_graph->saveVisualSettings(m_sitePath, view_name);
			reloadDataViewsCombobox();
			ui->cbDataView->setCurrentText(view_name);

			m_recentSettingsDir = QFileInfo(file_name).path();
		}
	}
}

void ChannelFilterDialog::refreshActions()
{
	m_saveViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
	m_deleteViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
	m_resetViewAction->setEnabled(!ui->cbDataView->currentText().isEmpty());
}

void ChannelFilterDialog::saveDataView()
{
	if(ui->cbDataView->currentText().isEmpty()) {
		shvWarning() << "Failed to save empty filter name.";
		return;
	}

	m_graph->setChannelFilter(channelFilter());
	m_graph->saveVisualSettings(m_sitePath, ui->cbDataView->currentText());
}

void ChannelFilterDialog::saveDataViewAs()
{
	QString view_name = QInputDialog::getText(this, tr("Save as"), tr("Input view name"));

	if (view_name.isEmpty()) {
		QMessageBox::warning(this, tr("Error"), tr("Failed to save view: name is empty."));
	}
	else {
		m_graph->setChannelFilter(channelFilter());
		m_graph->saveVisualSettings(m_sitePath, view_name);
		reloadDataViewsCombobox();
		ui->cbDataView->setCurrentText(view_name);
	}
}

void ChannelFilterDialog::discardUserChanges()
{
	onDataViewComboboxChanged(ui->cbDataView->currentIndex());
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

void ChannelFilterDialog::onPbUncheckItemsWithoutChangesClicked()
{
	QSet<QString> channels = m_channelsFilterModel->permittedChannels().subtract(m_graph->flatChannels());
	m_channelsFilterModel->setPermittedChannels(channels);
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

void ChannelFilterDialog::loadChannelFilterFomGraph()
{
	std::optional<ChannelFilter> f = m_graph->channelFilter();
	m_channelsFilterModel->setPermittedChannels((f) ? f.value().permittedPaths() : QSet<QString>{});
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

void ChannelFilterDialog::onDataViewComboboxChanged(int index)
{
	if (index >= 0) { //ignore event with index = -1, which is emmited from clear() method
		m_graph->loadVisualSettings(m_sitePath, ui->cbDataView->currentText());
		loadChannelFilterFomGraph();
	}
	refreshActions();
}

}
