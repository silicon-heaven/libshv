#include "dlgloginspector.h"
#include "ui_dlgloginspector.h"

#include "logmodel.h"
#include "logsortfilterproxymodel.h"

#include "../timeline/graphmodel.h"
#include "../timeline/graphwidget.h"
#include "../timeline/graph.h"
#include "../timeline/channelfilterdialog.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/exception.h>
#include <shv/core/string.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/utils.h>

#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTimeZone>
#include <QMessageBox>

namespace cp = shv::chainpack;
namespace tl = shv::visu::timeline;

namespace shv {
namespace visu {
namespace logview {

enum { TabGraph = 0, TabData, TabInfo };

static const int VIEW_SELECTOR_NO_VIEW_INDEX = 0;

DlgLogInspector::DlgLogInspector(const QString &shv_path, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgLogInspector)
{
	ui->setupUi(this);
	setShvPath(shv_path);
	{
		QMenu *m = new QMenu(this);
		{
			QAction *a = new QAction(tr("ChainPack"), m);
			connect(a, &QAction::triggered, [this]() {
				auto log = m_logModel->log();
				std::string data = log.toChainPack();
				saveData(data, ".chpk");
			});
			m->addAction(a);
		}
		{
			QAction *a = new QAction(tr("Cpon"), m);
			connect(a, &QAction::triggered, [this]() {
				auto log = m_logModel->log();
				std::string data = log.toCpon("\t");
				saveData(data, ".cpon");
			});
			m->addAction(a);
		}
		{
			QAction *a = new QAction(tr("CSV"), m);
			connect(a, &QAction::triggered, [this]() {
				std::string data;
				for(int row=0; row<m_logModel->rowCount(); row++) {
					std::string row_data;
					for(int col=0; col<m_logModel->columnCount(); col++) {
						QModelIndex ix = m_logModel->index(row, col);
						if(col > 0)
							row_data += '\t';
						row_data += ix.data(Qt::DisplayRole).toString().toStdString();
					}
					if(row > 0)
						data += '\n';
					data += row_data;
				}
				saveData(data, ".csv");
			});
			m->addAction(a);
		}
		ui->btSaveData->setMenu(m);
	}

	ui->lblInfo->hide();
	ui->btMoreOptions->setChecked(false);

	QDateTime dt2 = QDateTime::currentDateTime();
	QDateTime dt1 = dt2.addDays(-1);
	dt2 = dt2.addSecs(60 * 60);
	ui->edSince->setDateTime(dt1);
	ui->edUntil->setDateTime(dt2);
	connect(ui->btClearSince, &QPushButton::clicked, [this]() {
		ui->edSince->setDateTime(ui->edSince->minimumDateTime());
	});
	connect(ui->btClearUntil, &QPushButton::clicked, [this]() {
		ui->edUntil->setDateTime(ui->edUntil->minimumDateTime());
	});

	connect(ui->btTabGraph, &QAbstractButton::toggled, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabGraph);
	});
	connect(ui->btTabData, &QAbstractButton::toggled, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabData);
	});
	connect(ui->btTabInfo, &QAbstractButton::toggled, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabInfo);
	});

	m_logModel = new LogModel(this);
	m_logSortFilterProxy = new shv::visu::logview::LogSortFilterProxyModel(this);
	m_logSortFilterProxy->setShvPathColumn(LogModel::ColPath);
	m_logSortFilterProxy->setValueColumn(LogModel::ColValue);
	m_logSortFilterProxy->setSourceModel(m_logModel);
	ui->tblData->setModel(m_logSortFilterProxy);
	ui->tblData->setSortingEnabled(false);
	ui->tblData->verticalHeader()->setDefaultSectionSize((int)(fontMetrics().lineSpacing() * 1.3));

	ui->tblData->setContextMenuPolicy(Qt::ActionsContextMenu);
	{
		QAction *copy = new QAction(tr("&Copy"));
		copy->setShortcut(QKeySequence::Copy);
		ui->tblData->addAction(copy);
		connect(copy, &QAction::triggered, [this]() {
			auto table_view = ui->tblData;
			auto *m = table_view->model();
			if(!m)
				return;
			int n = 0;
			QString rows;
			QItemSelection sel = table_view->selectionModel()->selection();
			foreach(const QItemSelectionRange &sel1, sel) {
				if(sel1.isValid()) {
					for(int row=sel1.top(); row<=sel1.bottom(); row++) {
						QString cells;
						for(int col=sel1.left(); col<=sel1.right(); col++) {
							QModelIndex ix = m->index(row, col);
							QString s;
							s = ix.data(Qt::DisplayRole).toString();
							static constexpr bool replace_escapes = true;
							if(replace_escapes) {
								s.replace('\r', QStringLiteral("\\r"));
								s.replace('\n', QStringLiteral("\\n"));
								s.replace('\t', QStringLiteral("\\t"));
							}
							if(col > sel1.left())
								cells += '\t';
							cells += s;
						}
						if(n++ > 0)
							rows += '\n';
						rows += cells;
					}
				}
			}
			if(!rows.isEmpty()) {
				//qfInfo() << "\tSetting clipboard:" << rows;
				QClipboard *clipboard = QApplication::clipboard();
				clipboard->setText(rows);
			}
		});
	}

	m_graphModel = new tl::GraphModel(this);
	//connect(m_dataModel, &tl::GraphModel::xRangeChanged, this, &MainWindow::onGraphXRangeChanged);
	//ui->graphView->viewport()->show();
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//shvInfo() << qobject_cast<QWidget*>(ui->graphView->widget());
	m_graph = new tl::Graph(this);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);

	m_channelFilterDialog = new shv::visu::timeline::ChannelFilterDialog(this);

	connect(ui->cbxTimeZone, &QComboBox::currentTextChanged, [this](const QString &) {
		auto tz = ui->cbxTimeZone->currentTimeZone();
		setTimeZone(tz);
	});
	setTimeZone(ui->cbxTimeZone->currentTimeZone());

	connect(ui->btLoad, &QPushButton::clicked, this, &DlgLogInspector::downloadLog);

	connect(m_graph, &shv::visu::timeline::Graph::channelFilterChanged, this, &DlgLogInspector::onGraphChannelFilterChanged);
	connect(ui->pbChannelsFilter, &QPushButton::clicked, this, &DlgLogInspector::onChannelsFilterClicked);

	connect(ui->btResizeColumnsToFitWidth, &QAbstractButton::clicked, [this]() {
		ui->tblData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	});

	loadSettings();

	initVisualSettingSelector(shv_path);
	ui->cbViews->setCurrentIndex(VIEW_SELECTOR_NO_VIEW_INDEX);

	connect(ui->cbViews, QOverload<int>::of(&QComboBox::activated), this, &DlgLogInspector::onViewSelected);

	connect(ui->pbDeleteView, &QPushButton::clicked, this, &DlgLogInspector::onDeleteViewClicked);
	connect(ui->pbSaveView, &QPushButton::clicked, this, &DlgLogInspector::onSaveViewClicked);
	onViewSelected(VIEW_SELECTOR_NO_VIEW_INDEX);
}

