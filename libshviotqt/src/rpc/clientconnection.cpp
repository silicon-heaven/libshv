#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/iotqt/rpc/clientappclioptions.h>
#include <shv/iotqt/rpc/socket.h>
#include <shv/iotqt/rpc/socketrpcconnection.h>
#include <shv/iotqt/rpc/websocket.h>

#include <shv/iotqt/utils.h>

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>
#include <shv/core/string.h>

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/irpcconnection.h>

#include <QTcpSocket>
#include <QSslSocket>
#include <QLocalSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>
#include <QThread>
#ifdef QT_SERIALPORT_LIB
#include <shv/iotqt/rpc/serialportsocket.h>
#include <QSerialPort>
#endif

#ifdef WITH_SHV_WEBSOCKETS
#include <QWebSocket>
#endif

#include <fstream>
#include <regex>

namespace cp = shv::chainpack;
using namespace std;

namespace shv::iotqt::rpc {

ClientConnection::ClientConnection(QObject *parent)
	: Super(parent)
	, m_loginType(IRpcConnection::LoginType::Sha1)
{
	connect(this, &SocketRpcConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);

	m_checkBrokerConnectedTimer = new QTimer(this);
	connect(m_checkBrokerConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkBrokerConnected);
}

ClientConnection::~ClientConnection()
{
	disconnect(this, &SocketRpcConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);
	shvDebug() << __FUNCTION__;
}

QUrl ClientConnection::connectionUrl() const
{
	return m_connectionUrl;
}

void ClientConnection::setConnectionUrl(const QUrl &url)
{
	m_connectionUrl = url;
	if(auto user = m_connectionUrl.userName(); !user.isEmpty()) {
		setUser(user.toStdString());
	}
	if(auto password = m_connectionUrl.password(); !password.isEmpty()) {
		setPassword(password.toStdString());
	}
	m_connectionUrl.setUserInfo({});
}

void ClientConnection::setConnectionString(const QString &connection_string)
{
	setConnectionUrl(connectionUrlFromString(connection_string));
}

void ClientConnection::setHost(const std::string &host)
{
	setConnectionString(QString::fromStdString(host));
}

QUrl ClientConnection::connectionUrlFromString(const QString &url_str)
{
	static QVector<QString> known_schemes {
		Socket::schemeToString(Socket::Scheme::Tcp),
				Socket::schemeToString(Socket::Scheme::Ssl),
				Socket::schemeToString(Socket::Scheme::WebSocket),
				Socket::schemeToString(Socket::Scheme::WebSocketSecure),
				Socket::schemeToString(Socket::Scheme::SerialPort),
				Socket::schemeToString(Socket::Scheme::LocalSocket),
	};
	QUrl url(url_str);
	if(!known_schemes.contains(url.scheme())) {
		url = QUrl(Socket::schemeToString(Socket::Scheme::Tcp) + QStringLiteral("://") + url_str);
	}
	if(url.port(0) == 0) {
		auto scheme = Socket::schemeFromString(url.scheme().toStdString());
		switch (scheme) {
		case Socket::Scheme::Tcp: url.setPort(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED); break;
		case Socket::Scheme::Ssl: url.setPort(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_SECURED); break;
		case Socket::Scheme::WebSocket: url.setPort(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_NONSECURED); break;
		case Socket::Scheme::WebSocketSecure: url.setPort(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_SECURED); break;
		default: break;
		}
	}
	return url;
}

void ClientConnection::tst_connectionUrlFromString()
{
	for(const auto &[u1, u2] : {
		std::tuple<string, string>("localhost"s, "tcp://localhost:3755"s),
		std::tuple<string, string>("localhost:80"s, "tcp://localhost:80"s),
		std::tuple<string, string>("127.0.0.1"s, "tcp://127.0.0.1:3755"s),
		std::tuple<string, string>("127.0.0.1:80"s, "tcp://127.0.0.1:80"s),
		std::tuple<string, string>("jessie.elektroline.cz"s, "tcp://jessie.elektroline.cz:3755"s),
		std::tuple<string, string>("jessie.elektroline.cz:80"s, "tcp://jessie.elektroline.cz:80"s),
	}) {
		QUrl url1 = connectionUrlFromString(QString::fromStdString(u1));
		QUrl url2(QString::fromStdString(u2));
		shvInfo() << url1.toString() << "vs" << url2.toString() << "host:" << url1.host() << "port:" << url1.port();
		shvInfo() << "url scheme:" << url1.scheme();
		shvInfo() << "url host:" << url1.host();
		shvInfo() << "url port:" << url1.port();
		shvInfo() << "url path:" << url1.path();
		shvInfo() << "url query:" << url1.query();
		if(url1 == url2) {
			shvInfo() << "OK";
		}
		else {
			shvError() << "FAIL";
		}
	}
}

void ClientConnection::close()
{
	closeOrAbort(false);
}

void ClientConnection::abort()
{
	closeOrAbort(true);
}

void ClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	if(cli_opts->rpcTimeout_isset()) {
		cp::RpcDriver::setDefaultRpcTimeoutMsec(cli_opts->rpcTimeout() * 1000);
		shvInfo() << "Default RPC timeout set to:" << cp::RpcDriver::defaultRpcTimeoutMsec() << "msec.";
	}

