#include "socket.h"

#include <shv/coreqt/log.h>
#include <shv/chainpack/irpcconnection.h>

#include <QHostAddress>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QTimer>

namespace shv::iotqt::rpc {

//======================================================
// Socket
//======================================================
Socket::Socket(QObject *parent)
	: QObject(parent)
{

}

const char * Socket::schemeToString(Scheme schema)
{
	switch (schema) {
	case Scheme::Tcp: return "tcp";
	case Scheme::Ssl: return "ssl";
	case Scheme::WebSocket: return "ws";
	case Scheme::WebSocketSecure: return "wss";
	case Scheme::SerialPort: return "serialport";
	case Scheme::LocalSocket: return "localsocket";
	}
	return "";
}

Socket::Scheme Socket::schemeFromString(const std::string &schema)
{
	if(schema == "tcp") return Scheme::Tcp;
	if(schema == "ssl") return Scheme::Ssl;
	if(schema == "ws") return Scheme::WebSocket;
	if(schema == "wss") return Scheme::WebSocketSecure;
	if(schema == "serialport") return Scheme::SerialPort;
	if(schema == "localsocket") return Scheme::LocalSocket;
	return Scheme::Tcp;
}

void Socket::onParseDataException(const shv::chainpack::ParseException &)
{
	abort();
}

//======================================================
// TcpSocket
//======================================================
TcpSocket::TcpSocket(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(m_socket, &QTcpSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &Socket::readyRead);
	connect(m_socket, &QTcpSocket::bytesWritten, this, &Socket::bytesWritten);
	connect(m_socket, &QTcpSocket::stateChanged, this, &Socket::stateChanged);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Socket::error);
#else
	connect(m_socket, &QAbstractSocket::errorOccurred, this, &Socket::error);
#endif
}

void TcpSocket::connectToHost(const QUrl &url)
{
	auto port = url.port(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED);
	m_socket->connectToHost(url.host(), static_cast<quint16>(port));
}

void TcpSocket::close()
{
	m_socket->close();
}

void TcpSocket::abort()
{
	m_socket->abort();
}

QAbstractSocket::SocketState TcpSocket::state() const
{
	return m_socket->state();
}

QString TcpSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress TcpSocket::peerAddress() const
{
	return m_socket->peerAddress();
}

quint16 TcpSocket::peerPort() const
{
	return m_socket->peerPort();
}

QByteArray TcpSocket::readAll()
{
	return m_socket->readAll();
}

qint64 TcpSocket::write(const char *data, qint64 max_size)
{
	return m_socket->write(data, max_size);
}

void TcpSocket::writeMessageBegin()
{
}

void TcpSocket::writeMessageEnd()
{
}

void TcpSocket::ignoreSslErrors()
{
}

//======================================================
// LocalSocket
//======================================================
static QAbstractSocket::SocketState LocalSocket_convertState(QLocalSocket::LocalSocketState state)
{
	switch (state) {
	case QLocalSocket::UnconnectedState:
		return QAbstractSocket::UnconnectedState;
	case QLocalSocket::ConnectingState:
		return QAbstractSocket::ConnectingState;
	case QLocalSocket::ConnectedState:
		return QAbstractSocket::ConnectedState;
	case QLocalSocket::ClosingState:
		return QAbstractSocket::ClosingState;
	}
	return QAbstractSocket::UnconnectedState;
}

LocalSocket::LocalSocket(QLocalSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QLocalSocket::connected, this, &Socket::connected);
	connect(m_socket, &QLocalSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QLocalSocket::readyRead, this, &Socket::readyRead);
	connect(m_socket, &QLocalSocket::bytesWritten, this, &Socket::bytesWritten);
	connect(m_socket, &QLocalSocket::stateChanged, this, [this](QLocalSocket::LocalSocketState state) {
		emit stateChanged(LocalSocket_convertState(state));
	});
	connect(m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError socket_error) {
		switch (socket_error) {
		case QLocalSocket::ConnectionRefusedError:
			emit error(QAbstractSocket::ConnectionRefusedError);
			break;
		case QLocalSocket::PeerClosedError:
			emit error(QAbstractSocket::RemoteHostClosedError);
			break;
		case QLocalSocket::ServerNotFoundError:
			emit error(QAbstractSocket::HostNotFoundError);
			break;
		case QLocalSocket::SocketAccessError:
			emit error(QAbstractSocket::SocketAddressNotAvailableError);
			break;
		case QLocalSocket::SocketResourceError:
			emit error(QAbstractSocket::SocketResourceError);
			break;
		case QLocalSocket::SocketTimeoutError:
			emit error(QAbstractSocket::SocketTimeoutError);
			break;
		case QLocalSocket::DatagramTooLargeError:
			emit error(QAbstractSocket::DatagramTooLargeError);
			break;
		case QLocalSocket::ConnectionError:
			emit error(QAbstractSocket::NetworkError);
			break;
		case QLocalSocket::UnsupportedSocketOperationError:
			emit error(QAbstractSocket::UnsupportedSocketOperationError);
			break;
		case QLocalSocket::UnknownSocketError:
			emit error(QAbstractSocket::UnknownSocketError);
			break;
		case QLocalSocket::OperationError:
			emit error(QAbstractSocket::OperationError);
			break;
		}
	});
}

void LocalSocket::connectToHost(const QUrl &url)
{
	m_socket->connectToServer(url.path());
}

void LocalSocket::close()
{
	m_socket->close();
}

void LocalSocket::abort()
{
	m_socket->abort();
}

QAbstractSocket::SocketState LocalSocket::state() const
{
	return LocalSocket_convertState(m_socket->state());
}

QString LocalSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress LocalSocket::peerAddress() const
{
	return QHostAddress(m_socket->serverName());
}

quint16 LocalSocket::peerPort() const
{
	return 0;
}

QByteArray LocalSocket::readAll()
{
	return m_socket->readAll();
}

qint64 LocalSocket::write(const char *data, qint64 max_size)
{
	return m_socket->write(data, max_size);
}

void LocalSocket::writeMessageBegin()
{
}

void LocalSocket::writeMessageEnd()
{
}

void LocalSocket::ignoreSslErrors()
{
}

//======================================================
// SslSocket
//======================================================
#ifndef QT_NO_SSL
SslSocket::SslSocket(QSslSocket *socket, QSslSocket::PeerVerifyMode peer_verify_mode, QObject *parent)
	: Super(socket, parent)
	, m_peerVerifyMode(peer_verify_mode)
{
	disconnect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(socket, &QSslSocket::encrypted, this, &Socket::connected);
	connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &Socket::sslErrors);
}

void SslSocket::connectToHost(const QUrl &url)
{
	auto *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->setPeerVerifyMode(m_peerVerifyMode);
	shvDebug() << "connectToHostEncrypted" << "host:" << url.toString();
	auto port = url.port(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_SECURED);
	ssl_socket->connectToHostEncrypted(url.host(), static_cast<quint16>(port));
}

void SslSocket::ignoreSslErrors()
{
	auto *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->ignoreSslErrors();
}

#endif
} // namespace shv
