#include "aclmanagersqlite.h"
#include "brokeraclnode.h"
#include "brokerappnode.h"
#include "brokerrootnode.h"
#include "clientshvnode.h"
#include "rpc/brokertcpserver.h"
#include "rpc/clientconnectiononbroker.h"
#include "rpc/masterbrokerconnection.h"
#include "subscriptionsnode.h"
#include <shv/broker/brokerapp.h>
#include <shv/broker/currentclientshvnode.h>
#include <shv/broker/clientconnectionnode.h>

#ifdef WITH_SHV_WEBSOCKETS
#include "rpc/websocketserver.h"
#endif

#ifdef WITH_SHV_LDAP
#include "openldap_dynamic.h"
#include <shv/broker/ldap/ldap.h>
#include <dlfcn.h>
#endif

#include <shv/iotqt/utils/network.h>
#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/utils/shvpath.h>

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

#include <QDir>
#include <QFile>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFuture>

#include <ctime>
//#include <fstream>

#ifdef Q_OS_UNIX
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define logTunnelD() nCDebug("Tunnel")

#define logBrokerDiscoveryW() nCWarning("BrokerDiscovery")
#define logBrokerDiscoveryM() nCMessage("BrokerDiscovery")
#define logServiceProvidersM() nCMessage("ServiceProviders")

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSigResolveD() nCDebug("SigRes").color(NecroLog::Color::LightGreen)

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

namespace cp = shv::chainpack;

using namespace std;