	setConnectionString(QString::fromStdString(cli_opts->serverHost()));
	setPeerVerify(cli_opts->serverPeerVerify());
	if(cli_opts->user_isset())
		setUser(cli_opts->user());
	if(cli_opts->passwordFile_isset()) {
		if(std::ifstream is(cli_opts->passwordFile(), std::ios::binary); is) {
			std::string pwd;
			is >> pwd;
			setPassword(pwd);
		}
		else {
			shvError() << "Cannot open password file";
		}
	}
	if(cli_opts->password_isset())
		setPassword(cli_opts->password());
	shvDebug() << cli_opts->loginType() << "-->" << static_cast<int>(shv::chainpack::UserLogin::loginTypeFromString(cli_opts->loginType()));
	setLoginType(shv::chainpack::UserLogin::loginTypeFromString(cli_opts->loginType()));

	setHeartBeatInterval(cli_opts->heartBeatInterval());
	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * heartBeatInterval();
		setConnectionOptions(opts);
	}
}

void ClientConnection::setTunnelOptions(const chainpack::RpcValue &opts)
{
	shv::chainpack::RpcValue::Map conn_opts = connectionOptions().asMap();
	conn_opts[cp::Rpc::KEY_TUNNEL] = opts;
	setConnectionOptions(conn_opts);
}

void ClientConnection::open()
{
	if(!hasSocket()) {
		QUrl url = connectionUrl();
		auto scheme = Socket::schemeFromString(url.scheme().toStdString());
		Socket *socket;
		if(scheme == Socket::Scheme::WebSocket || scheme == Socket::Scheme::WebSocketSecure) {
#ifdef WITH_SHV_WEBSOCKETS
			socket = new WebSocket(new QWebSocket());
#else
			SHV_EXCEPTION("Web socket support is not part of this build.");
#endif
		}
		else if(scheme == Socket::Scheme::LocalSocket) {
			socket = new LocalSocket(new QLocalSocket());
		}

#ifdef QT_SERIALPORT_LIB
		else if(scheme == Socket::Scheme::SerialPort) {
			socket = new SerialPortSocket(new QSerialPort());
		}
#endif
		else {
	#ifndef QT_NO_SSL
			QSslSocket::PeerVerifyMode peer_verify_mode = isPeerVerify() ? QSslSocket::AutoVerifyPeer : QSslSocket::VerifyNone;
			socket = scheme == Socket::Scheme::Ssl ? new SslSocket(new QSslSocket(), peer_verify_mode): new TcpSocket(new QTcpSocket());
	#else
			socket = new TcpSocket(new QTcpSocket());
	#endif
		}
		setSocket(socket);
	}
	checkBrokerConnected();
	if(m_checkBrokerConnectedInterval > 0) {
		shvInfo() << "Starting check-connected timer, interval:" << m_checkBrokerConnectedInterval/1000 << "sec.";
		m_checkBrokerConnectedTimer->start(m_checkBrokerConnectedInterval);
	}
	else {
		shvInfo() << "Check-connected timer DISABLED";
	}
}

