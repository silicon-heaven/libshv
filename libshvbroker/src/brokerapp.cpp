#include "brokerapp.h"
#include "aclmanagersqlite.h"
#include "currentclientshvnode.h"
#include "clientshvnode.h"
#include "brokerappnode.h"
#include "subscriptionsnode.h"
#include "brokeraclnode.h"
#include "clientconnectionnode.h"
#include "brokerrootnode.h"
#include "rpc/brokertcpserver.h"
#include "rpc/clientconnectiononbroker.h"
#include "rpc/masterbrokerconnection.h"

#ifdef WITH_SHV_WEBSOCKETS
#include "rpc/websocketserver.h"
#endif

#include <shv/iotqt/utils/network.h>
#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/utils/shvpath.h>
#include <shv/core/utils/shvurl.h>

#include <shv/coreqt/log.h>

#include <shv/core/string.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/accessgrant.h>

#include <QFile>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QTimer>
#include <QUdpSocket>

#include <ctime>
#include <fstream>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define logTunnelD() nCDebug("Tunnel")

//#define logAccessM() nCMessage("Access")

#define logAclResolveW() nCWarning("AclResolve")
#define logAclResolveM() nCMessage("AclResolve")

#define logBrokerDiscoveryM() nCMessage("BrokerDiscovery")
#define logServiceProvidersM() nCMessage("ServiceProviders")

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSigResolveD() nCDebug("SigRes").color(NecroLog::Color::LightGreen)

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

namespace cp = shv::chainpack;
namespace acl = shv::iotqt::acl;

using namespace std;

namespace shv {
namespace broker {

#ifdef Q_OS_UNIX
int BrokerApp::m_sigTermFd[2];
#endif

static string BROKER_CURRENT_CLIENT_SHV_PATH = string(cp::Rpc::DIR_BROKER) + '/' + CurrentClientShvNode::NodeId;

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods {
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		}
	{ }

	StringList childNames(const StringViewList &shv_path) override
	{
		auto sl = Super::childNames(shv_path);
		if(shv_path.empty()) {
			std::vector<int> ids;
			ids.reserve(sl.size());
			for(const auto &s : sl)
				ids.push_back(std::stoi(s));
			std::sort(ids.begin(), ids.end());
			for (size_t i = 0; i < ids.size(); ++i)
				sl[i] = std::to_string(ids[i]);
		}
		return sl;
	}
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MasterBrokersNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	MasterBrokersNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
		}
	{ }
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MountsNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	MountsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(parent)
	{ }

	size_t methodCount(const StringViewList &shv_path) override
	{
		if(shv_path.empty())
			return m_metaMethods.size() - 1;
		if(shv_path.size() == 1)
			return m_metaMethods.size();
		return Super::methodCount(shv_path);
	}

	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override
	{
		if(methodCount(shv_path) <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(methodCount(shv_path)));
		if(shv_path.empty())
			return &(m_metaMethods[ix]);
		if(shv_path.size() == 1)
			return &(m_metaMethods[ix]);
		return Super::metaMethod(shv_path, ix);
	}

