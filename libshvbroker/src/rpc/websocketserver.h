#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace shv::broker::rpc {

class ClientConnectionOnBroker;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(SslMode secureMode, QObject *parent = nullptr);
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
};
}