void ClientConnection::closeOrAbort(bool is_abort)
{
	shvInfo() << "close connection, abort:" << is_abort;
	m_checkBrokerConnectedTimer->stop();
	if(m_socket) {
		if(is_abort)
			abortSocket();
		else
			closeSocket();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	setState(State::NotConnected);
}

void ClientConnection::setCheckBrokerConnectedInterval(int ms)
{
	m_checkBrokerConnectedInterval = ms;
	if(ms == 0)
		m_checkBrokerConnectedTimer->stop();
	else
		m_checkBrokerConnectedTimer->setInterval(ms);
}

int ClientConnection::checkBrokerConnectedInterval() const
{
	return m_checkBrokerConnectedInterval;
}

bool ClientConnection::isBrokerConnected() const
{
	return state() == State::BrokerConnected;
}

static constexpr std::string_view::size_type MAX_LOG_LEN = 1024;
static const auto TOPIC_RPC_MSG = "RpcMsg";

void ClientConnection::sendRpcMessage(const cp::RpcMessage &rpc_msg)
{
	if(NecroLog::shouldLog(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, TOPIC_RPC_MSG))) {
		if(isShvPathMutedInLog(rpc_msg.shvPath().asString(), rpc_msg.method().asString())) {
			if(rpc_msg.isRequest()) {
				auto rq_id = rpc_msg.requestId().toInt64();
				QElapsedTimer tm;
				tm.start();
				m_responseIdsMutedInLog.emplace_back(rq_id, tm);
			}
		}
		else {
			NecroLog::create(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, TOPIC_RPC_MSG))
				<< SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< std::string_view(m_rawRpcMessageLog? rpc_msg.toCpon(): rpc_msg.toPrettyString()).substr(0, MAX_LOG_LEN);
		}
	}
	sendRpcMessage(rpc_msg);
}

void ClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &rpc_msg)
{
	if(NecroLog::shouldLog(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, TOPIC_RPC_MSG))) {
		bool skip_log = false;
		if(rpc_msg.isResponse()) {
			QElapsedTimer now;
			now.start();
			for (auto iter = m_responseIdsMutedInLog.begin(); iter != m_responseIdsMutedInLog.end(); ) {
				const auto &[rq_id, elapsed_tm] = *iter;
				//shvError() << rq_id << "vs" << msg.requestId().toInt64();
				if(rq_id == rpc_msg.requestId().toInt64()) {
					iter = m_responseIdsMutedInLog.erase(iter);
					skip_log = true;
				}
				else if(elapsed_tm.msecsTo(now) > 10000) {
					iter = m_responseIdsMutedInLog.erase(iter);
				}
				else {
					++iter;
				}
			}
		}
		else {
			skip_log = isShvPathMutedInLog(rpc_msg.shvPath().asString(), rpc_msg.method().asString());
		}
		if(!skip_log) {
			NecroLog::create(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, TOPIC_RPC_MSG))
				<< cp::RpcDriver::RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< std::string_view(m_rawRpcMessageLog? rpc_msg.toCpon(): rpc_msg.toPrettyString()).substr(0, MAX_LOG_LEN);
		}
	}
	if(isLoginPhase()) {
		processLoginPhase(rpc_msg);
		return;
	}
	if(rpc_msg.isResponse()) {
		cp::RpcResponse rp(rpc_msg);
		if(rp.requestId() == m_connectionState.pingRqId) {
			m_connectionState.pingRqId = 0;
			return;
		}
	}
	emit rpcMessageReceived(rpc_msg);
}

