#pragma once

#include "../shvvisuglobal.h"

#include <shv/core/utils.h>

#include <QDialog>

#if SHVVISU_HAS_TIMEZONE
#include <QTimeZone>
#endif

namespace shv { namespace chainpack { class RpcValue; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace visu { namespace timeline { class GraphWidget; class GraphModel; class Graph; class ChannelFilterDialog;}}}

namespace shv {
namespace visu {
namespace logview {

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

	void applyFilters(const QSet<QString> &channel_paths);
	void onChannelsFilterClicked();
	void onGraphChannelFilterChanged();

	void initVisualSettingSelector(const QString &shv_path);
	void onSaveViewClicked();
	void onDeleteViewClicked();
	void onViewSelected(int index);
	void setView(const QString &name);

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
	shv::visu::timeline::ChannelFilterDialog *m_channelFilterDialog = nullptr;
};

}}}
