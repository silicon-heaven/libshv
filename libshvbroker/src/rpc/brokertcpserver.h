#pragma once

#include "tcpserver.h"

#include <QSslConfiguration>

namespace shv::broker::rpc {

class ClientConnectionOnBroker;

class BrokerTcpServer : public shv::iotqt::rpc::TcpServer
{
	Q_OBJECT
	using Super = shv::iotqt::rpc::TcpServer;

public:
	enum SslMode { SecureMode = 0, NonSecureMode };
public:
	BrokerTcpServer(SslMode ssl_mode, const std::optional<std::string>& azureClientId, QObject *parent = nullptr);
	~BrokerTcpServer() override;

	ClientConnectionOnBroker* connectionById(int connection_id);
	bool loadSslConfig();
protected:
	void incomingConnection(qintptr socket_descriptor) override;
	shv::iotqt::rpc::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
protected:
	SslMode m_sslMode;
	QSslConfiguration m_sslConfiguration;
	std::optional<std::string> m_azureClientId;
};
}