void ClientConnection::setState(ClientConnection::State state)
{
	if(m_connectionState.state == state)
		return;
	State old_state = m_connectionState.state;
	m_connectionState.state = state;
	emit stateChanged(state);
	if(old_state == State::BrokerConnected)
		whenBrokerConnectedChanged(false);
	else if(state == State::BrokerConnected)
		whenBrokerConnectedChanged(true);
}

void ClientConnection::sendHello()
{
	m_connectionState.helloRequestId = callShvMethod({}, cp::Rpc::METH_HELLO);
}

void ClientConnection::sendLogin(const shv::chainpack::RpcValue &server_hello)
{
	m_connectionState.loginRequestId = callShvMethod({}, cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

void ClientConnection::checkBrokerConnected()
{
	shvDebug() << "check broker connected: " << isSocketConnected();
	if(!isBrokerConnected()) {
		abortSocket();
		m_connectionState = ConnectionState();
		auto url = connectionUrl();
		shvInfo().nospace() << "connecting to: " << url.toString();
		setState(State::Connecting);
		connectToHost(url);
	}
}

void ClientConnection::whenBrokerConnectedChanged(bool b)
{
	if(b) {
		shvInfo() << "Connected to broker" << "client id:" << brokerClientId();// << "mount point:" << brokerMountPoint();
		if(heartBeatInterval() > 0) {
			if(!m_heartBeatTimer) {
				shvInfo() << "Creating heart-beat timer, interval:" << heartBeatInterval() << "sec.";
				m_heartBeatTimer = new QTimer(this);
				m_heartBeatTimer->setInterval(heartBeatInterval() * 1000);
				connect(m_heartBeatTimer, &QTimer::timeout, this, [this]() {
					if(m_connectionState.pingRqId > 0) {
						shvError() << "PING response not received within" << (m_heartBeatTimer->interval() / 1000) << "seconds, restarting conection to broker.";
						restartIfAutoConnect();
					}
					else {
						m_connectionState.pingRqId = callShvMethod(pingShvPath(), cp::Rpc::METH_PING);
					}
				});
			}
			m_heartBeatTimer->start();
		}
		else {
			shvWarning() << "Heart-beat timer interval is set to 0, heart beats will not be sent.";
		}
	}
	else {
		if(m_heartBeatTimer)
			m_heartBeatTimer->stop();
	}
	emit brokerConnectedChanged(b);
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	if(is_connected) {
		shvInfo() << objectName() << "connection id:" << connectionId() << "Socket connected to RPC server";
		shvInfo() << "peer:" << peerAddress() << "port:" << peerPort();
		setState(State::SocketConnected);
		if(loginType() == LoginType::None) {
			shvInfo() << "Connection scheme:" << connectionUrl().scheme() << " is skipping login phase.";
			setState(State::BrokerConnected);
		}
		else {
			sendHello();
		}
	}
	else {
		shvInfo() << objectName() << "connection id:" << connectionId() << "Socket disconnected from RPC server";
		setState(State::NotConnected);
	}
}

chainpack::RpcValue ClientConnection::createLoginParams(const chainpack::RpcValue &server_hello) const
{
	shvDebug() << server_hello.toCpon() << "login type:" << static_cast<int>(loginType());
	std::string user_name = user();
	std::string pass;
	if(loginType() == chainpack::IRpcConnection::LoginType::Sha1) {
		std::string server_nonce = server_hello.asMap().value("nonce").toString();
		std::string pwd = password();
		do {
			if(pwd.size() == 40) {
				// sha1passwd
				std::regex re(R"([0-9a-f]{40})");
				if(std::regex_match(pwd, re)) {
					/// SHA1 password
					break;
				}
			}
			pwd = utils::sha1Hex(pwd);
		} while(false);
		std::string pn = server_nonce + pwd;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
		hash.addData(QByteArrayView(pn.data(), static_cast<int>(pn.length())));
#else
		hash.addData(pn.data(), static_cast<int>(pn.length()));
#endif
		QByteArray sha1 = hash.result().toHex();
		pass = std::string(sha1.constData(), static_cast<size_t>(sha1.size()));
	}
	else if(loginType() == chainpack::IRpcConnection::LoginType::Plain) {
		pass = password();
		shvDebug() << "plain password:" << pass;
	}
	else if(loginType() == chainpack::IRpcConnection::LoginType::AzureAccessToken) {
		user_name = "";
		pass = password();
		shvDebug() << "AzureAccessToken:" << pass;
	}
	else {
		shvError() << "Login type:" << chainpack::UserLogin::loginTypeToString(loginType()) << "not supported";
	}
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			{"user", user_name},
			{"password", pass},
			{"type", chainpack::UserLogin::loginTypeToString(loginType())},
		 },
		},
		{"options", connectionOptions()},
	};
}

