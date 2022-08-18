#include "socket.h"

#include <shv/coreqt/log.h>

#include <QHostAddress>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QSerialPort>
#include <QTimer>

namespace shv {
namespace iotqt {
namespace rpc {

//======================================================
// Socket
//======================================================
Socket::Socket(QObject *parent)
	: QObject(parent)
{

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

void TcpSocket::connectToHost(const QString &host_name, quint16 port, const QString &)
{
	m_socket->connectToHost(host_name, port);
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

void TcpSocket::writeMessageEnd()
{
	/// direct flush in QSslSocket call can cause readyRead() emit
	/// QSslSocketBackendPrivate::transmit() is badly implemented
	/// It does not hurt to comment this, since flush() called automatically from event loop anyway
	//m_socket->flush(); // bacha na to, je to zlo
	//QTimer::singleShot(0, m_socket, &QTcpSocket::flush);
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

void LocalSocket::connectToHost(const QString &host_name, quint16 , const QString &)
{
	m_socket->connectToServer(host_name);
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

//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(parent)
	, m_port(port)
{
	m_port->setParent(this);

	//connect(m_port, &QSerialPort::connected, this, &Socket::connected);
	//connect(m_port, &QSerialPort::disconnected, this, &Socket::disconnected);
	connect(m_port, &QSerialPort::readyRead, this, &Socket::readyRead);
	connect(m_port, &QSerialPort::bytesWritten, this, &Socket::bytesWritten);
	//connect(m_port, &QSerialPort::stateChanged, this, [this](QLocalSocket::LocalSocketState state) {
	//	emit stateChanged(LocalSocket_convertState(state));
	//});
	connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError port_error) {
		switch (port_error) {
		case QSerialPort::NoError:
			break;
		case QSerialPort::DeviceNotFoundError:
			emit error(QAbstractSocket::HostNotFoundError);
			break;
		case QSerialPort::PermissionError:
			emit error(QAbstractSocket::SocketAccessError);
			break;
		case QSerialPort::OpenError:
			emit error(QAbstractSocket::ConnectionRefusedError);
			break;
#if QT_VERSION_MAJOR < 6
		case QSerialPort::ParityError:
		case QSerialPort::FramingError:
		case QSerialPort::BreakConditionError:
#endif
		case QSerialPort::WriteError:
		case QSerialPort::ReadError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::ResourceError:
			emit error(QAbstractSocket::SocketResourceError);
			break;
		case QSerialPort::UnsupportedOperationError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::UnknownError:
			emit error(QAbstractSocket::UnknownSocketError);
			break;
		case QSerialPort::TimeoutError:
			emit error(QAbstractSocket::SocketTimeoutError);
			break;
		case QSerialPort::NotOpenError:
			emit error(QAbstractSocket::SocketAccessError);
			break;
		}
	});
}

void SerialPortSocket::connectToHost(const QString &host_name, quint16 , const QString &)
{
	abort();
	setState(QAbstractSocket::ConnectingState);
	m_port->setPortName(host_name);
	if(m_port->open(QIODevice::ReadWrite)) {
		setState(QAbstractSocket::ConnectedState);
	}
	else {
		setState(QAbstractSocket::UnconnectedState);
	}
}

void SerialPortSocket::close()
{
	if(state() == QAbstractSocket::UnconnectedState)
		return;
	setState(QAbstractSocket::ClosingState);
	m_port->close();
	setState(QAbstractSocket::UnconnectedState);
}

void SerialPortSocket::abort()
{
	close();
}

QString SerialPortSocket::errorString() const
{
	return m_port->errorString();
}

QHostAddress SerialPortSocket::peerAddress() const
{
	return QHostAddress(m_port->portName());
}

quint16 SerialPortSocket::peerPort() const
{
	return 0;
}

QByteArray SerialPortSocket::readAll()
{
	return m_port->readAll();
}

qint64 SerialPortSocket::write(const char *data, qint64 max_size)
{
	return m_port->write(data, max_size);
}

void SerialPortSocket::writeMessageEnd()
{
	m_port->flush();
}

void SerialPortSocket::setState(QAbstractSocket::SocketState state)
{
	if(state == m_state)
		return;
	m_state = state;
	emit stateChanged(m_state);
}

//======================================================
// SslSocket
//======================================================
#ifndef QT_NO_SSL
SslSocket::SslSocket(QSslSocket *socket, QSslSocket::PeerVerifyMode peer_verify_mode, QObject *parent)
	: Super(socket, parent)
	, m_peerVerifyMode(peer_verify_mode)
{
	/*
	{
		QSslConfiguration ssl_configuration = socket->sslConfiguration();
		shvDebug() << "SSL socket config:";
		shvDebug() << "protocol:" << (int)ssl_configuration.protocol();
		//ssl_configuration.setProtocol(QSsl::TlsV1SslV3);
		//socket->setSslConfiguration(ssl_configuration);
	}
	*/
	disconnect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(socket, &QSslSocket::encrypted, this, &Socket::connected);
	connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &Socket::sslErrors);
}

void SslSocket::connectToHost(const QString &host_name, quint16 port, const QString &)
{
	QSslSocket *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->setPeerVerifyMode(m_peerVerifyMode);
	shvDebug() << "connectToHostEncrypted" << "host:" << host_name << "port:" << port;
	ssl_socket->connectToHostEncrypted(host_name, port);
}

void SslSocket::ignoreSslErrors()
{
	QSslSocket *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->ignoreSslErrors();
}

#endif
} // namespace rpc
} // namespace iotqt
} // namespace shv