	StringList childNames(const StringViewList &shv_path) override
	{
		if(shv_path.empty()) {
			BrokerApp *app = BrokerApp::instance();
			StringList lst;
			for(int id : app->clientConnectionIds()) {
				rpc::ClientConnectionOnBroker *conn = app->clientConnectionById(id);
				const string mp = conn->mountPoint();
				if(!mp.empty())
					lst.push_back(shv::core::utils::ShvPath::SHV_PATH_QUOTE + mp + shv::core::utils::ShvPath::SHV_PATH_QUOTE);
			}
			std::sort(lst.begin(), lst.end());
			return lst;
		}
		else {
			if(shv_path.size() == 1) {
				return StringList();
			}
		}
		return Super::childNames(shv_path);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override
	{
		if(shv_path.size() == 1) {
			if(method == METH_CLIENT_IDS) {
				BrokerApp *app = BrokerApp::instance();
				std::string path = shv_path.at(0).slice(1, -1).toString();
				auto *nd1 = app->m_nodesTree->cd(path);
				if(nd1 == nullptr)
					SHV_EXCEPTION("Cannot find node on path: " + path);
				auto *nd = qobject_cast<ClientShvNode*>(nd1);
				if(nd == nullptr)
					SHV_EXCEPTION("Wrong node type on path: " + path + ", looking for ClientShvNode, found: " + nd1->metaObject()->className());
				cp::RpcValue::List lst;
				for(rpc::ClientConnectionOnBroker *conn : nd->connections())
					lst.push_back(conn->connectionId());
				return cp::RpcValue{lst};
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}
private:
	static const char *METH_CLIENT_IDS;
	static std::vector<cp::MetaMethod> m_metaMethods;
};

const char *MountsNode::METH_CLIENT_IDS = "clientIds";

std::vector<cp::MetaMethod> MountsNode::m_metaMethods = {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
	{METH_CLIENT_IDS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_CONFIG},
};

static const auto SQL_CONFIG_CONN_NAME = QStringLiteral("ShvBrokerDbConfigSqlConnection");
//static constexpr int SQL_RECONNECT_INTERVAL = 3000;
BrokerApp::BrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//shvInfo() << "creating SHV BROKER application object ver." << versionString();
	m_brokerId = m_cliOptions->brokerId();
	std::srand(std::time(nullptr));
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif
	m_nodesTree = new shv::iotqt::node::ShvNodeTree(new BrokerRootNode(), this);
	connect(m_nodesTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, this, &BrokerApp::onRootNodeSendRpcMesage);
	BrokerAppNode *bn = new BrokerAppNode();
	m_nodesTree->mount(cp::Rpc::DIR_BROKER_APP, bn);
	m_nodesTree->mount(BROKER_CURRENT_CLIENT_SHV_PATH, new CurrentClientShvNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/clients", new ClientsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/masters", new MasterBrokersNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/mounts", new MountsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/etc/acl", new EtcAclRootNode());

	if (m_cliOptions->discoveryPort() > 0) {
		QUdpSocket *udp_socket = new QUdpSocket(this);
		udp_socket->bind(m_cliOptions->discoveryPort(), QUdpSocket::ShareAddress);
		logBrokerDiscoveryM() << "shvbrokerDiscovery listen on UDP port:" << m_cliOptions->discoveryPort();
		connect(udp_socket, &QUdpSocket::readyRead, this, [this, udp_socket]() {
			QByteArray datagram;
			QHostAddress address;
			quint16 port;
			while (udp_socket->hasPendingDatagrams()) {
				datagram.resize(int(udp_socket->pendingDatagramSize()));
				udp_socket->readDatagram(datagram.data(), datagram.size(), &address, &port);

				std::string rq_cpon(datagram.constData(), datagram.length());
				shv::chainpack::RpcValue rv = shv::chainpack::RpcValue::fromCpon(rq_cpon);
				shv::chainpack::RpcRequest rq(rv);
				if (rq.method() == "shvbrokerDiscovery") {
					logBrokerDiscoveryM() << "Received broadcast request shvbrokerDiscovery:" << rq.toPrettyString();
					shv::chainpack::RpcResponse resp = rq.makeResponse();
					QString ipv4 = shv::iotqt::utils::Network::primaryIPv4Address().toString();
					shv::chainpack::RpcValue response = shv::chainpack::RpcValue::Map { {"brokerId", m_brokerId}, {"brokerIPv4", ipv4.toStdString()}, {"brokerPort", m_cliOptions->serverPort() } };
					resp.setResult(response);
					QByteArray response_datagram(resp.toCpon().c_str(), resp.toCpon().length());
					udp_socket->writeDatagram(response_datagram, address, port);
					logBrokerDiscoveryM() << "Send response on broadcast shvbrokerDiscovery:" << resp.toPrettyString();
				}
			}
		});
	}

	QTimer::singleShot(0, this, &BrokerApp::lazyInit);
}

BrokerApp::~BrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
}

void BrokerApp::registerLogTopics()
{
	NecroLog::registerTopic("Access", "users access log");
	NecroLog::registerTopic("AclResolve", "users and grants resolving");
	NecroLog::registerTopic("AclManager", "ACL manager");
	NecroLog::registerTopic("RpcData", "dump RPC mesages as HEX data");
	NecroLog::registerTopic("RpcMsg", "dump RPC messages");
	NecroLog::registerTopic("RpcRawMsg", "dump raw RPC messages");
	NecroLog::registerTopic("ServiceProviders", "service providers");
	NecroLog::registerTopic("SigRes", "signal resolution in client subscriptions");
	NecroLog::registerTopic("Subscr", "subscriptions creation and propagation");
	NecroLog::registerTopic("Tunnel", "tunneling");
	NecroLog::registerTopic("WriteQueue", "RpcDriver write queue");
}

#ifdef Q_OS_UNIX
void BrokerApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	for(int sig_num : {SIGTERM, SIGHUP, SIGUSR1}) {
		struct sigaction sa;

		sa.sa_handler = BrokerApp::nativeSigHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags |= SA_RESTART;

		if(sigaction(sig_num, &sa, nullptr) > 0)
			qFatal("Couldn't register posix signal handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &BrokerApp::handlePosixSignals);
	shvInfo() << "SIG_TERM handler installed OK";
}

void BrokerApp::nativeSigHandler(int sig_number)
{
	shvInfo() << "SIG:" << sig_number;
	unsigned char a = static_cast<unsigned char>(sig_number);
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void BrokerApp::handlePosixSignals()
{
	m_snTerm->setEnabled(false);
	unsigned char sig_num;
	::read(m_sigTermFd[1], &sig_num, sizeof(sig_num));

	shvInfo() << "SIG" << sig_num << "catched.";
	if(sig_num == SIGTERM || sig_num == SIGHUP) {
		QTimer::singleShot(0, this, &BrokerApp::quit);
	}
	else if(sig_num == SIGUSR1) {
		//QMetaObject::invokeMethod(this, &BrokerApp::reloadConfig, Qt::QueuedConnection); since Qt 5.10
		QTimer::singleShot(0, this, &BrokerApp::reloadConfigRemountDevices);
	}

	m_snTerm->setEnabled(true);
}
#endif

rpc::BrokerTcpServer *BrokerApp::tcpServer()
{
	if(!m_tcpServer)
		SHV_EXCEPTION("TCP server is NULL!");
	return m_tcpServer;
}

rpc::BrokerTcpServer *BrokerApp::sslServer()
{
	if(!m_sslServer)
		SHV_EXCEPTION("SSL server is NULL!");
	return m_sslServer;
}

rpc::ClientConnectionOnBroker *BrokerApp::clientById(int client_id)
{
	return clientConnectionById(client_id);
}

#ifdef WITH_SHV_WEBSOCKETS
rpc::WebSocketServer *BrokerApp::webSocketServer()
{
	if(!m_webSocketServer)
		SHV_EXCEPTION("WebSocket server is NULL!");
	return m_webSocketServer;
}
#endif

void BrokerApp::reloadConfig()
{
	shvInfo() << "Reloading config";
	reloadAcl();
}

void BrokerApp::reloadConfigRemountDevices()
{
	reloadConfig();
	remountDevices();
	//emit configReloaded();
}

void BrokerApp::reloadAcl()
{
	aclManager()->reload();
}

void BrokerApp::startTcpServers()
{
	const auto *opts = cliOptions();

	if(opts->serverPort_isset()) {
		// port must be set explicitly to enable server
		SHV_SAFE_DELETE(m_tcpServer);
		int port = opts->serverPort();
		if(port > 0) {
			shvInfo() << "Starting plain socket server on port" << port;
			m_tcpServer = new rpc::BrokerTcpServer(rpc::BrokerTcpServer::NonSecureMode, this);
			if(!m_tcpServer->start(port)) {
				SHV_EXCEPTION("Cannot start TCP server!");
			}
		}
		else {
			shvInfo() << "TCP server port is not set, it will not be started.";
		}
	}

	if(opts->serverSslPort_isset()) {
		SHV_SAFE_DELETE(m_sslServer);
		int port = opts->serverSslPort();
		if(port > 0) {
			shvInfo() << "Starting SSL server on port" << port;
			m_sslServer = new rpc::BrokerTcpServer(rpc::BrokerTcpServer::SecureMode, this);
			if(!m_sslServer->loadSslConfig()) {
				SHV_EXCEPTION("Cannot start SSL server, invalid SSL config!");
			}
			if(!m_sslServer->start(port)) {
				SHV_EXCEPTION("Cannot start SSL server!");
			}
		}
		else {
			shvInfo() << "SSL server port is not set, it will not be started.";
		}
	}
}

void BrokerApp::startWebSocketServers()
{
#ifdef WITH_SHV_WEBSOCKETS
	auto *opts = cliOptions();
	if(opts->serverWebsocketPort_isset()) {
		SHV_SAFE_DELETE(m_webSocketServer);
		int port = opts->serverWebsocketPort();
		if(port > 0) {
			m_webSocketServer = new rpc::WebSocketServer(QWebSocketServer::NonSecureMode, this);
			if(!m_webSocketServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket server!");
			}
		}
	}
	else {
		shvInfo() << "Websocket server port is not set, it will not be started.";
	}
	if(opts->serverWebsocketSslPort_isset()) {
		SHV_SAFE_DELETE(m_webSocketSslServer);
		int port = opts->serverWebsocketSslPort();
		if(port > 0) {
			m_webSocketSslServer = new rpc::WebSocketServer(QWebSocketServer::SecureMode, this);
			if(!m_webSocketSslServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket SSL server!");
			}
		}
	}
	else {
		shvInfo() << "Websocket SSL server port is not set, it will not be started.";
	}
#else
	shvWarning() << "Websocket server is not included in this build";
#endif
}

rpc::ClientConnectionOnBroker *BrokerApp::clientConnectionById(int connection_id)
{
	shvLogFuncFrame() << "conn id:" << connection_id;

	rpc::ClientConnectionOnBroker *conn = nullptr;
	if (m_sslServer) {
		conn = sslServer()->connectionById(connection_id);
		shvDebug() << "SSL connection:" << conn;
	}
	if (!conn && m_tcpServer) {
		conn = tcpServer()->connectionById(connection_id);
		shvDebug() << "TCP connection:" << conn;
	}
#ifdef WITH_SHV_WEBSOCKETS
	if(!conn && m_webSocketServer) {
		conn = m_webSocketServer->connectionById(connection_id);
		shvDebug() << "ws connection:" << conn;
	}
	if(!conn && m_webSocketSslServer) {
		conn = m_webSocketSslServer->connectionById(connection_id);
		shvDebug() << "wss connection:" << conn;
	}
#endif
	return conn;
}

std::vector<int> BrokerApp::clientConnectionIds()
{
	std::vector<int> ids;
	if(m_tcpServer) {
		ids = m_tcpServer->connectionIds();
	}
	if(m_sslServer) {
		std::vector<int> ids2 = m_sslServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
#ifdef WITH_SHV_WEBSOCKETS
	if(m_webSocketServer) {
		std::vector<int> ids2 = m_webSocketServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
	if(m_webSocketSslServer) {
		std::vector<int> ids2 = m_webSocketSslServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
#endif
	return ids;
}

void BrokerApp::lazyInit()
{
	initDbConfigSqlConnection();
	reloadConfig();
	startTcpServers();
	startWebSocketServers();
	createMasterBrokerConnections();
}

bool BrokerApp::checkTunnelSecret(const std::string &s)
{
	return m_tunnelSecretList.checkSecret(s);
}

chainpack::UserLoginResult BrokerApp::checkLogin(const chainpack::UserLoginContext &ctx)
{
	return aclManager()->checkPassword(ctx);
}

void BrokerApp::sendNewLogEntryNotify(const std::string &msg)
{
	if(isSendLogAsSignalEnabled())
		sendNotifyToSubscribers(".broker/app/log", cp::Rpc::SIG_VAL_CHANGED, msg);
}

void BrokerApp::initDbConfigSqlConnection()
{
	AppCliOptions *opts = cliOptions();
	if(!opts->isSqlConfigEnabled())
		return;
	if(opts->sqlConfigDriver() == "QSQLITE") {
		std::string fn = opts->sqlConfigDatabase();
		if(fn.empty())
			SHV_EXCEPTION("SQL config database not set.");
		if(fn[0] != '/')
			fn = opts->effectiveConfigDir() + '/' + fn;
		QString qfn = QString::fromStdString(fn);
		shvInfo() << "Openning SQL config database:" << fn;
		QSqlDatabase db = QSqlDatabase::addDatabase(QString::fromStdString(cliOptions()->sqlConfigDriver()), SQL_CONFIG_CONN_NAME);
		db.setDatabaseName(qfn);
		if(!db.open())
			SHV_EXCEPTION("Cannot open SQLite SQL config database " + fn);
	}
	else {
		SHV_EXCEPTION("SQL config SQL driver " + opts->sqlConfigDriver() + " is not supported.");
	}
}

AclManager *BrokerApp::createAclManager()
{
	auto *opts = cliOptions();
	if(opts->isSqlConfigEnabled()) {
		return new AclManagerSqlite(this);
	}
	return new AclManagerConfigFiles(this);
}

iotqt::node::ShvNode *BrokerApp::nodeForService(const core::utils::ShvUrl &spp)
{
	if(spp.isServicePath()) {
		iotqt::node::ShvNode *ret = m_nodesTree->cd(spp.service().toString());
		if(ret) {
			core::StringView request_broker_id = spp.brokerId();
			if(!request_broker_id.empty() && !(request_broker_id == brokerId()))
				return nullptr;
			return ret;
		}
	}
	return nullptr;
}

void BrokerApp::remountDevices()
{
	shvInfo() << "Remounting devices by dropping their connection";
	for(int conn_id : clientConnectionIds()) {
		rpc::ClientConnectionOnBroker *conn = clientConnectionById(conn_id);
		if(conn && !conn->mountPoint().empty()) {
			shvInfo() << "Dropping connection ID:" << conn_id << "mounted on:" << conn->mountPoint();
			conn->close();
		}
	}
}

std::string BrokerApp::resolveMountPoint(const shv::chainpack::RpcValue::Map &device_opts)
{
	std::string mount_point;
	shv::chainpack::RpcValue device_id = device_opts.value(cp::Rpc::KEY_DEVICE_ID);
	if(device_id.isValid())
		mount_point = aclManager()->mountPointForDevice(device_id);
	if(mount_point.empty()) {
		mount_point = device_opts.value(cp::Rpc::KEY_MOUT_POINT).toString();
		std::vector<shv::core::StringView> path = shv::core::utils::ShvPath::split(mount_point);
		if(path.size() && !(path[0] == "test")) {
			shvWarning() << "Mount point can be explicitly specified to test/ dir only, dev id:" << device_id.toCpon();
			mount_point.clear();
		}
	}
	if(mount_point.empty()) {
		shvWarning() << "cannot find mount point for device id:" << device_id.toCpon();// << "connection id:" << connection_id;
	}
	return mount_point;
}

std::string BrokerApp::primaryIPAddress(bool &is_public)
{
	if(cliOptions()->publicIP_isset())
		return cliOptions()->publicIP();
	QHostAddress ha = shv::iotqt::utils::Network::primaryPublicIPv4Address();
	if(!ha.isNull()) {
		is_public = true;
		return ha.toString().toStdString();
	}
	is_public = false;
	ha = shv::iotqt::utils::Network::primaryIPv4Address();
	if(!ha.isNull())
		return ha.toString().toStdString();
	return std::string();
}

void BrokerApp::propagateSubscriptionsToMasterBroker(rpc::MasterBrokerConnection *mbrconn)
{
	logSubscriptionsD() << "Connected to main master broker, propagating client subscriptions.";
	for(int id : clientConnectionIds()) {
		rpc::ClientConnectionOnBroker *conn = clientConnectionById(id);
		for (size_t i = 0; i < conn->subscriptionCount(); ++i) {
			const rpc::CommonRpcClientHandle::Subscription &subs = conn->subscriptionAt(i);
			shv::core::utils::ShvUrl spp(subs.localPath);
			if(spp.isServicePath()) {
				logSubscriptionsD() << "client id:" << id << "propagating subscription for path:" << subs.localPath << "method:" << subs.method;
				mbrconn->callMethodSubscribe(subs.localPath, subs.method);
			}
		}
	}
}

chainpack::AccessGrant BrokerApp::accessGrantForRequest(rpc::CommonRpcClientHandle *conn, const shv::core::utils::ShvUrl &shv_url, const std::string &method, const shv::chainpack::RpcValue &rq_grant)
{
	logAclResolveM() << "==== accessGrantForShvPath user:" << conn->loggedUserName() << "requested path:" << shv_url.toString() << "method:" << method << "request grant:" << rq_grant.toCpon();
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	PathGrantCache *user_path_grants = m_userPathGrantCache.object(user_name);
	if(user_path_grants) {
		cp::Rpc::AccessGrant *pg = user_path_grants->object(shv_path);
		if(pg) {
			logAclD() << "\t cache hit:" << pg->grant << "weight:" << pg->weight;
			return *pg;
		}
	}
#endif
	bool is_request_from_master_broker = conn->isMasterBrokerConnection();
	auto request_grant = cp::AccessGrant::fromRpcValue(rq_grant);
	if(is_request_from_master_broker) {
		if(request_grant.isValid()) {
			// access resolved by master broker already, forward use this
			logAclResolveM() << "\t Resolved on master broker already.";
			return request_grant;
		}
	}
	else {
		if(request_grant.isValid()) {
			logAclResolveM() << "Client defined grants in RPC request are not implemented yet and will be ignored.";
		}
	}
	//string rq_service = sp_path.service().toString();
	//string rq_shv_path = sp_path.pathPart().toString();
	std::vector<AclManager::FlattenRole> flatten_user_roles;
	if(is_request_from_master_broker) {
		// set masterBroker role to requests from master broker without access grant specified
		// This is used mainly for service calls as (un)subscribe propagation to slave brokers etc.
		if(shv_url.pathPart() == cp::Rpc::DIR_BROKER_APP) {
			// master broker has always rd grant to .broker/app path
			return cp::AccessGrant(cp::Rpc::ROLE_WRITE);
		}
		flatten_user_roles = aclManager()->flattenRole(cp::Rpc::ROLE_MASTER_BROKER);
	}
	else {
		flatten_user_roles = aclManager()->userFlattenRoles(conn->loggedUserName());
	}
	//logAclResolveM() << "user:" << conn->loggedUserName() << "flatten roles:";
	//for(const AclManager::FlattenRole &flatten_role : flatten_user_roles) {
	//	logAclResolveM() << "\t" << flatten_role.name << "\tweight:" << flatten_role.weight << "\tnest level:" << flatten_role.nestLevel;
	//}
	logAclResolveM() << "searched rules:" << [this, &flatten_user_roles]()
	{
		auto to_str = [](const QVariant &v, int len) {
			bool right = false;
			if(len < 0) {
				right = true;
				len = -len;
			}
			QString s = v.toString();
			if(s.length() < len) {
				QString spaces = QString(len - s.length(), ' ');
				if(right)
					s = spaces + s;
				else
					s = s + spaces;
			}
			return s;
		};
		vector<int> cols = {15, 10, 10, 20, 15, 1};
		const int row_len = 80;
		QString tbl = "\n" + QString(row_len, '-') + '\n';
		tbl += to_str("role", cols[0]);
		tbl += to_str("weight", cols[1]);
		tbl += to_str("service", cols[2]);
		tbl += to_str("pattern", cols[3]);
		tbl += to_str("method", cols[4]);
		tbl += to_str("grant", cols[5]);
		tbl += "\n" + QString(row_len, '-');
		for(const AclManager::FlattenRole &role : flatten_user_roles) {
			const acl::AclRoleAccessRules &role_rules = aclManager()->accessRoleRules(role.name);
			for(const acl::AclAccessRule &access_rule : role_rules) {
				tbl += "\n";
				tbl += to_str(role.name.c_str(), cols[0]);
				tbl += to_str(role.weight, cols[1]);
				tbl += to_str(access_rule.service.c_str(), cols[2]);
				tbl += to_str(access_rule.pathPattern.c_str(), cols[3]);
				tbl += to_str(access_rule.method.c_str(), cols[4]);
				tbl += to_str(access_rule.grant.toRpcValue().toCpon().c_str(), cols[5]);
			}
		}
		tbl += "\n" + QString(row_len, '-');
		return tbl;
	}();
	// find most specific path grant for role with highest weight
	// user_flattent_grants are sorted by weight DESC
	acl::AclAccessRule most_specific_rule;
	if(shv_url.pathPart() == BROKER_CURRENT_CLIENT_SHV_PATH) {
		// client has WR grant on currentClient node
		most_specific_rule.grant = cp::AccessGrant{cp::Rpc::ROLE_WRITE};
	}
	else {
		/*
		shv::core::utils::ServiceProviderPath sp_path(rq_shv_path);
		string service_name;
		string rq_path_part = rq_shv_path;
		if(sp_path.isDownTree()) {
			service_name = sp_path.service().toString();
			rq_path_part = sp_path.pathPart().toString();
		}
		*/
		// roles are sorted in weight DESC
		int old_weight = std::numeric_limits<int>::max();
		for(const AclManager::FlattenRole &flatten_role : flatten_user_roles) {
			if(flatten_role.weight != old_weight) {
				if(most_specific_rule.isValid()) {
					// roles with lower weight have lower priority, skip them
					break;
				}
				old_weight = flatten_role.weight;
			}
			logAclResolveM() << "----- checking role:" << flatten_role.name << "with weight:" << flatten_role.weight << "nest level:" << flatten_role.nestLevel;
			const acl::AclRoleAccessRules &role_rules = aclManager()->accessRoleRules(flatten_role.name);
			if(role_rules.empty()) {
				logAclResolveM() << "\t no paths defined.";
			}
			else for(const acl::AclAccessRule &access_rule : role_rules) {
				// rules are sorted as most specific first
				logAclResolveM() << "rule:" << access_rule.toRpcValue().toCpon();
				if(access_rule.isPathMethodMatch(shv_url, method)) {
					if(access_rule.isMoreSpecificThan(most_specific_rule)) {
						logAclResolveM() << "\t+++HIT more specific rule than previous:"
											<< (most_specific_rule.isValid()? most_specific_rule.toRpcValue().toCpon(): "INVALID")
											<< "found";
						most_specific_rule = access_rule;
					}
					else if(!most_specific_rule.isMoreSpecificThan(access_rule)) {
						// the same specific rules, this is problem
						logAclResolveW() << "the same specific rules found!";
						logAclResolveW() << "\t" << access_rule.toRpcValue().toCpon();
						logAclResolveW() << "\t" << most_specific_rule.toRpcValue().toCpon();
					}
					else {
						logAclResolveM() << "\t---HIT but rule is not more specific than:" << most_specific_rule.toRpcValue().toCpon();
					}
				}
			}
		}
	}
	if(!most_specific_rule.isValid()) {
		logAclResolveM() << "no match found, permission denied!";
	}
	logAclResolveM() << "access user:" << conn->loggedUserName()
				 << "shv_path:" << shv_url.toString()
				 << "rq_grant:" << (rq_grant.isValid()? rq_grant.toCpon(): "<none>")
				 << "==== path:" << most_specific_rule.pathPattern << "method:" << most_specific_rule.method << "grant:" << most_specific_rule.grant.toRpcValue().toCpon();
	return most_specific_rule.grant;
}

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::ClientConnectionOnBroker *conn = clientConnectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));
	//const shv::chainpack::RpcValue::Map &opts = conn->connectionOptions();

	shvInfo() << "Client login connection id:" << connection_id;// << "connection type:" << conn->connectionType();
	{
		std::string dir_mount_point = brokerClientDirPath(connection_id);
		{
			shv::iotqt::node::ShvNode *dir = m_nodesTree->cd(dir_mount_point);
			if(dir) {
				shvError() << "Client dir" << dir_mount_point << "exists already and will be deleted, this should never happen!";
				dir->setParentNode(nullptr);
				delete dir;
			}
		}
		shv::iotqt::node::ShvNode *clients_nd = m_nodesTree->mkdir(std::string(cp::Rpc::DIR_BROKER) + "/clients/");
		if(!clients_nd)
			SHV_EXCEPTION("Cannot create parent for ClientDirNode id: " + std::to_string(connection_id));
		ClientConnectionNode *client_id_node = new ClientConnectionNode(connection_id, clients_nd);
		ClientShvNode *client_app_node = new ClientShvNode("app", conn, client_id_node);
		// delete whole client tree, when client is destroyed
		connect(conn, &rpc::ClientConnectionOnBroker::destroyed, client_id_node, &ClientShvNode::deleteLater);

		conn->setParent(client_app_node);
		{
			std::string mount_point = client_id_node->shvPath() + "/subscriptions";
			SubscriptionsNode *nd = new SubscriptionsNode(conn);
			if(!m_nodesTree->mount(mount_point, nd))
				SHV_EXCEPTION("Cannot mount connection subscription list to device tree, connection id: " + std::to_string(connection_id)
							  + " shv path: " + mount_point);
		}
	}

	if(conn->deviceOptions().isMap()) {
		const auto device_opts = conn->deviceOptions();
		std::string mount_point = resolveMountPoint(device_opts.asMap());
		if(!mount_point.empty()) {
			string path_rest;
			ClientShvNode *cli_nd = qobject_cast<ClientShvNode*>(m_nodesTree->cd(mount_point, &path_rest));
			if(cli_nd) {
				/*
				shvWarning() << "The mount point" << mount_point << "exists already";
				if(cli_nd->connection()->deviceId() == device_id) {
					shvWarning() << "The same device ID will be remounted:" << device_id.toCpon();
					delete cli_nd;
				}
				*/
				cli_nd->addConnection(conn);
			}
			else if(path_rest.empty()) {
				SHV_EXCEPTION("Cannot mount device to not-leaf node, connection id: " + std::to_string(connection_id) + ", mount point: " + mount_point);
			}
			else {
				cli_nd = new ClientShvNode(std::string(), conn);
				if(!m_nodesTree->mount(mount_point, cli_nd))
					SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
				connect(cli_nd, &ClientShvNode::destroyed, cli_nd->parentNode(), &shv::iotqt::node::ShvNode::deleteIfEmptyWithParents, Qt::QueuedConnection);
			}
			mount_point = cli_nd->shvPath();
			QTimer::singleShot(0, this, [this, mount_point]() {
				//shvInfo() << "mounted node created:" << mount_point;
				sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, true);
			});
			shvInfo() << "client connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->setMountPoint(mount_point);
			connect(cli_nd, &ClientShvNode::destroyed, this, [this, mount_point]() {
				shvInfo() << "mounted node destroyed:" << mount_point;
				sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, false);
			});
			QTimer::singleShot(0, this, [this, connection_id]() {
				onClientConnected(connection_id);
			});
		}
	}
	//shvInfo() << m_deviceTree->dumpTree();
}

void BrokerApp::onConnectedToMasterBrokerChanged(int connection_id, bool is_connected)
{
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(connection_id);
	if(!conn) {
		shvError() << "Cannot find master broker connection for ID:" << connection_id;
		return;
	}
	std::string masters_path = std::string(cp::Rpc::DIR_BROKER) + "/" + cp::Rpc::DIR_MASTERS;
	shv::iotqt::node::ShvNode *masters_nd = m_nodesTree->cd(masters_path);
	if(!masters_nd) {
		shvError() << ".broker/masters shv directory does not exist, this should never happen";
		return;
	}
	std::string connection_path = masters_path + '/' + conn->objectName().toStdString();
	shv::iotqt::node::ShvNode *node = m_nodesTree->cd(connection_path);
	if(is_connected) {
		shvInfo() << "Logged-in to master broker, connection id:" << connection_id;
		{
			if(node) {
				shvError() << "Master broker connection dir" << connection_path << "exists already and will be deleted, this should never happen!";
				node->setParentNode(nullptr);
				delete node;
			}
			MasterBrokerShvNode *mbnd = new MasterBrokerShvNode(masters_nd);
			mbnd->setNodeId(conn->objectName().toStdString());
			/*shv::iotqt::node::RpcValueMapNode *config_nd = */
			new shv::iotqt::node::RpcValueMapNode("config", conn->options(), mbnd);
			new SubscriptionsNode(conn, mbnd);
		}
		if(conn == masterBrokerConnectionForClient(connection_id)) {
			/// propagate relative subscriptions
			propagateSubscriptionsToMasterBroker(conn);
		}
	}
	else {
		shvInfo() << "Connection to master broker lost, connection id:" << connection_id;
		if(node) {
			node->setParentNode(nullptr);
			delete node;
		}
	}
}

void BrokerApp::onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, cp::RpcValue::MetaData &&meta, std::string &&data)
{
	cp::RpcMessage::setProtocolType(meta, protocol_type);
	if(cp::RpcMessage::isRegisterRevCallerIds(meta))
		cp::RpcMessage::pushRevCallerId(meta, connection_id);
	if(cp::RpcMessage::isRequest(meta)) {
		shvMessage() << "RPC request on broker connection id:" << connection_id << "protocol type:" << shv::chainpack::Rpc::protocolTypeToString(protocol_type) << meta.toPrettyString();
		// prepare response for catch block
		// it cannot be constructed from meta, since meta is moved in the try block
		shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(meta);
		rpc::ClientConnectionOnBroker *client_connection = clientConnectionById(connection_id);
		rpc::MasterBrokerConnection *master_broker_connection = masterBrokerConnectionById(connection_id);
		rpc::CommonRpcClientHandle *connection_handle = client_connection;
		if(connection_handle == nullptr)
			connection_handle = master_broker_connection;
		try {
			std::string shv_path = cp::RpcMessage::shvPath(meta).toString();
			bool is_service_provider_mount_point_relative_call = false;
			if(connection_handle) {
				using ShvUrl = shv::core::utils::ShvUrl;
				ShvUrl shv_url(shv_path);
				if(shv_url.isServicePath()) {
					logServiceProvidersM()  << "broker id:" << brokerId()
											<< "Service path found:" << shv_path
											<< "service:" << shv_url.service().toString()
											<< "type:" << shv_url.typeString()
											<< "path part:" << shv_url.pathPart().toString();
				}
				if(client_connection) {
					if(!client_connection->isSlaveBrokerConnection()) {
						{
							// erase grant from client connections
							cp::RpcValue ag = cp::RpcMessage::accessGrant(meta);
							if(ag.isValid() /*&& !ag.isUserLogin()*/) {
								shvWarning() << "Client request with access grant specified not allowed, erasing:" << ag.toPrettyString();
								cp::RpcMessage::setAccessGrant(meta, cp::RpcValue());
							}
						}
						{
							// fill in user_id, for current client issuing rpc request
							cp::RpcValue user_id = cp::RpcRequest::userId(meta);
							cp::RpcValue::Map m = user_id.asMap();
							m[cp::Rpc::KEY_SHV_USER] = client_connection->userName();
							m[cp::Rpc::KEY_BROKER_ID] = cliOptions()->brokerId();
							user_id = m;
							cp::RpcRequest::setUserId(meta, user_id);
						}
					}
					if(shv_url.isUpTree()) {
						iotqt::node::ShvNode *service_node = nullptr;
						string resolved_local_path = client_connection->resolveLocalPath(shv_url, &service_node);
						logServiceProvidersM() << "Up-tree SP call, resolved local path:" << resolved_local_path << "service node:" << service_node;
						if(service_node) {
							logServiceProvidersM() << shv_path << "service path resolved on this broker, making path absolute:" << resolved_local_path;
							cp::RpcRequest::setShvPath(meta, resolved_local_path);
							is_service_provider_mount_point_relative_call = shv_url.isUpTreeMountPointRelative();
						}
						else {
							rpc::MasterBrokerConnection *master_broker = masterBrokerConnectionForClient(connection_id);
							logServiceProvidersM() << "service path not found, forwarding it to master broker:" << master_broker;
							if(master_broker == nullptr)
								ACCESS_EXCEPTION("Cannot call service provider path " + cp::RpcMessage::shvPath(meta).toString() + ", there is no master broker to forward the request.");
							logServiceProvidersM() << "forwarded shv path:" << resolved_local_path;
							cp::RpcRequest::setShvPath(meta, resolved_local_path);
							cp::RpcMessage::pushCallerId(meta, connection_id);
							master_broker->sendRawData(std::move(meta), std::move(data));
							return;
						}
					}
				}
				else if(master_broker_connection) {
					//shvInfo() << "request from master broker:" << shv_url.toString() << "is plain:" << shv_url.isPlain();
					if (shv::core::utils::ShvPath::startsWithPath(shv_path, shv::iotqt::node::ShvNode::LOCAL_NODE_HACK)) {
						if (cp::RpcMessage::accessGrant(meta).toString() != cp::Rpc::ROLE_ADMIN)
							ACCESS_EXCEPTION("Insufficient access rights to make call on node: " + shv::iotqt::node::ShvNode::LOCAL_NODE_HACK);
						shv::core::StringView path(shv_path);
						shv::core::utils::ShvPath::takeFirsDir(path);
						cp::RpcMessage::setShvPath(meta, path.toString());
					}
					else if(shv_url.isPlain()) {
						if (shv_path.empty() && cp::RpcMessage::method(meta) == cp::Rpc::METH_LS && cp::RpcMessage::accessGrant(meta).toString() == cp::Rpc::ROLE_ADMIN) {
							/// if superuser calls 'ls' on broker exported root, then '.local' dir is added to the ls result
							/// this enables access slave broker root via virtual '.local' directory
							meta.setValue(shv::iotqt::node::ShvNode::ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY, true);
						}
						auto path = master_broker_connection->masterExportedToLocalPath(shv_path);
						cp::RpcMessage::setShvPath(meta, path);
					}
					else if(shv_url.isDownTree()) {
						iotqt::node::ShvNode *service_node = nodeForService(shv_url);
						logServiceProvidersM() << "Down-tree SP call,  path:" << shv_url.toString() << "resolved on local broker:" << (service_node != nullptr);
						//logServiceProvidersM() << "Resolved local path:" << resolved_local_path << "service node:" << service_node;
						if(service_node) {
							string resolved_local_path = shv::core::Utils::joinPath(shv_url.service().toString(), shv_url.pathPart().toString());
							//resolved_local_path = shv::core::Utils::joinPath(resolved_local_path, shv_url.pathPart().toString());
							logServiceProvidersM() << shv_path << "service path resolved on this broker, making path absolute:" << resolved_local_path;
							cp::RpcRequest::setShvPath(meta, resolved_local_path);
						}
						else {
							string exported_path = master_broker_connection->exportedShvPath();
							//if(shv::core::String::startsWith(exported_path, "shv/"))
							//	exported_path = exported_path.substr(4);
							string resolved_path = shv::core::Utils::joinPath(exported_path, shv_url.pathPart().toString());
							resolved_path = shv::core::utils::ShvUrl::makeShvUrlString(shv_url.type(), shv_url.service(), shv_url.fullBrokerId(), resolved_path);
							logServiceProvidersM() << shv_path << "service path not resolved on this broker, preppending exported path:" << resolved_path;
							cp::RpcRequest::setShvPath(meta, resolved_path);
						}
					}
				}
				const std::string method = cp::RpcMessage::method(meta).asString();
				const std::string resolved_shv_path = cp::RpcMessage::shvPath(meta).asString();
				ShvUrl resolved_shv_url(resolved_shv_path);
				//shvError() << resolved_shv_url.toShvUrl() << "path:" << resolved_shv_url.pathPart();
				cp::AccessGrant acg;
				if(is_service_provider_mount_point_relative_call) {
					logAclResolveM() << "==== access grant for user:" << connection_handle->loggedUserName() << "requested path:" << shv_path << "method:" << method;
					acg = cp::AccessGrant(cp::Rpc::ROLE_WRITE);
					logAclResolveM() << "==== resolved path:" << resolved_shv_path << "grant:" << acg.toRpcValue().toCpon();
				}
				else {
					acg = accessGrantForRequest(connection_handle, resolved_shv_url, method, cp::RpcMessage::accessGrant(meta));
				}
				if(!acg.isValid()) {
					if(master_broker_connection)
						shvWarning() << "Acces to shv path '" + resolved_shv_path + "' not granted for master broker";
					ACCESS_EXCEPTION("Acces to shv path '" + resolved_shv_path + "' not granted for user '" + connection_handle->loggedUserName() + "'");
				}
				cp::RpcMessage::setAccessGrant(meta, acg.toRpcValue());
				cp::RpcMessage::pushCallerId(meta, connection_id);
				if(m_nodesTree->root()) {
					//shvInfo() << "REQUEST conn id:" << connection_id << meta.toPrettyString();
					m_nodesTree->root()->handleRawRpcRequest(std::move(meta), std::move(data));
				}
				else {
					SHV_EXCEPTION("Device tree root node is NULL");
				}
			}
			else {
				SHV_EXCEPTION("Connection ID: " + std::to_string(connection_id) + " doesn't exist.");
			}
		}
		catch (std::exception &e) {
			if(connection_handle) {
				rsp.setError(cp::RpcResponse::Error::create(
								 cp::RpcResponse::Error::MethodCallException
								 , e.what()));
				connection_handle->sendMessage(rsp);
			}
		}
	}
	else if(cp::RpcMessage::isResponse(meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << meta.toPrettyString();
		shv::chainpack::RpcValue orig_caller_ids = cp::RpcMessage::callerIds(meta);
		cp::RpcValue::Int caller_id = cp::RpcMessage::popCallerId(meta);
		shvDebug() << "top caller id:" << caller_id;
		if(caller_id > 0) {
			cp::TunnelCtl tctl = cp::RpcMessage::tunnelCtl(meta);
			if(tctl.state() == cp::TunnelCtl::State::FindTunnelRequest) {
				logTunnelD() << "FindTunnelRequest received:" << meta.toPrettyString();
				cp::FindTunnelReqCtl find_tunnel_request(tctl);
				//uint32_t tctl_ipv4_addr = utils::Network::toIntIPv4Address(find_tunnel_request.host());
				//bool tctl_ipv4_addr_is_public = utils::Network::isPublicIPv4Address(tctl_ipv4_addr);
				bool last_broker = cp::RpcValueGenList(cp::RpcMessage::callerIds(meta)).empty();
				bool is_public_node;
				std::string server_ip = primaryIPAddress(is_public_node);
				if(is_public_node || (last_broker && find_tunnel_request.host().empty())) {
					find_tunnel_request.setHost(server_ip);
					find_tunnel_request.setPort(tcpServer()->serverPort());
					find_tunnel_request.setCallerIds(orig_caller_ids);
					find_tunnel_request.setSecret(m_tunnelSecretList.createSecret());
				}
				if(last_broker) {
					cp::FindTunnelRespCtl find_tunnel_response = cp::FindTunnelRespCtl::fromFindTunnelRequest(find_tunnel_request);
					cp::RpcMessage msg;
					msg.setRequestId(cp::RpcMessage::requestId(meta));
					// send response to FindTunnelRequest, remove top client id, since it is this connection id
					msg.setCallerIds(cp::RpcMessage::revCallerIds(meta));
					int top_connection_id = msg.popCallerId();
					if(top_connection_id != connection_id) {
						shvError() << "(top_connection_id != connection_id) this should never happen";
						return;
					}
					msg.setTunnelCtl(find_tunnel_response);
					rpc::CommonRpcClientHandle *cch = commonClientConnectionById(connection_id);
					if(cch) {
						logTunnelD() << "Sending FindTunnelResponse:" << msg.toPrettyString();
						cch->sendMessage(msg);
					}
					return;
				}
				cp::RpcMessage::setTunnelCtl(meta, find_tunnel_request);
				logTunnelD() << "Forwarding FindTunnelRequest:" << meta.toPrettyString();
			}
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(caller_id);
			if(cch) {
				cch->sendRawData(std::move(meta), std::move(data));
			}
			else {
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
			}
		}
		else {
			// broker messages like create master broker subscription
			shvDebug() << "Got RPC response without src connection specified, it should be this broker call like create master broker subscription, throwing message away." << meta.toPrettyString();
		}
	}
	else if(cp::RpcMessage::isSignal(meta)) {
		logSigResolveD() << "SIGNAL:" << meta.toPrettyString() << "from:" << connection_id;

		const std::string sig_shv_path = cp::RpcMessage::shvPath(meta).asString();
		std::string full_shv_path = sig_shv_path;
		std::string mount_point;
		rpc::ClientConnectionOnBroker *client_connection = clientConnectionById(connection_id);
		if(client_connection) {
			/// if signal arrives from client, its path must be prepended by client mount points
			mount_point = client_connection->mountPoint();
			full_shv_path = mount_point + '/' + sig_shv_path;
		}
		if(full_shv_path.empty()) {
			shvError() << "SIGNAL with empty shv path received from master broker connection.";
		}
		else {
			//logSigResolveD() << client_connection->connectionId() << "forwarding signal to client on mount point:" << mp << "as:" << full_shv_path;
			cp::RpcMessage::setShvPath(meta, full_shv_path);
			bool sig_sent = sendNotifyToSubscribers(meta, data);
			if(!sig_sent && client_connection && client_connection->isSlaveBrokerConnection()) {
				logSubscriptionsD() << "Rejecting unsubscribed signal, shv_path:" << full_shv_path << "method:" << cp::RpcMessage::method(meta).asString();
				cp::RpcRequest rq;
				rq.setRequestId(client_connection->nextRequestId());
				rq.setMethod(cp::Rpc::METH_REJECT_NOT_SUBSCRIBED)
						.setParams(cp::RpcValue::Map{
									   { cp::Rpc::PAR_PATH, sig_shv_path},
									   { cp::Rpc::PAR_METHOD, cp::RpcMessage::method(meta).toString()}})
						.setShvPath(cp::Rpc::DIR_BROKER_APP);
				client_connection->sendMessage(rq);
			}
		}
	}
}

rpc::MasterBrokerConnection *BrokerApp::masterBrokerConnectionForClient(int client_id)
{
	Q_UNUSED(client_id)
	//TODO: find master broker connection according to client mount path
	return masterBrokerConnections().value(0);
}

void BrokerApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		shv::chainpack::RpcValue::Int connection_id = resp.popCallerId();
		rpc::CommonRpcClientHandle *conn = commonClientConnectionById(connection_id);
		if(conn)
			conn->sendMessage(resp);
		else
			shvError() << "Cannot find connection for ID:" << connection_id;
		return;
	}
	else if(msg.isSignal()) {
		cp::RpcSignal sig(msg);
		sendNotifyToSubscribers(sig.shvPath().asString(), sig.method().asString(), sig.params());
	}
	else {
		shvError() << "Send message not implemented.";// << msg.toCpon();
	}
}