bool ClientConnection::isShvPathMutedInLog(const std::string &shv_path, const std::string &method) const
{
	if(shv_path.empty())
		return false;
	for(const auto &muted_path : m_mutedShvPathsInLog) {
		shv::core::StringView sv(muted_path.pathPattern);
		if(!sv.empty() && sv.at(0) == '*') {
			sv = sv.substr(1);
			if(shv::core::StringView(shv_path).ends_with(sv)) {
				return muted_path.methodPattern.empty() || muted_path.methodPattern == method;
			}
		}
		else {
			if(shv_path == muted_path.pathPattern) {
				return muted_path.methodPattern.empty() || muted_path.methodPattern == method;
			}
		}
	}
	return false;
}

bool ClientConnection::isAutoConnect() const
{
	return m_checkBrokerConnectedInterval > 0;
}

void ClientConnection::restartIfAutoConnect()
{
	if(isAutoConnect())
		setState(State::ConnectionError);
	else
		close();
}

const string &ClientConnection::pingShvPath() const
{
	if(m_shvApiVersion == ShvApiVersion::V3) {
		static std::string s = ".app";
		return s;
	}
	else {
		static std::string s = ".broker/app";
		return s;
	}
}

ClientConnection::State ClientConnection::state() const
{
	return m_connectionState.state;
}

const shv::chainpack::RpcValue::Map &ClientConnection::loginResult() const
{
	return m_connectionState.loginResult.asMap();
}

int ClientConnection::brokerClientId() const
{
	return m_connectionState.loginResult.asMap().value(cp::Rpc::KEY_CLIENT_ID).toInt();
}

void ClientConnection::muteShvPathInLog(const std::string &shv_path, const std::string &method)
{
	shvInfo() << "RpcMsg log, mutting shv_path:" << shv_path << "method:" << method;
	m_mutedShvPathsInLog.emplace_back(MutedPath{.pathPattern = shv_path, .methodPattern = method});
}

void ClientConnection::setRawRpcMessageLog(bool b)
{
	m_rawRpcMessageLog = b;
}

bool ClientConnection::isLoginPhase() const
{
	return state() == State::SocketConnected;
}

void ClientConnection::processLoginPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		if(resp.isError()) {
			setState(State::ConnectionError);
			emit brokerLoginError(resp.error());
			break;
		}
		int id = resp.requestId().toInt();
		if(id == 0)
			break;
		if(m_connectionState.helloRequestId == id) {
			sendLogin(resp.result());
			return;
		}
		if(m_connectionState.loginRequestId == id) {
			m_connectionState.loginResult = resp.result();
			setState(State::BrokerConnected);
			return;
		}
	} while(false);
	shvError() << "Invalid handshake message! Dropping connection." << msg.toCpon();
	restartIfAutoConnect();
}

const char *ClientConnection::stateToString(ClientConnection::State state)
{
	switch (state) {
	case State::NotConnected: return "NotConnected";
	case State::Connecting: return "Connecting";
	case State::SocketConnected: return "SocketConnected";
	case State::BrokerConnected: return "BrokerConnected";
	case State::ConnectionError: return "ConnectionError";
	}
	return "this could never happen";
}

}