namespace shv::broker {

#ifdef Q_OS_UNIX
namespace {
#ifdef WITH_SHV_LDAP
const auto LDAP_LIBNAME = "libldap.so.2";
#endif

const auto sig_term_fd = [] {
	std::array<int, 2> fd;
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, fd.data())) {
		qFatal("Couldn't create SIG_TERM socketpair");
	}
	return fd;
}();
}
#endif

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods {
			shv::chainpack::methods::DIR,
			shv::chainpack::methods::LS,
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
			shv::chainpack::methods::DIR,
				shv::chainpack::methods::LS,
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

		if(shv_path.size() == 1) {
			return StringList();
		}

		return Super::childNames(shv_path);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override
	{
		if(shv_path.size() == 1) {
			if(method == METH_CLIENT_IDS) {
				BrokerApp *app = BrokerApp::instance();
				std::string path = std::string{shv::core::utils::slice(shv_path.at(0), 1, -1)};
				auto *nd1 = app->m_nodesTree->cd(path);
				if(nd1 == nullptr)
					SHV_EXCEPTION("Cannot find node on path: " + path);
				auto *nd = qobject_cast<ClientShvNode*>(nd1);
				if(nd == nullptr)
					SHV_EXCEPTION("Wrong node type on path: " + path + ", looking for ClientShvNode, found: " + nd1->metaObject()->className());
				cp::RpcList lst;
				for(rpc::ClientConnectionOnBroker *conn : nd->connections())
					lst.push_back(conn->connectionId());
				return cp::RpcValue{lst};
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}
private:
	static constexpr auto METH_CLIENT_IDS = "clientIds";
	static const std::vector<cp::MetaMethod> m_metaMethods;
};

const std::vector<cp::MetaMethod> MountsNode::m_metaMethods = {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{METH_CLIENT_IDS, cp::MetaMethod::Flag::IsGetter, {}, "List", shv::chainpack::AccessLevel::Config},
};

#ifdef WITH_SHV_LDAP
std::optional<LdapConfig> BrokerApp::ldapConfig()
{
	return m_ldapConfig;
}


namespace {
void check_open_success(auto* ptr, auto name)
{
	if (!ptr) {
		throw std::runtime_error(std::string{"LDAP configured, but "} + name + " couldn't be opened: " + dlerror());
	}
}

void do_open_function(const auto& ldap_lib, auto fn_name, auto& fn_field)
{
	auto fn = dlsym(ldap_lib.get(), fn_name);
	check_open_success(fn, fn_name);
	fn_field = reinterpret_cast<decltype(fn_field)>(fn);
}
}
#endif

std::optional<AzureConfig> BrokerApp::azureConfig()
{
	return m_azureConfig;
}

namespace {
auto transform_cli_group_mapping(const chainpack::RpcList& cli_group_mapping)
{
	std::vector<GroupMapping> group_mapping;
	std::transform(cli_group_mapping.begin(),
				   cli_group_mapping.end(),
				   std::back_inserter(group_mapping),
				   [] (const auto& mapping) {
					   auto mapping_list = mapping.asList();
					   if (mapping_list.size() != 2) {
#ifdef _MSC_VER
						   SHV_EXCEPTION(__FUNCSIG__ + std::string{": group mapping isn't a pair."});
#else
						   SHV_EXCEPTION(__PRETTY_FUNCTION__ + std::string{": group mapping isn't a pair."});
#endif
					   }
					   return GroupMapping {
						   .nativeGroup = mapping.asList().at(0).asString(),
						   .shvGroup = mapping.asList().at(1).asString()
					   };
				   });

	return group_mapping;
}

}

static const auto SQL_CONFIG_CONN_NAME = QStringLiteral("ShvBrokerDbConfigSqlConnection");
BrokerApp::BrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	#ifdef WITH_SHV_LDAP
	, m_ldapLib(nullptr, nullptr)
	#endif
{
	m_brokerId = m_cliOptions->brokerId();
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
#ifdef Q_OS_UNIX
	installUnixSignalHandlers();
#endif
	m_nodesTree = new shv::iotqt::node::ShvNodeTree(new BrokerRootNode(), this);
	connect(m_nodesTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, this, &BrokerApp::onRootNodeSendRpcMesage);
	auto *bn = new BrokerAppNode();
	m_nodesTree->mount(cp::Rpc::DIR_BROKER_APP, bn);
	m_nodesTree->mount(BROKER_CURRENT_CLIENT_SHV_PATH, new CurrentClientShvNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/clients", new ClientsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/masters", new MasterBrokersNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/mounts", new MountsNode());
	auto etc_acl_root_node = new EtcAclRootNode();
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/etc/acl", etc_acl_root_node);

	if (m_cliOptions->discoveryPort() > 0) {
		auto *udp_socket = new QUdpSocket(this);
		udp_socket->bind(static_cast<quint16>(m_cliOptions->discoveryPort()), QUdpSocket::ShareAddress);
		logBrokerDiscoveryM() << "shvbrokerDiscovery listen on UDP port:" << m_cliOptions->discoveryPort();
		connect(udp_socket, &QUdpSocket::readyRead, this, [this, udp_socket]() {
			try {
				QByteArray datagram;
				QHostAddress address;
				quint16 port;
				while (udp_socket->hasPendingDatagrams()) {
					datagram.resize(int(udp_socket->pendingDatagramSize()));
					udp_socket->readDatagram(datagram.data(), datagram.size(), &address, &port);

					std::string rq_cpon(datagram.constData(), static_cast<size_t>(datagram.length()));
					shv::chainpack::RpcValue rv = shv::chainpack::RpcValue::fromCpon(rq_cpon);
					shv::chainpack::RpcRequest rq(rv);
					if (rq.method() == "shvbrokerDiscovery") {
						logBrokerDiscoveryM() << "Received broadcast request shvbrokerDiscovery:" << rq.toPrettyString();
						shv::chainpack::RpcResponse resp = rq.makeResponse();
						QString ipv4 = shv::iotqt::utils::Network::primaryIPv4Address().toString();
						shv::chainpack::RpcValue response = shv::chainpack::RpcValue::Map { {"brokerId", m_brokerId}, {"brokerIPv4", ipv4.toStdString()}, {"brokerPort", m_cliOptions->serverPort() } };
						resp.setResult(response);
						QByteArray response_datagram(resp.toCpon().c_str(), static_cast<int>(resp.toCpon().length()));
						udp_socket->writeDatagram(response_datagram, address, port);
						logBrokerDiscoveryM() << "Send response on broadcast shvbrokerDiscovery:" << resp.toPrettyString();
					}
				}
			}
			catch (const std::exception &e) {
				logBrokerDiscoveryW() << "BrokerDiscovery error:" << e.what();
			}
		});
	}

	if (cli_opts->ldapHostname_isset()) {
#ifdef WITH_SHV_LDAP
		m_ldapLib = {dlopen(LDAP_LIBNAME, RTLD_LAZY), [] (void* handle) {
			if (dlclose(handle)) {
				shvError() << "Couldn't close OpenLDAP library:" << dlerror();
			}
		}};
		check_open_success(m_ldapLib.get(), LDAP_LIBNAME);

#define open_function(FN_NAME) do_open_function(m_ldapLib, #FN_NAME, ldap::OpenLDAP::FN_NAME)
		open_function(ber_free);
		open_function(ldap_connect);
		open_function(ldap_destroy);
		open_function(ldap_err2string);
		open_function(ldap_first_attribute);
		open_function(ldap_first_entry);
		open_function(ldap_get_values_len);
		open_function(ldap_initialize);
		open_function(ldap_msgfree);
		open_function(ldap_next_attribute);
		open_function(ldap_next_entry);
		open_function(ldap_sasl_bind_s);
		open_function(ldap_search_ext_s);
		open_function(ldap_set_option);
		open_function(ldap_set_rebind_proc);
		open_function(ldap_value_free_len);

		shvInfo() << "Enabling LDAP authentication";

		if (!cli_opts->ldapSearchBaseDN_isset()) {
			SHV_EXCEPTION("LDAP searchBaseDN not set");
		}

		if (!cli_opts->ldapSearchAttrs_isset()) {
			SHV_EXCEPTION("LDAP searchAttrs not set");
		}

		if (!cli_opts->ldapGroupMapping_isset()) {
			SHV_EXCEPTION("LDAP groupMapping not set");
		}

		auto cli_search_attrs = cli_opts->ldapSearchAttrs();
		std::vector<std::string> search_attrs;
		std::transform(cli_search_attrs.begin(),
					   cli_search_attrs.end(),
					   std::back_inserter(search_attrs),
					   [] (const auto& attr) {
						   return attr.asString();
					   });

		m_ldapConfig = LdapConfig {
			.brokerUsername = cli_opts->ldapUsername_isset() ? std::optional(cli_opts->ldapUsername()) : std::nullopt,
			.brokerPassword = cli_opts->ldapPassword_isset() ? std::optional(cli_opts->ldapPassword()) : std::nullopt,
			.hostName = cli_opts->ldapHostname(),
			.searchBaseDN = cli_opts->ldapSearchBaseDN(),
			.searchAttrs = search_attrs,
			.groupMapping = transform_cli_group_mapping(cli_opts->ldapGroupMapping())
		};
		new LdapAclNode(*m_ldapConfig, etc_acl_root_node);
#else
		shvWarning() << "LDAP has been configured, but this broker wasn't compiled with LDAP support. LDAP will NOT be enabled.";
#endif
	}
	if (cli_opts->azureGroupMapping_isset()) {
		m_azureConfig = AzureConfig {
			.groupMapping = transform_cli_group_mapping(cli_opts->azureGroupMapping())
		};
	}

	QTimer::singleShot(0, this, &BrokerApp::lazyInit);
}

BrokerApp::~BrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
}

AppCliOptions* BrokerApp::cliOptions()
{
	return m_cliOptions;
}

BrokerApp* BrokerApp::instance()
{
	return qobject_cast<BrokerApp*>(Super::instance());
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
	m_snTerm = new QSocketNotifier(sig_term_fd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &BrokerApp::handlePosixSignals);
	shvInfo() << "SIG_TERM handler installed OK";
}

void BrokerApp::nativeSigHandler(int sig_number)
{
	shvInfo() << "SIG:" << sig_number;
	auto a = static_cast<unsigned char>(sig_number);
	if (auto err = ::write(sig_term_fd[0], &a, sizeof(a)); err == -1) {
		shvWarning() << "Unable to write into the signal handler pipe:" << strerror(errno);
	}
}

void BrokerApp::handlePosixSignals()
{
	m_snTerm->setEnabled(false);
	unsigned char sig_num;
	if (auto err = ::read(sig_term_fd[1], &sig_num, sizeof(sig_num)); err == -1) {
		shvWarning() << "Unable to read from the signal handler pipe:" << strerror(errno);
		return;
	}

	shvInfo() << "SIG" << sig_num << "catched.";
	if(sig_num == SIGTERM || sig_num == SIGHUP) {
		QTimer::singleShot(0, this, &BrokerApp::quit);
	}
	else if(sig_num == SIGUSR1) {
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
			shvMessage() << "TCP server port is not set, it will not be started.";
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
			shvMessage() << "SSL server port is not set, it will not be started.";
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
			shvInfo() << "Starting ws server on port" << port;
			if(!m_webSocketServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket server!");
			}
		}
	}
	else {
		shvMessage() << "Websocket server port is not set, it will not be started.";
	}
	if(opts->serverWebsocketSslPort_isset()) {
		SHV_SAFE_DELETE(m_webSocketSslServer);
		int port = opts->serverWebsocketSslPort();
		if(port > 0) {
			m_webSocketSslServer = new rpc::WebSocketServer(QWebSocketServer::SecureMode, this);
			shvInfo() << "Starting wss server on port" << port;
			if(!m_webSocketSslServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket SSL server!");
			}
		}
	}
	else {
		shvMessage() << "Websocket SSL server port is not set, it will not be started.";
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

#ifdef WITH_SHV_LDAP
class LdapAuthThread : public QThread {
	Q_OBJECT
public:
	LdapAuthThread(const chainpack::UserLoginContext& ctx, const LdapConfig& ldap_config)
		: m_ctx(ctx)
		, m_ldapConfig(ldap_config)
	{
	}
	void run() override
	{
		auto login_details = m_ctx.userLogin();
		auto user_name = "ldap:" + login_details.user;
		try {
			auto ldap = ldap::Ldap::create(m_ldapConfig.hostName);
			ldap->setVersion(ldap::Version::Version3);
			ldap->connect();
			ldap->bindSasl(login_details.user, login_details.password);
			auto ldap_groups = ldap::getGroupsForUser(ldap, m_ldapConfig.searchBaseDN, m_ldapConfig.searchAttrs, login_details.user);
			std::vector<std::string> res_shv_groups{user_name};
			if (auto it = std::find_if(m_ldapConfig.groupMapping.begin(), m_ldapConfig.groupMapping.end(), [&ldap_groups] (const auto& mapping) {
				return std::find_if(ldap_groups.begin(), ldap_groups.end(), [&mapping] (const auto& ldap_group) {
					return mapping.nativeGroup == ldap_group;
				}) != ldap_groups.end();
			}); it != m_ldapConfig.groupMapping.end()) {
				res_shv_groups.push_back(it->shvGroup);
			}

			auto result = chainpack::UserLoginResult{};
			if (res_shv_groups.size() == 1) {
				// If the user doesn't have any group other than the
				// implicit (their username) one, than we'll reject the
				// login.
				result.passwordOk = false;
				result.loginError = "No relevant LDAP groups found.";
			} else {
				result.passwordOk = true;
			}
			result.userNameOverride = user_name;
			emit resultReady(result, user_name, res_shv_groups);
		} catch(ldap::LdapError& err) {
			// FIXME: If the LDAP host is set, the user probably depends on ldap, so I can probably just emit
			// and take what LDAP says for granted.
			emit resultReady(chainpack::UserLoginResult{false, err.what()}, user_name, {});
		}
	}

	Q_SIGNAL void resultReady(const chainpack::UserLoginResult& s, std::string user_name, const std::vector<std::string>& shv_groups);

private:
	chainpack::UserLoginContext m_ctx;
	LdapConfig m_ldapConfig;
};
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
namespace azure {
namespace {
[[noreturn]] void throw_with_msg(const std::string& msg)
{
	throw std::runtime_error{"Azure authorization failed: " + msg};
}
}
}

class AzureAuth : public QObject {
	Q_OBJECT
public:
	AzureAuth(const chainpack::UserLoginContext& ctx, const AzureConfig& azure_config)
		: m_azureConfig(azure_config)
		, m_ctx(ctx)
		, m_manager(new QNetworkAccessManager(this))
	{
		do_request(QUrl{"https://graph.microsoft.com/v1.0/me"}).then(this, [this] (const QJsonDocument& json) {
			if (!json.object().contains("mail")) {
				azure::throw_with_msg("Couldn't fetch user e-mail");
			}

			m_username = "azure:" + json.object()["mail"].toString().toStdString();
			return do_request(QUrl{"https://graph.microsoft.com/v1.0/me/transitiveMemberOf"});
		}).unwrap().then(this, [this] (const QJsonDocument& json) {
			std::vector<std::string> res_shv_groups{m_username};
			if (!json.object().contains("value")) {
				azure::throw_with_msg("Couldn't fetch user groups");
			}
			for (const auto& group : json.object()["value"].toArray()) {
				if (group.toObject()["@odata.type"].toString() == "#microsoft.graph.group") {
					auto group_id = group.toObject()["id"].toString().toStdString();
					if (auto it = std::find_if(m_azureConfig.groupMapping.begin(), m_azureConfig.groupMapping.end(), [group_id] (const auto& mapping) {
						return mapping.nativeGroup == group_id;
					}); it != m_azureConfig.groupMapping.end()) {
						res_shv_groups.emplace_back(it->shvGroup);
					}
				}
			}

			auto result = chainpack::UserLoginResult{};
			if (res_shv_groups.size() == 1) {
				// If the user doesn't have any group other than the
				// implicit (their username) one, than we'll reject the
				// login.
				result.passwordOk = false;
				result.loginError = "No relevant Azure groups found.";
			} else {
				result.passwordOk = true;
			}
			result.userNameOverride = m_username;
			emit resultReady(result, m_username, res_shv_groups);
		}).onFailed([this] (const std::runtime_error& ex) {
			auto result = chainpack::UserLoginResult{false, ex.what()};
			result.userNameOverride = m_username;
			emit resultReady(result, m_username, {});
		});

	}

	Q_SIGNAL void resultReady(const chainpack::UserLoginResult& s, std::string user_name, const std::vector<std::string>& shv_groups);

private:
	QFuture<QJsonDocument> do_request(auto url)
	{
		QNetworkRequest request;
		request.setRawHeader("Authorization", QByteArray("Bearer ") + QByteArray::fromStdString(m_ctx.userLogin().password));
		request.setUrl(url);
		auto reply = m_manager->get(request);
		return QtFuture::connect(reply, &QNetworkReply::finished).then([reply]() {
			reply->deleteLater();
			if (reply->error() != QNetworkReply::NoError) {
				azure::throw_with_msg(reply->errorString().toStdString());
			}

			const auto json = reply->readAll();
			return QJsonDocument::fromJson(json);
		});
	}

	AzureConfig m_azureConfig;
	chainpack::UserLoginContext m_ctx;
	std::string m_username;
	QNetworkAccessManager* m_manager;
};
#endif

void BrokerApp::checkLogin(const chainpack::UserLoginContext &ctx, const QObject* connection_ctx, const std::function<void(chainpack::UserLoginResult)> cb)
{
	if (ctx.userLogin().loginType == chainpack::UserLogin::LoginType::AzureAccessToken && m_azureConfig) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
		auto azure_auth = new AzureAuth(ctx, *m_azureConfig);
		connect(azure_auth, &AzureAuth::resultReady, connection_ctx, [cb, azure_auth] (const auto& azure_result, const auto& user_name, const auto& shv_groups) {
			BrokerApp::instance()->aclManager()->setGroupForAzureUser(user_name, shv_groups);
			cb(azure_result);
			azure_auth->deleteLater();
		});
#else
		shvError() << "Client tried to connect via Azure, but Azure is not supported in this Qt5 build.";
#endif
		return;
	}
	auto result = BrokerApp::instance()->aclManager()->checkPassword(ctx);
	// If the user exists in the ACL manager, we'll take the result as decisive.
	if (aclManager()->user(ctx.userLogin().user).isValid()) {
		cb(result);
		return;
	}

#ifdef WITH_SHV_LDAP
	if (m_ldapConfig) {
		if (ctx.userLogin().loginType == chainpack::IRpcConnection::LoginType::Plain) {
				auto auth_thread = new LdapAuthThread(ctx, *m_ldapConfig);
				connect(auth_thread, &LdapAuthThread::resultReady, connection_ctx, [cb] (const auto& ldap_result, const auto& user_name, const auto& shv_groups) {
					BrokerApp::instance()->aclManager()->setGroupForLdapUser(user_name, shv_groups);
					cb(ldap_result);
				});
				connect(auth_thread, &LdapAuthThread::finished, auth_thread, &QObject::deleteLater);
				auth_thread->start();
				return;
		}
		result.loginError += " Authentication over LDAP requires sending a plaintext password. Please use an SSL connection.";
	}
#else
	(void)connection_ctx;
#endif
	cb(result);
}

void BrokerApp::sendNewLogEntryNotify(const std::string &msg)
{
	if(isSendLogAsSignalEnabled())
		sendNotifyToSubscribers(".broker/app/log", cp::Rpc::SIG_VAL_CHANGED, cp::Rpc::METH_GET, msg);
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
		if (!QFile(qfn).exists()) {
			auto last_slash = qfn.lastIndexOf('/');
			QDir("/").mkpath(qfn.mid(0, last_slash));
		}
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
		auto *ret = new AclManagerSqlite(this);
		ret->init(); // we cannot call virtual functions from constructor
		return ret;
	}
	return new AclManagerConfigFiles(this);
}

const std::string& BrokerApp::brokerId() const
{
	return m_brokerId;
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
		if(!path.empty() && !(path[0] == "test")) {
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
	if(cliOptions()->publicIP_isset()) {
		is_public = true;
		return cliOptions()->publicIP();
	}

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

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::ClientConnectionOnBroker *conn = clientConnectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));

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
		auto *client_id_node = new ClientConnectionNode(connection_id, clients_nd);
		auto *client_app_node = new ClientShvNode("app", conn, client_id_node);
		// delete whole client tree, when client is destroyed
		connect(conn, &rpc::ClientConnectionOnBroker::destroyed, client_id_node, &ClientShvNode::deleteLater);

		conn->setParent(client_app_node);
		{
			std::string mount_point = client_id_node->shvPath() + "/subscriptions";
			auto *nd = new SubscriptionsNode(conn);
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
			auto *cli_nd = qobject_cast<ClientShvNode*>(m_nodesTree->cd(mount_point, &path_rest));
			if(cli_nd) {
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
				sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, "", true);
			});
			shvInfo() << "client connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->setMountPoint(mount_point);
			connect(cli_nd, &ClientShvNode::destroyed, this, [this, mount_point]() {
				shvInfo() << "mounted node destroyed:" << mount_point;
				sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, "", false);
			});
			QTimer::singleShot(0, this, [this, connection_id]() {
				onClientConnected(connection_id);
			});
		}
	}
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
			auto *mbnd = new MasterBrokerShvNode(masters_nd);
			mbnd->setNodeId(conn->objectName().toStdString());
			new shv::iotqt::node::RpcValueMapNode("config", conn->options(), mbnd);
			new SubscriptionsNode(conn, mbnd);
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