void BrokerApp::onClientConnected(int client_id)
{
	rpc::ClientConnectionOnBroker *cc = clientConnectionById(client_id);
	if(cc && cc->isSlaveBrokerConnection()) {
		/// if slave broker is connected, forward subscriptions of connected clients
		for(rpc::CommonRpcClientHandle *ch : allClientConnections()) {
			if(ch->connectionId() == client_id)
				continue;
			for(size_t i=0; i<ch->subscriptionCount(); i++) {
				const rpc::CommonRpcClientHandle::Subscription &subs = ch->subscriptionAt(i);
				cc->propagateSubscriptionToSlaveBroker(subs);
			}
		}
	}
}

std::string BrokerApp::brokerClientDirPath(int client_id)
{
	return std::string(cp::Rpc::DIR_BROKER) + "/clients/" + std::to_string(client_id);
}

std::string BrokerApp::brokerClientAppPath(int client_id)
{
	return brokerClientDirPath(client_id) + "/app";
}

bool BrokerApp::sendNotifyToSubscribers(const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data)
{
	// send it to all clients for now
	bool subs_sent = false;
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			const cp::RpcValue shv_path = cp::RpcMessage::shvPath(meta_data);
			const cp::RpcValue method = cp::RpcMessage::method(meta_data);
			int subs_ix = conn->isSubscribed(shv_path.toString(), method.asString());
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ClientConnectionOnBroker::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path.asString());
				if(new_path == shv_path.asString()) {
					conn->sendRawData(meta_data, std::string(data));
				}
				else {
					shv::chainpack::RpcValue::MetaData md2(meta_data);
					cp::RpcMessage::setShvPath(md2, new_path);
					conn->sendRawData(md2, std::string(data));
				}
				subs_sent = true;
			}
		}
	}
	return subs_sent;
}

