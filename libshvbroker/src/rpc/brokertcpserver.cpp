#include "brokertcpserver.h"
#include "ssl_common.h"
#include "clientconnectiononbroker.h"
#include <shv/broker/brokerapp.h>

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/socket.h>
#include <QSslSocket>

#include <QFile>
#include <QDir>

namespace shv::broker::rpc {

BrokerTcpServer::BrokerTcpServer(SslMode ssl_mode, const std::optional<AzureConfig>& azureConfig, QObject *parent)
	: Super(parent)
	, m_sslMode(ssl_mode)
	, m_azureConfig(azureConfig)
{
}

BrokerTcpServer::~BrokerTcpServer() = default;

ClientConnectionOnBroker *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ClientConnectionOnBroker *>(Super::connectionById(connection_id));
}

bool BrokerTcpServer::loadSslConfig()
{
	if (m_sslMode == SslMode::SecureMode) {
		m_sslConfiguration = load_ssl_configuration(BrokerApp::instance()->cliOptions());
	}
	return !m_sslConfiguration.isNull();
}

void BrokerTcpServer::incomingConnection(qintptr socket_descriptor)
{
	shvLogFuncFrame() << socket_descriptor;
	if (m_sslMode == SecureMode) {
		auto *socket = new QSslSocket(this);
		{
			connect(socket, &QSslSocket::connected, this, [this]() {
				shvDebug() << this << "Socket connected!!!";
			});
			connect(socket, &QSslSocket::disconnected, this, [this]() {
				shvDebug() << this << "Socket disconnected!!!";
			});
			connect(socket, &QSslSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
				shvDebug() << this << "Socket state changed:" << state;
			});
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
			connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [socket](QAbstractSocket::SocketError socket_error) {
#else
			connect(socket, &QAbstractSocket::errorOccurred, this, [socket](QAbstractSocket::SocketError socket_error) {
#endif
				shvWarning() << "Socket error:" << socket_error << socket->errorString();
			});
			connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), [](const QList<QSslError> &errors){
				shvWarning() << "SSL Errors:";
				for(const auto &err : errors)
					shvWarning() << err.errorString();
			});
			connect(socket, &QSslSocket::peerVerifyError, [](const QSslError &error) {
				shvWarning() << "SSL peer verify errors:" << error.errorString();
			});

		}
		if (!socket->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		shvDebug() << "startServerEncryption";
		socket->setSslConfiguration(m_sslConfiguration);
		socket->startServerEncryption();
		addPendingConnection(socket);
	} else if (m_sslMode == NonSecureMode) {
		auto *sock = new QTcpSocket(this);
		if (!sock->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		addPendingConnection(sock);
	}
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	if (m_sslMode == SecureMode) {
		return new ClientConnectionOnBroker(new shv::iotqt::rpc::SslSocket(qobject_cast<QSslSocket *>(socket)), m_azureConfig, parent);
	}

	return new ClientConnectionOnBroker(new shv::iotqt::rpc::TcpSocket(socket), m_azureConfig, parent);
}

}
