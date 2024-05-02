#pragma once

#include "../shvvisuglobal.h"

#include <shv/core/utils.h>

#include <QDialog>

#if SHVVISU_HAS_TIMEZONE
#include <QTimeZone>
#endif

namespace shv::chainpack { class RpcValue; }
namespace shv::iotqt::rpc { class ClientConnection; }
namespace shv::visu::timeline { class GraphWidget; class GraphModel; class Graph; class ChannelFilterDialog;}

namespace shv::visu::logview {

namespace Ui {
class DlgLogInspector;
}

class LogModel;
class LogSortFilterProxyModel;

class SHVVISU_DECL_EXPORT DlgLogInspector : public QDialog
{
	Q_OBJECT
public:
	explicit DlgLogInspector(const QString &shv_path, QWidget *parent = nullptr);
	~DlgLogInspector();

	shv::iotqt::rpc::ClientConnection* rpcConnection();
	void setRpcConnection(shv::iotqt::rpc::ClientConnection *c);

	QString shvPath() const;

private:
	void setShvPath(const QString &s);
	void downloadLog();
	void loadSettings();
	void saveSettings();

	shv::chainpack::RpcValue getLogParams();
	void parseLog(shv::chainpack::RpcValue log);

	void showInfo(const QString &msg = QString(), bool is_error = false);
	void saveData(const std::string &data, QString ext);
	std::string loadData(const QString &ext);

#if SHVVISU_HAS_TIMEZONE
	void setTimeZone(const QTimeZone &tz);
#endif

	void onGraphChannelFilterChanged();

private:
	Ui::DlgLogInspector *ui;

#if SHVVISU_HAS_TIMEZONE
	QTimeZone m_timeZone;
#endif

	shv::iotqt::rpc::ClientConnection* m_rpcConnection = nullptr;

	LogModel *m_logModel = nullptr;
	LogSortFilterProxyModel *m_logSortFilterProxy = nullptr;

	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
};
}
