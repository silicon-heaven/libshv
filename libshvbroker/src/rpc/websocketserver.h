#pragma once

#include <shv/broker/azureconfig.h>
#include <QWebSocketServer>

#include <optional>

class QWebSocket;

namespace shv::broker::rpc {

class ClientConnectionOnBroker;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(SslMode secureMode, const std::optional<AzureConfig>& azureConfig, QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	ClientConnectionOnBroker* connectionById(int connection_id);
private:
	ClientConnectionOnBroker* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void unregisterConnection(int connection_id);
private:
	std::map<int, ClientConnectionOnBroker*> m_connections;
	std::optional<AzureConfig> m_azureConfig;
};
}