void BrokerApp::sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	//shvWarning() << shv_path << method << params.toPrettyString();
	cp::RpcSignal sig;
	sig.setShvPath(shv_path);
	sig.setMethod(method);
	sig.setParams(params);
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			int subs_ix = conn->isSubscribed(shv_path, method);
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ClientConnectionOnBroker::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path);
				if(new_path != shv_path)
					sig.setShvPath(new_path);
				conn->sendMessage(sig);
			}
		}
	}
}

void BrokerApp::addSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	//using ServiceProviderPath = shv::core::utils::ServiceProviderPath;
	rpc::CommonRpcClientHandle *connection_handle = commonClientConnectionById(client_id);
	if(!connection_handle)
		SHV_EXCEPTION("Cannot create subscription, invalid connection ID.");
	rpc::CommonRpcClientHandle::Subscription subs = connection_handle->createSubscription(shv_path, method);
	connection_handle->addSubscription(subs);
	//rpc::ClientConnection *cli = dynamic_cast<rpc::ClientConnection*>(connection_handle);
	shv::core::utils::ShvUrl spp(subs.localPath);
	//logSubscriptionsD() << "addSubscription path:" << subs.localPath << "method:" << subs.method << "for client:" << cli;
	if(spp.isServicePath()) {
		/// not served here, should be propagated to the master broker
		rpc::MasterBrokerConnection *mbrconn = masterBrokerConnectionForClient(client_id);
		if(mbrconn) {
			mbrconn->callMethodSubscribe(subs.localPath, method);
		}
		else {
			shvError() << "Cannot propagate relative path subscription, without master broker connected:" << subs.localPath;
		}
	}
	else {
		/// check slave broker connections
		/// whether this subsciption should be propagated to them
		/// skip service providers subscriptions, since it does not make ense to send them downstream
		for (int connection_id : clientConnectionIds()) {
			rpc::ClientConnectionOnBroker *conn = clientConnectionById(connection_id);
			if(conn->isSlaveBrokerConnection()) {
				conn->propagateSubscriptionToSlaveBroker(subs);
			}
		}
	}
}

