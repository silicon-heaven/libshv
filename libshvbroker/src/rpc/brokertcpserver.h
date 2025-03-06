#pragma once

#include <shv/broker/azureconfig.h>
#include "tcpserver.h"

#include <QSslConfiguration>

#include <optional>

namespace shv::broker::rpc {

class ClientConnectionOnBroker;

class BrokerTcpServer : public shv::iotqt::rpc::TcpServer
{
	Q_OBJECT
	using Super = shv::iotqt::rpc::TcpServer;

public:
	enum SslMode { SecureMode = 0, NonSecureMode };
public:
	BrokerTcpServer(SslMode ssl_mode, const std::optional<AzureConfig>& azureConfig, QObject *parent = nullptr);
	~BrokerTcpServer() override;

	ClientConnectionOnBroker* connectionById(int connection_id);
	bool loadSslConfig();
protected:
	void incomingConnection(qintptr socket_descriptor) override;
	shv::iotqt::rpc::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
protected:
	SslMode m_sslMode;
	QSslConfiguration m_sslConfiguration;
	std::optional<AzureConfig> m_azureConfig;
};
}