void BrokerApp::onRpcFrameReceived(int connection_id, shv::chainpack::RpcFrame &&frame)
{
	if(cp::RpcMessage::isRegisterRevCallerIds(frame.meta))
		cp::RpcMessage::pushRevCallerId(frame.meta, connection_id);
	if(cp::RpcMessage::isRequest(frame.meta)) {
		shvMessage() << "RPC request on broker connection id:" << connection_id << frame.meta.toPrettyString();
		// prepare response for catch block
		// it cannot be constructed from meta, since meta is moved in the try block
		shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(frame.meta);
		rpc::ClientConnectionOnBroker *client_connection = clientConnectionById(connection_id);
		rpc::MasterBrokerConnection *master_broker_connection = masterBrokerConnectionById(connection_id);
		rpc::CommonRpcClientHandle *connection_handle = client_connection;
		if(connection_handle == nullptr)
			connection_handle = master_broker_connection;
		try {
			std::string shv_path = cp::RpcMessage::shvPath(frame.meta).toString();
			if(connection_handle) {
				if(client_connection) {
					if(!client_connection->isSlaveBrokerConnection()) {
						{
							// erase grant from client connections
							auto ag = cp::RpcMessage::accessGrant(frame.meta);
							if(ag.accessLevel > chainpack::AccessLevel::None) {
								shvWarning() << "Client request with access grant specified not allowed, erasing:" << ag.toPrettyString();
								cp::RpcMessage::setAccessGrant(frame.meta, {});
							}
						}
						{
							// fill in user_id, for current client issuing rpc request
							cp::RpcValue user_id = cp::RpcRequest::userId(frame.meta);
							cp::RpcValue::Map m = user_id.asMap();
							m[cp::Rpc::KEY_SHV_USER] = client_connection->userName();
							m[cp::Rpc::KEY_BROKER_ID] = cliOptions()->brokerId();
							user_id = m;
							cp::RpcRequest::setUserId(frame.meta, user_id);
						}
					}
				}
				else if(master_broker_connection) {
					auto has_dot_local_access = [](const cp::RpcValue::MetaData &meta_data) {
						auto access_grant = cp::RpcMessage::accessGrant(meta_data);
						if (access_grant.accessLevel < chainpack::AccessLevel::SuperService) {
							auto roles = shv::core::utils::split(access_grant.access, ',');
							return std::find(roles.begin(), roles.end(), "dot_local") != roles.end();
						}
						return true;
					};
					if (shv::core::utils::ShvPath::startsWithPath(shv_path, shv::iotqt::node::ShvNode::LOCAL_NODE_HACK)) {
						if(has_dot_local_access(frame.meta)) {
							shv::core::StringView path(shv_path);
							shv::core::utils::ShvPath::takeFirsDir(path);
							cp::RpcMessage::setShvPath(frame.meta, std::string{path});
						}
						else {
							ACCESS_EXCEPTION("Insufficient access rights to make call on node: " + shv::iotqt::node::ShvNode::LOCAL_NODE_HACK);
						}
					}
					else {
						if (shv_path.empty() && cp::RpcMessage::method(frame.meta) == cp::Rpc::METH_LS && has_dot_local_access(frame.meta)) {
							/// if superuser calls 'ls' on broker exported root, then '.local' dir is added to the ls result
							/// this enables access slave broker root via virtual '.local' directory
							frame.meta.setValue(shv::iotqt::node::ShvNode::ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY, true);
						}
						auto path = master_broker_connection->masterExportedToLocalPath(shv_path);
						cp::RpcMessage::setShvPath(frame.meta, path);
					}
				}
				const std::string method = cp::RpcMessage::method(frame.meta).asString();
				auto acg = aclManager()->accessGrantForShvPath(connection_handle->loggedUserName(), shv_path, method, connection_handle->isMasterBrokerConnection(), cp::RpcMessage::accessGrant(frame.meta));
				if(acg.accessLevel > shv::chainpack::AccessLevel::None) {
					if(acg.accessLevel < shv::chainpack::AccessLevel::Write) {
						// remove iser id for read operations
						cp::RpcMessage::setUserId(frame.meta, {});
					}
				}
				else {
					if(master_broker_connection)
						shvWarning() << "Acces to shv path '" + shv_path + "' not granted for master broker";
					ACCESS_EXCEPTION("Acces to shv path '" + shv_path + "' not granted for user '" + connection_handle->loggedUserName() + "'");
				}
				cp::RpcMessage::setAccessGrant(frame.meta, acg);
				cp::RpcMessage::pushCallerId(frame.meta, connection_id);
				if(m_nodesTree->root()) {
					m_nodesTree->root()->handleRpcFrame(std::move(frame));
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
				connection_handle->sendRpcMessage(rsp);
			}
		}
	}
	else if(cp::RpcMessage::isResponse(frame.meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << frame.meta.toPrettyString();
		shv::chainpack::RpcValue orig_caller_ids = cp::RpcMessage::callerIds(frame.meta);
		cp::RpcValue::Int caller_id = cp::RpcMessage::popCallerId(frame.meta);
		shvDebug() << "top caller id:" << caller_id;
		if(caller_id > 0) {
			cp::TunnelCtl tctl = cp::RpcMessage::tunnelCtl(frame.meta);
			if(tctl.state() == cp::TunnelCtl::State::FindTunnelRequest) {
				logTunnelD() << "FindTunnelRequest received:" << frame.meta.toPrettyString();
				cp::FindTunnelReqCtl find_tunnel_request(tctl);
				bool last_broker = cp::RpcValueGenList(cp::RpcMessage::callerIds(frame.meta)).empty();
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
					msg.setRequestId(cp::RpcMessage::requestId(frame.meta));
					// send response to FindTunnelRequest, remove top client id, since it is this connection id
					msg.setCallerIds(cp::RpcMessage::revCallerIds(frame.meta));
					int top_connection_id = msg.popCallerId();
					if(top_connection_id != connection_id) {
						shvError() << "(top_connection_id != connection_id) this should never happen";
						return;
					}
					msg.setTunnelCtl(find_tunnel_response);
					rpc::CommonRpcClientHandle *cch = commonClientConnectionById(connection_id);
					if(cch) {
						logTunnelD() << "Sending FindTunnelResponse:" << msg.toPrettyString();
						cch->sendRpcMessage(msg);
					}
					return;
				}
				cp::RpcMessage::setTunnelCtl(frame.meta, find_tunnel_request);
				logTunnelD() << "Forwarding FindTunnelRequest:" << frame.meta.toPrettyString();
			}
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(caller_id);
			if(cch) {
				cch->sendRpcFrame(std::move(frame));
			}
			else {
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
			}
		}
		else {
			// broker messages like create master broker subscription
			shvDebug() << "Got RPC response without src connection specified, it should be this broker call like create master broker subscription, throwing message away." << frame.meta.toPrettyString();
		}
	}
	else if(cp::RpcMessage::isSignal(frame.meta)) {
		logSigResolveD() << "SIGNAL:" << frame.meta.toPrettyString() << "from:" << connection_id;

		const std::string sig_shv_path = cp::RpcMessage::shvPath(frame.meta).asString();
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
			cp::RpcMessage::setShvPath(frame.meta, full_shv_path);
			bool sig_sent = sendNotifyToSubscribers(frame);
			if(!sig_sent && client_connection && client_connection->isSlaveBrokerConnection()) {
				logSubscriptionsD() << "Rejecting unsubscribed signal, shv_path:" << full_shv_path << "method:" << cp::RpcMessage::method(frame.meta).asString();
				cp::RpcRequest rq;
				rq.setRequestId(client_connection->nextRequestId());
				rq.setMethod(cp::Rpc::METH_REJECT_NOT_SUBSCRIBED)
						.setParams(cp::RpcValue::Map{
									   { cp::Rpc::PAR_PATH, sig_shv_path},
									   { cp::Rpc::PAR_METHOD, cp::RpcMessage::method(frame.meta).toString()}})
						.setShvPath(cp::Rpc::DIR_BROKER_APP);
				client_connection->sendRpcMessage(rq);
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
			conn->sendRpcMessage(resp);
		else
			shvError() << "Cannot find connection for ID:" << connection_id;
		return;
	}
	if(msg.isSignal()) {
		cp::RpcSignal sig(msg);
		sendNotifyToSubscribers(sig.shvPath().asString(), sig.method().asString(), sig.source(), sig.params());
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

bool BrokerApp::sendNotifyToSubscribers(const chainpack::RpcFrame &frame)
{
	// send it to all clients for now
	bool subs_sent = false;
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			const auto shv_path = cp::RpcMessage::shvPath(frame.meta);
			const auto method = cp::RpcMessage::method(frame.meta);
			const auto source = cp::RpcMessage::source(frame.meta);
			int subs_ix = conn->isSubscribed(shv_path.toString(), method.asString(), source);
			if(subs_ix >= 0) {
				std::string new_path = conn->toSubscribedPath(shv_path.asString());
				auto frame2 = frame;
				cp::RpcMessage::setShvPath(frame2.meta, new_path);
				conn->sendRpcFrame(std::move(frame2));
				subs_sent = true;
			}
		}
	}
	return subs_sent;
}

void BrokerApp::sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const std::string& source, const shv::chainpack::RpcValue &params)
{
	cp::RpcSignal sig;
	sig.setShvPath(shv_path);
	sig.setMethod(method);
	sig.setParams(params);
	sig.setSource(!source.empty() ? source : cp::Rpc::METH_GET);
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			int subs_ix = conn->isSubscribed(shv_path, method, source);
			if(subs_ix >= 0) {
				std::string new_path = conn->toSubscribedPath(shv_path);
				if(new_path != shv_path)
					sig.setShvPath(new_path);
				conn->sendRpcMessage(sig);
			}
		}
	}
}