bool BrokerApp::removeSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	rpc::CommonRpcClientHandle *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot remove subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	rpc::CommonRpcClientHandle::Subscription subs(string(), shv_path, method);
	return conn->removeSubscription(subs);
}

bool BrokerApp::rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method)
{
	logSubscriptionsD() << "signal rejected, shv_path:" << path << "method:" << method;
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(client_id);
	if(conn) {
		return conn->rejectNotSubscribedSignal(conn->masterExportedToLocalPath(path), method);
	}
	return false;
}

void BrokerApp::createMasterBrokerConnections()
{
	if(!cliOptions()->isMasterBrokersEnabled())
		return;
	shvInfo() << "Creating master broker connections";
	shv::chainpack::RpcValue masters = cliOptions()->masterBrokersConnections();

	for(const auto &kv : masters.asMap()) {
		cp::RpcValue::Map opts = kv.second.asMap();

		//if (cliOptions()->masterBrokerDeviceId_isset()) {
		//	cp::RpcValue::Map dev = opts.value("device").toMap();
		//	dev.setValue("id", cliOptions()->masterBrokerDeviceId());
		//	opts.setValue("device", dev);
		//}
		shvInfo() << "master broker device ID:" << opts.value("device").toPrettyString() << "for connection:" << kv.first;

		if(opts.value("enabled").toBool() == false)
			continue;

		shvInfo() << "creating master broker connection:" << kv.first;
		rpc::MasterBrokerConnection *bc = new rpc::MasterBrokerConnection(this);
		bc->setObjectName(QString::fromStdString(kv.first));
		int id = bc->connectionId();
		connect(bc, &rpc::MasterBrokerConnection::brokerConnectedChanged, this, [id, this](bool is_connected) {
			this->onConnectedToMasterBrokerChanged(id, is_connected);
		});
		bc->setOptions(opts);
		bc->open();
	}
}