DlgLogInspector::~DlgLogInspector()
{
	saveSettings();
	delete ui;
}

void DlgLogInspector::loadSettings()
{
	QSettings settings;
	QByteArray ba = settings.value("ui/DlgLogInspector/geometry").toByteArray();
	restoreGeometry(ba);
}

void DlgLogInspector::initVisualSettingSelector(const QString &shv_path)
{
	ui->cbViews->clear();

	ui->cbViews->addItem(tr("Initial view"));
	for (const QString &view_name : m_graph->savedVisualSettingsNames(shv_path)) {
		ui->cbViews->addItem(view_name);
	}
}

void DlgLogInspector::onSaveViewClicked()
{
	QString current_name = ui->cbViews->currentText();
	if (current_name.isEmpty() || current_name == ui->cbViews->itemText(0)) {
		return;
	}
	if (ui->cbViews->findText(current_name) == -1) {
		int index = ui->cbViews->count();
		ui->cbViews->addItem(current_name);
		ui->cbViews->setCurrentIndex(index);
	}
	m_graph->saveVisualSettings(shvPath(), current_name);
}

void DlgLogInspector::onDeleteViewClicked()
{
	int index = ui->cbViews->currentIndex();
	const QString &current_name = ui->cbViews->currentText();
	if (ui->cbViews->findText(current_name) == -1) {
		ui->cbViews->setEditText(ui->cbViews->itemText(index));
		return;
	}

	m_graph->deleteVisualSettings(shvPath(), current_name);
	ui->cbViews->removeItem(index);
	ui->cbViews->setCurrentIndex(VIEW_SELECTOR_NO_VIEW_INDEX);
	onViewSelected(VIEW_SELECTOR_NO_VIEW_INDEX);
}