void BrokerApp::addSubscription(int client_id, const std::string &shv_path, const std::string &method, const std::string& source)
{
	rpc::CommonRpcClientHandle *connection_handle = commonClientConnectionById(client_id);
	if(!connection_handle)
		SHV_EXCEPTION("Cannot create subscription, invalid connection ID.");
	rpc::CommonRpcClientHandle::Subscription subs = connection_handle->createSubscription(shv_path, method, source);
	connection_handle->addSubscription(subs);
	{
		/// check slave broker connections
		/// whether this subsciption should be propagated to them
		/// skip service providers subscriptions, since it does not make sense to send them downstream
		for (int connection_id : clientConnectionIds()) {
			rpc::ClientConnectionOnBroker *conn = clientConnectionById(connection_id);
			if(conn->isSlaveBrokerConnection()) {
				conn->propagateSubscriptionToSlaveBroker(subs);
			}
		}
	}
}

bool BrokerApp::removeSubscription(int client_id, const std::string &shv_path, const std::string &method, const std::string& source)
{
	rpc::CommonRpcClientHandle *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot remove subscription, client doesn't exist.");
	rpc::CommonRpcClientHandle::Subscription subs(shv_path, method, source);
	return conn->removeSubscription(subs);
}

bool BrokerApp::rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method, const std::string& source)
{
	logSubscriptionsD() << "signal rejected, shv_path:" << path << "method:" << method << "source:" << source;
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(client_id);
	if(conn) {
		return conn->rejectNotSubscribedSignal(conn->masterExportedToLocalPath(path), method, source);
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

		shvInfo() << "master broker device ID:" << opts.value("device").toPrettyString() << "for connection:" << kv.first;

		if(opts.value("enabled").toBool() == false)
			continue;

		shvInfo() << "creating master broker connection:" << kv.first;
		auto *bc = new rpc::MasterBrokerConnection(this);
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
}
// For LdapAuthThread.
#include <brokerapp.moc>
