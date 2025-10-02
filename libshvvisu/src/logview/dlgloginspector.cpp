#include <shv/visu/logview/dlgloginspector.h>
#include "ui_dlgloginspector.h"

#include <shv/visu/logview/logmodel.h>
#include <shv/visu/logview/logsortfilterproxymodel.h>

#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/channelfilterdialog.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/exception.h>
#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/utils.h>

#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#if QT_CONFIG(timezone)
#include <QTimeZone>
#endif

namespace cp = shv::chainpack;
namespace tl = shv::visu::timeline;

namespace shv::visu::logview {

enum { TabGraph = 0, TabData, TabInfo };

DlgLogInspector::DlgLogInspector(const QString &shv_path, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgLogInspector)
{
	ui->setupUi(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	ui->edSince->setTimeZone(QTimeZone::UTC);
	ui->edUntil->setTimeZone(QTimeZone::UTC);
#else
	ui->edSince->setTimeSpec(Qt::UTC);
	ui->edUntil->setTimeSpec(Qt::UTC);
#endif
	setShvPath(shv_path);
	{
		auto *m = new QMenu(this);
		{
			auto *a = new QAction(tr("ChainPack"), m);
			connect(a, &QAction::triggered, this, [this]() {
				auto log = m_logModel->log();
				std::string log_data = log.toChainPack();
				saveData(log_data, ".chpk");
			});
			m->addAction(a);
		}
		{
			auto *a = new QAction(tr("Cpon"), m);
			connect(a, &QAction::triggered, this, [this]() {
				auto log = m_logModel->log();
				std::string log_data = log.toCpon("\t");
				saveData(log_data, ".cpon");
			});
			m->addAction(a);
		}
		{
			auto *a = new QAction(tr("CSV"), m);
			connect(a, &QAction::triggered, this, [this]() {
				std::string log_data;
				for(int row=0; row<m_logModel->rowCount(); row++) {
					std::string row_data;
					for(int col=0; col<m_logModel->columnCount(); col++) {
						QModelIndex ix = m_logModel->index(row, col);
						if(col > 0)
							row_data += '\t';
						row_data += ix.data(Qt::DisplayRole).toString().toStdString();
					}
					if(row > 0)
						log_data += '\n';
					log_data += row_data;
				}
				saveData(log_data, ".csv");
			});
			m->addAction(a);
		}
		ui->btSaveData->setMenu(m);
	}

	{
		auto *m = new QMenu(this);
		{
			auto *a = new QAction(tr("ChainPack"), m);
			connect(a, &QAction::triggered, this, [this]() {
				std::string log_data = loadData(".chpk");
				std::string err;
				auto log = shv::chainpack::RpcValue::fromChainPack(log_data, &err);
				if (err.empty()) {
					m_logModel->setLog(log);
					parseLog(m_logModel->log());
				}
				else {
					QMessageBox::warning(this, tr("Warning"), tr("Invalid ChainPack file: ") + QString::fromStdString(err));
				}
			});
			m->addAction(a);
		}
		{
			auto *a = new QAction(tr("Cpon"), m);
			connect(a, &QAction::triggered, this, [this]() {
				std::string log_data = loadData(".cpon");
				std::string err;
				auto log = shv::chainpack::RpcValue::fromCpon(log_data, &err);
				if (err.empty()) {
					m_logModel->setLog(log);
					parseLog(m_logModel->log());
				}
				else {
					QMessageBox::warning(this, tr("Warning"), tr("Invalid CPON file: ") + QString::fromStdString(err));
				}
			});
			m->addAction(a);
		}

		ui->btLoadData->setMenu(m);
	}


	ui->lblInfo->hide();
	ui->btMoreOptions->setChecked(false);

	QDateTime dt2 = QDateTime::currentDateTime();
	QDateTime dt1 = dt2.addDays(-1);
	dt2 = dt2.addSecs(60 * 60);
	ui->edSince->setDateTime(dt1);
	ui->edUntil->setDateTime(dt2);
	connect(ui->btClearSince, &QPushButton::clicked, this, [this]() {
		ui->edSince->setDateTime(ui->edSince->minimumDateTime());
	});
	connect(ui->btClearUntil, &QPushButton::clicked, this, [this]() {
		ui->edUntil->setDateTime(ui->edUntil->minimumDateTime());
	});

	connect(ui->btTabGraph, &QAbstractButton::toggled, this, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabGraph);
	});
	connect(ui->btTabData, &QAbstractButton::toggled, this, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabData);
	});
	connect(ui->btTabInfo, &QAbstractButton::toggled, this, [this](bool is_checked) {
		if(is_checked)
			ui->stackedWidget->setCurrentIndex(TabInfo);
	});

	m_logModel = new LogModel(this);
	m_logSortFilterProxy = new shv::visu::logview::LogSortFilterProxyModel(this);
	m_logSortFilterProxy->setChannelFilterPathColumn(LogModel::ColPath);
	m_logSortFilterProxy->setFulltextFilterPathColumn(LogModel::ColPath);
	m_logSortFilterProxy->setValueColumn(LogModel::ColValue);
	m_logSortFilterProxy->setSourceModel(m_logModel);
	ui->tblData->setModel(m_logSortFilterProxy);
	ui->tblData->setSortingEnabled(false);
	ui->tblData->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().lineSpacing() * 1.3));

	ui->tblData->setContextMenuPolicy(Qt::ActionsContextMenu);
	{
		auto *copy = new QAction(tr("&Copy"));
		copy->setShortcut(QKeySequence::Copy);
		ui->tblData->addAction(copy);
		connect(copy, &QAction::triggered, this, [this]() {
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
				QClipboard *clipboard = QApplication::clipboard();
				clipboard->setText(rows);
			}
		});
	}

	m_graphModel = new tl::GraphModel(this);
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	m_graph = new tl::Graph(this);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);

	ui->wDataViewFilterSelector->init(ui->edShvPath->text(), m_graph);