void DlgLogInspector::onViewSelected(int index)
{
	if (index == VIEW_SELECTOR_NO_VIEW_INDEX) {
		m_graph->reset();
	}
	else {
		setView(ui->cbViews->currentText());
	}
}

void DlgLogInspector::setView(const QString &name)
{
	m_graph->loadVisualSettings(shvPath(), name);
}

void DlgLogInspector::saveSettings()
{
	QSettings settings;
	QByteArray ba = saveGeometry();
	settings.setValue("ui/DlgLogInspector/geometry", ba);
}

shv::iotqt::rpc::ClientConnection *DlgLogInspector::rpcConnection()
{
	if(!m_rpcConnection)
		SHV_EXCEPTION("RPC connection is NULL");
	return m_rpcConnection;
}

void DlgLogInspector::setRpcConnection(iotqt::rpc::ClientConnection *c)
{
	m_rpcConnection = c;
}

QString DlgLogInspector::shvPath() const
{
	return ui->edShvPath->text();
}

void DlgLogInspector::setShvPath(const QString &s)
{
	ui->edShvPath->setText(s);
}

shv::chainpack::RpcValue DlgLogInspector::getLogParams()
{
	shv::core::utils::ShvGetLogParams params;
	auto get_dt = [this](QDateTimeEdit *ed) {
		QDateTime dt = ed->dateTime();
		if(dt == ed->minimumDateTime())
			return  cp::RpcValue();
		dt = QDateTime(dt.date(), dt.time(), m_timeZone);
		return cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(dt.toMSecsSinceEpoch()));
	};
	params.since = get_dt(ui->edSince);
	params.until = get_dt(ui->edUntil);
	params.pathPattern = ui->edPathPattern->text().trimmed().toStdString();
	if(ui->chkIsPathPatternRexex)
		params.pathPatternType = shv::core::utils::ShvGetLogParams::PatternType::RegEx;
	if(ui->edMaxRecordCount->value() > ui->edMaxRecordCount->minimum())
		params.recordCountLimit = ui->edMaxRecordCount->value();
	params.withPathsDict = ui->chkPathsDict->isChecked();
	params.withSnapshot = ui->chkWithSnapshot->isChecked();
	params.withTypeInfo = ui->chkWithTypeInfo->isChecked();
	/*
	unsigned header_opts = 0;
	if(ui->chkBasicInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvGetLogParams::HeaderOptions::BasicInfo);
	if(ui->chkFieldInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvGetLogParams::HeaderOptions::FieldInfo);
	if(ui->chkTypeInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvGetLogParams::HeaderOptions::TypeInfo);
	if(ui->chkPathsDict->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvGetLogParams::HeaderOptions::PathsDict);
	params.headerOptions = header_opts;
	*/
	shvDebug() << params.toRpcValue().toCpon();
	return params.toRpcValue();
}

void DlgLogInspector::downloadLog()
{
	std::string shv_path = shvPath().toStdString();
	if(ui->chkUseHistoryProvider->isChecked()) {
		if(shv::core::String::startsWith(shv_path, "shv/"))
			shv_path = "history" + shv_path.substr(3);
	}
	showInfo(QString::fromStdString("Downloading data from " + shv_path));
	cp::RpcValue params = getLogParams();
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	cb->setTimeout(ui->edTimeout->value() * 1000);
	cb->start(this, [this, shv_path](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError()) {
				showInfo(QString::fromStdString("GET " + shv_path + " RPC request error: " + resp.error().toString()), true);
			}
			else {
				showInfo();
				this->parseLog(resp.result());
			}
		}
		else {
			showInfo(QString::fromStdString("GET " + shv_path + " RPC request timeout"), true);
		}
	});
	conn->callShvMethod(rq_id, shv_path, cp::Rpc::METH_GET_LOG, params);
}

