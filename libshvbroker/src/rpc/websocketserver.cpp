#include "websocketserver.h"

#include "ssl_common.h"
#include "clientconnectiononbroker.h"
#include <shv/broker/brokerapp.h>

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/websocket.h>

#include <QFile>
#include <QDir>
#include <QSslKey>
#include <QWebSocket>

namespace shv::broker::rpc {

WebSocketServer::WebSocketServer(SslMode secureMode, const std::optional<AzureConfig>& azureConfig, QObject *parent)
	: Super("shvbroker", secureMode, parent)
	, m_azureConfig(azureConfig)
{
	if (secureMode == SecureMode) {
		const QSslConfiguration ssl_configuration = load_ssl_configuration(BrokerApp::instance()->cliOptions());
		if (ssl_configuration.isNull())
			shvError() << "Cannot load SSL configuration";
		else
			setSslConfiguration(ssl_configuration);
	}
	connect(this, &WebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer() = default;

bool WebSocketServer::start(int port)
{
	quint16 p = port == 0? 3777: static_cast<quint16>(port);
	shvInfo() << "Starting" << (secureMode() == QWebSocketServer::SecureMode? "secure (wss:)": "not-secure (ws:)") << "WebSocket server on port:" << p;
	if (!listen(QHostAddress::AnyIPv4, p)) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace()
			<< "WebSocket RPC server is listenning on "
			<< serverAddress().toString() << ":" << serverPort()
			<< " in " << (secureMode() == QWebSocketServer::SecureMode? " secure (wss:)": " not-secure (ws:)") << " mode";
	return true;
}

std::vector<int> WebSocketServer::connectionIds() const
{
	std::vector<int> ret;
	for(const auto &pair : m_connections)
		ret.push_back(pair.first);
	return ret;
}

ClientConnectionOnBroker *WebSocketServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}

ClientConnectionOnBroker *WebSocketServer::createServerConnection(QWebSocket *socket, QObject *parent)
{
	return new ClientConnectionOnBroker(new shv::iotqt::rpc::WebSocket(socket), m_azureConfig, parent);
}

void WebSocketServer::onNewConnection()
{
	shvInfo() << "New WebSocket connection";
	QWebSocket *sock = nextPendingConnection();
	if(sock) {
		ClientConnectionOnBroker *c = createServerConnection(sock, this);
		shvInfo().nospace() << "web socket client connected: " << sock->peerAddress().toString() << ':' << sock->peerPort()
							<< " connection ID: " << c->connectionId();
		c->setConnectionName(sock->peerAddress().toString().toStdString() + ':' + std::to_string(sock->peerPort()));
		m_connections[c->connectionId()] = c;
		connect(c, &ClientConnectionOnBroker::aboutToBeDeleted, this, &WebSocketServer::unregisterConnection);
	}
}

void WebSocketServer::unregisterConnection(int connection_id)
{
	m_connections.erase(connection_id);
}

}
