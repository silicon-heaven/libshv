#pragma once

#ifdef USE_SHV_PATHS_GRANTS_CACHE
#include <string>

inline unsigned qHash(const std::string &s) noexcept //Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(s)))
{
	std::hash<std::string> hash_fn;
	return hash_fn(s);
}
#include <QCache>
#endif

#include <shv/broker/shvbrokerglobal.h>
#include <shv/broker/appclioptions.h>
#include <shv/broker/currentclientshvnode.h>
#include <shv/broker/tunnelsecretlist.h>
#include <shv/broker/aclmanager.h>

#include <shv/iotqt/node/shvnode.h>

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>
#include <QDateTime>

#ifdef WITH_SHV_LDAP
#include <optional>
#include <shv/broker/ldap/ldapconfig.h>
#endif
#include <shv/broker/azureconfig.h>
#include <set>

class QSocketNotifier;
class QSqlDatabase;

namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}
namespace shv { namespace core { namespace utils { class ShvUrl; }}}
namespace shv { namespace chainpack { class RpcSignal; }}

namespace shv {
namespace broker {

const std::string BROKER_CURRENT_CLIENT_SHV_PATH = std::string(shv::chainpack::Rpc::DIR_BROKER) + '/' + CurrentClientShvNode::NodeId;

namespace rpc { class WebSocketServer; class BrokerTcpServer; class ClientConnectionOnBroker;  class MasterBrokerConnection; class CommonRpcClientHandle; }

class AclManager;

class SHVBROKER_DECL_EXPORT BrokerApp : public QCoreApplication
{
	Q_OBJECT

	SHV_FIELD_BOOL_IMPL2(s, S, endLogAsSignalEnabled, false)
private:
	using Super = QCoreApplication;
	friend class AclManager;
	friend class MountsNode;
public:
	BrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BrokerApp() Q_DECL_OVERRIDE;

	AppCliOptions* cliOptions();
	static void registerLogTopics();

	static BrokerApp* instance();

	void onClientLogin(int connection_id);
	void onConnectedToMasterBrokerChanged(int connection_id, bool is_connected);

	void onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&meta, std::string &&data);

	rpc::MasterBrokerConnection* masterBrokerConnectionForClient(int client_id);

	void addSubscription(int client_id, const std::string &path, const std::string &method);
	bool removeSubscription(int client_id, const std::string &shv_path, const std::string &method);
	bool rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method);

	rpc::BrokerTcpServer* tcpServer();
	rpc::BrokerTcpServer* sslServer();
	rpc::ClientConnectionOnBroker* clientById(int client_id);

#ifdef WITH_SHV_WEBSOCKETS
	rpc::WebSocketServer* webSocketServer();
#endif

	rpc::CommonRpcClientHandle* commonClientConnectionById(int connection_id);

	QSqlDatabase sqlConfigConnection();

	AclManager *aclManager();
#ifdef WITH_SHV_LDAP
	std::optional<LdapConfig> ldapConfig();
#endif
	std::optional<AzureConfig> azureConfig();
	void setAclManager(AclManager *mng);
	void reloadConfig();
	void reloadConfigRemountDevices();
	bool checkTunnelSecret(const std::string &s);

	void checkLogin(const chainpack::UserLoginContext &ctx, const QObject* connection_ctx, const std::function<void(chainpack::UserLoginResult)> cb);

	void sendNewLogEntryNotify(const std::string &msg);

	const std::string& brokerId() const;
	iotqt::node::ShvNode * nodeForService(const shv::core::utils::ShvUrl &spp);

protected:
	virtual void initDbConfigSqlConnection();
	virtual AclManager* createAclManager();
private:
	void remountDevices();
	void reloadAcl();

	void lazyInit();

	void startTcpServers();

	void startWebSocketServers();

	rpc::ClientConnectionOnBroker* clientConnectionById(int connection_id);
	std::vector<int> clientConnectionIds();

	void createMasterBrokerConnections();
	QList<rpc::MasterBrokerConnection*> masterBrokerConnections() const;
	rpc::MasterBrokerConnection* masterBrokerConnectionById(int connection_id);

	std::vector<rpc::CommonRpcClientHandle *> allClientConnections();

	std::string resolveMountPoint(const shv::chainpack::RpcValue::Map &device_opts);

	void onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg);

	void onClientConnected(int client_id);

	void sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params);
	bool sendNotifyToSubscribers(const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data);

	static std::string brokerClientDirPath(int client_id);
	static std::string brokerClientAppPath(int client_id);

	std::string primaryIPAddress(bool &is_public);

	void propagateSubscriptionsToMasterBroker(rpc::MasterBrokerConnection *mbrconn);

protected:
	AppCliOptions *m_cliOptions;
	std::string m_brokerId;
	rpc::BrokerTcpServer *m_tcpServer = nullptr;
	rpc::BrokerTcpServer *m_sslServer = nullptr;
#ifdef WITH_SHV_WEBSOCKETS
	rpc::WebSocketServer *m_webSocketServer = nullptr;
	rpc::WebSocketServer *m_webSocketSslServer = nullptr;
#endif
#ifdef WITH_SHV_LDAP
	std::unique_ptr<void, void(*)(void*)> m_ldapLib;
	// LDAP username -> group
	std::optional<LdapConfig> m_ldapConfig;
#endif
	std::optional<AzureConfig> m_azureConfig;

	shv::iotqt::node::ShvNodeTree *m_nodesTree = nullptr;
	TunnelSecretList m_tunnelSecretList;
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	using PathGrantCache = QCache<std::string, shv::chainpack::Rpc::AccessGrant>;
	using UserPathGrantCache = QCache<std::string, PathGrantCache>;
	UserPathGrantCache m_userPathGrantCache;
#endif
	AclManager *m_aclManager = nullptr;
#ifdef Q_OS_UNIX
private:
	// Unix signal handlers.
	// You can't call Qt functions from Unix signal handlers,
	// but you can write to socket
	void installUnixSignalHandlers();
	Q_SLOT void handlePosixSignals();
	static void nativeSigHandler(int sig_number);

	QSocketNotifier *m_snTerm = nullptr;
#endif
};

}}