void DlgLogInspector::parseLog(shv::chainpack::RpcValue log)
{
	{
		std::string str = log.metaData().toString("\t");
		ui->edInfo->setPlainText(QString::fromStdString(str));
	}
	{
		m_logModel->setLog(log);
		ui->tblData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	}
	try {
		struct ShortTime {
			int64_t msec_sum = 0;
			uint16_t last_msec = 0;

			int64_t addShortTime(uint16_t msec)
			{
				msec_sum += static_cast<uint16_t>(msec - last_msec);
				last_msec = msec;
				return msec_sum;
			}
		};
		QMap<std::string, ShortTime> short_times;
		shv::core::utils::ShvLogRpcValueReader rd(log, shv::core::Exception::Throw);
		m_graphModel->clear();
		m_graphModel->beginAppendValues();
		m_graphModel->setTypeInfo(rd.logHeader().typeInfo());
		shvDebug() << "typeinfo:" << m_graphModel->typeInfo().toRpcValue().toCpon("  ");
		ShortTime anca_hook_short_time;
		while(rd.next()) {
			const core::utils::ShvJournalEntry &entry = rd.entry();
			if(!(entry.domain.empty()
				 || entry.domain == cp::Rpc::SIG_VAL_CHANGED
				 || entry.domain == cp::Rpc::SIG_VAL_FASTCHANGED))
				continue;
			int64_t msec = entry.epochMsec;
			if(entry.shortTime != core::utils::ShvJournalEntry::NO_SHORT_TIME) {
				uint16_t short_msec = static_cast<uint16_t>(entry.shortTime);
				ShortTime &st = short_times[entry.path];
				if(st.msec_sum == 0)
					st.msec_sum = msec;
				msec = st.addShortTime(short_msec);
			}
			bool ok;
			QVariant v = shv::coreqt::Utils::rpcValueToQVariant(entry.value, &ok);
			if(ok && v.isValid()) {
				shvDebug() << entry.path << v.typeName();
#if QT_VERSION_MAJOR >= 6
				if(entry.path == "data" && v.typeId() == QMetaType::QVariantList) {
#else
				if(entry.path == "data" && v.type() == QVariant::Map) {
#endif
					// Anca hook
					QVariantList vl = v.toList();
					uint16_t short_msec = static_cast<uint16_t>(vl.value(0).toInt());
					if(anca_hook_short_time.msec_sum == 0)
						anca_hook_short_time.msec_sum = msec;
					msec = anca_hook_short_time.addShortTime(short_msec);
					m_graphModel->appendValueShvPath("U", tl::Sample{msec, vl.value(1)});
					m_graphModel->appendValueShvPath("I", tl::Sample{msec, vl.value(2)});
					m_graphModel->appendValueShvPath("P", tl::Sample{msec, vl.value(3)});
				}
				else {
					m_graphModel->appendValueShvPath(entry.path, tl::Sample{msec, v});
				}
			}
		}
		m_graphModel->endAppendValues();
	}
	catch (const shv::core::Exception &e) {
		QMessageBox::warning(this, tr("Warning"), QString::fromStdString(e.message()));
	}

	m_graph->createChannelsFromModel();

	QSet<QString> channel_paths = m_graph->channelPaths();
	m_channelFilterDialog->init(shvPath(), channel_paths);
	ui->graphView->makeLayout();
	applyFilters(channel_paths);
}

void DlgLogInspector::showInfo(const QString &msg, bool is_error)
{
	if(msg.isEmpty()) {
		ui->lblInfo->hide();
	}
	else {
		QString ss = is_error? QString("background: salmon"): QString("background: lime");
		ui->lblInfo->setStyleSheet(ss);
		ui->lblInfo->setText(msg);
		ui->lblInfo->show();
	}
}

void DlgLogInspector::saveData(const std::string &data, QString ext)
{
	QString fn = QFileDialog::getSaveFileName(this, tr("Savefile"), QString(), "*" + ext);
	if(fn.isEmpty())
		return;
	if(!fn.endsWith(ext))
		fn = fn + ext;
	QFile f(fn);
	if(f.open(QFile::WriteOnly)) {
		f.write(data.data(), (qint64)data.size());
	}
	else {
		QMessageBox::warning(this, tr("Warning"), tr("Cannot open file '%1' for write.").arg(fn));
	}
}

void DlgLogInspector::setTimeZone(const QTimeZone &tz)
{
	shvDebug() << "Setting timezone to:" << tz.id();
	m_timeZone = tz;
	m_logModel->setTimeZone(tz);
	m_graphWidget->setTimeZone(tz);
}

void DlgLogInspector::applyFilters(const QSet<QString> &channel_paths)
{
	auto graph_filter = m_graph->channelFilter();
	graph_filter.setMatchingPaths(channel_paths);
	m_graph->setChannelFilter(graph_filter);
}

void DlgLogInspector::onChannelsFilterClicked()
{
	m_channelFilterDialog->setSelectedChannels(m_graph->channelFilter().matchingPaths());

	if(m_channelFilterDialog->exec() == QDialog::Accepted) {
		applyFilters(m_channelFilterDialog->selectedChannels());
	}
}

void DlgLogInspector::onGraphChannelFilterChanged()
{
	m_logSortFilterProxy->setChannelFilter(m_graph->channelFilter());
}

}}}