QList<rpc::MasterBrokerConnection *> BrokerApp::masterBrokerConnections() const
{
	return findChildren<rpc::MasterBrokerConnection*>(QString(), Qt::FindDirectChildrenOnly);
}

rpc::MasterBrokerConnection *BrokerApp::masterBrokerConnectionById(int connection_id)
{
	for(rpc::MasterBrokerConnection *bc : masterBrokerConnections()) {
		if(bc->connectionId() == connection_id)
			return bc;
	}
	return nullptr;
}

std::vector<rpc::CommonRpcClientHandle *> BrokerApp::allClientConnections()
{
	std::vector<rpc::CommonRpcClientHandle *> ret;
	for (int i : clientConnectionIds())
		ret.push_back(clientConnectionById(i));
	QList<rpc::MasterBrokerConnection *> mbc = masterBrokerConnections();
	std::copy(mbc.begin(), mbc.end(), std::back_inserter(ret));
	return ret;
}

rpc::CommonRpcClientHandle *BrokerApp::commonClientConnectionById(int connection_id)
{
	rpc::CommonRpcClientHandle *ret = clientConnectionById(connection_id);
	if(ret)
		return ret;
	ret = masterBrokerConnectionById(connection_id);
	return ret;
}

QSqlDatabase BrokerApp::sqlConfigConnection()
{
	if(!cliOptions()->isSqlConfigEnabled())
		return QSqlDatabase();
	QSqlDatabase db = QSqlDatabase::database(SQL_CONFIG_CONN_NAME, false);
	return db;
}

AclManager *BrokerApp::aclManager()
{
	if(m_aclManager == nullptr)
		setAclManager(createAclManager());
	return m_aclManager;
}

void BrokerApp::setAclManager(AclManager *mng)
{
	m_aclManager = mng;
}


}}