#if QT_CONFIG(timezone)
	connect(ui->cbxTimeZone, &QComboBox::currentTextChanged, this, [this](const QString &) {
		auto tz = ui->cbxTimeZone->currentTimeZone();
		setTimeZone(tz);
	});
	setTimeZone(ui->cbxTimeZone->currentTimeZone());
#else
	ui->cbxTimeZone->setEnabled(false);
#endif

	connect(ui->btLoad, &QPushButton::clicked, this, &DlgLogInspector::downloadLog);

	connect(m_graph, &shv::visu::timeline::Graph::channelFilterChanged, this, &DlgLogInspector::onGraphChannelFilterChanged);

	connect(ui->btResizeColumnsToFitWidth, &QAbstractButton::clicked, this, [this]() {
		ui->tblData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	});

	loadSettings();
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
#if QT_CONFIG(timezone)
	auto get_dt = [this](QDateTimeEdit *ed) {
#else
	auto get_dt = [](QDateTimeEdit *ed) {
#endif
		QDateTime dt = ed->dateTime();
		if(dt == ed->minimumDateTime())
			return  cp::RpcValue();
#if QT_CONFIG(timezone)
		dt = QDateTime(dt.date(), dt.time(), m_timeZone);
#else
		dt = QDateTime(dt.date(), dt.time());
#endif
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
	shvDebug() << params.toRpcValue().toCpon();
	return params.toRpcValue();
}

void DlgLogInspector::downloadLog()
{
	std::string shv_path = shvPath().toStdString();
	if(ui->chkUseHistoryProvider->isChecked()) {
		if(shv_path.starts_with("shv/"))
			shv_path = "history" + shv_path.substr(3);
	}
	showInfo(QString::fromStdString("Downloading data from " + shv_path));
	cp::RpcValue params = getLogParams();
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	int rq_id = conn->nextRequestId();
	auto *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
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

void DlgLogInspector::parseLog(const shv::chainpack::RpcValue& log)
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
				 || entry.domain == cp::Rpc::SIG_VAL_FASTCHANGED
				 || entry.domain == "F"     //obsolete values
				 || entry.domain == "C"))
				continue;
			int64_t msec = entry.epochMsec;
			if(entry.shortTime != core::utils::ShvJournalEntry::NO_SHORT_TIME) {
				auto short_msec = static_cast<uint16_t>(entry.shortTime);
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
					m_graphModel->appendValueShvPath(QString::fromStdString(entry.path), tl::Sample{msec, v, !entry.isSpontaneous()});
				}
			}
		}
		m_graphModel->endAppendValues();
	}
	catch (const shv::core::Exception &e) {
		QMessageBox::warning(this, tr("Warning"), QString::fromStdString(e.message()));
	}

	m_graph->createChannelsFromModel();
	ui->graphView->makeLayout();
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

std::string DlgLogInspector::loadData(const QString &ext)
{
	QString fn = QFileDialog::getOpenFileName(this, tr("Loadfile"), QString(), "*" + ext);
	if(fn.isEmpty())
		return "";
	if(!fn.endsWith(ext))
		fn = fn + ext;
	QFile f(fn);
	if(f.open(QFile::ReadOnly)) {
		return f.readAll().toStdString();
	}

	QMessageBox::warning(this, tr("Warning"), tr("Cannot open file '%1' for read.").arg(fn));
	return "";
}

void DlgLogInspector::saveData(const std::string &data_to_be_saved, const QString& ext)
{
	QString fn = QFileDialog::getSaveFileName(this, tr("Savefile"), QString(), "*" + ext);
	if(fn.isEmpty())
		return;
	if(!fn.endsWith(ext))
		fn = fn + ext;
	QFile f(fn);
	if(f.open(QFile::WriteOnly)) {
		f.write(data_to_be_saved.data(), static_cast<qint64>(data_to_be_saved.size()));
	}
	else {
		QMessageBox::warning(this, tr("Warning"), tr("Cannot open file '%1' for write.").arg(fn));
	}
}

#if QT_CONFIG(timezone)
void DlgLogInspector::setTimeZone(const QTimeZone &tz)
{
	shvDebug() << "Setting timezone to:" << tz.id();
	m_timeZone = tz;
	m_logModel->setTimeZone(tz);
	m_graphWidget->setTimeZone(tz);
}
#endif

void DlgLogInspector::onGraphChannelFilterChanged()
{
	m_logSortFilterProxy->setChannelFilter(m_graph->channelFilter());
}

}
