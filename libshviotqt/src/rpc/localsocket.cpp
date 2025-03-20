#include <shv/iotqt/rpc/localsocket.h>

#include <shv/chainpack/utils.h>
#include <shv/iotqt/rpc/serialportsocket.h>

#include <QLocalSocket>
#include <QHostAddress>
#include <QUrl>

namespace shv::iotqt::rpc {

//======================================================
// LocalSocket
//======================================================
namespace {
QAbstractSocket::SocketState LocalSocket_convertState(QLocalSocket::LocalSocketState state)
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

template <typename Type>
std::unique_ptr<Type> make(LocalSocket::Protocol protocol)
{
	switch (protocol) {
	case shv::iotqt::rpc::LocalSocket::Protocol::Serial:
#ifndef QT_SERIALPORT_LIB
		throw std::runtime_error("libshv wasn't compiled with serial port support");
#endif
		if constexpr (std::same_as<Type, FrameReader>) {
			return std::make_unique<SerialFrameReader>(SerialFrameReader::CrcCheck::No);
		} else {
			return std::make_unique<SerialFrameWriter>(SerialFrameWriter::CrcCheck::No);
		}
	case shv::iotqt::rpc::LocalSocket::Protocol::Stream:
		if constexpr (std::same_as<Type, FrameReader>) {
			return std::make_unique<StreamFrameReader>();
		} else {
			return std::make_unique<StreamFrameWriter>();
		}
	default:
		throw std::runtime_error("Unknown protocol " + std::to_string(static_cast<int>(protocol)));
	}
}
}

LocalSocket::LocalSocket(QLocalSocket *socket, Protocol protocol, QObject *parent)
	: Super(make<FrameReader>(protocol), make<FrameWriter>(protocol), parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QLocalSocket::connected, this, &Socket::connected);
	connect(m_socket, &QLocalSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QLocalSocket::readyRead, this, &LocalSocket::onDataReadyRead);
	connect(m_socket, &QLocalSocket::bytesWritten, this, &LocalSocket::flushWriteBuffer);
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

LocalSocket::~LocalSocket() = default;

void LocalSocket::connectToHost(const QUrl &url)
{
	m_socket->connectToServer(url.path());
}

void LocalSocket::close()
{
	Super::close();
	m_socket->close();
}

void LocalSocket::abort()
{
	Super::abort();
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

void LocalSocket::ignoreSslErrors()
{
}

void LocalSocket::onDataReadyRead()
{
	auto ba = m_socket->readAll();
	try {
		std::string_view escaped_data(ba.constData(), ba.size());
		for (auto rqid : frameReader().addData(escaped_data)) {
			emit responseMetaReceived(rqid);
		}
		emit dataChunkReceived();
		if (!frameReader().isEmpty()) {
			emit readyRead();
		}
	}
	catch (const std::runtime_error &e) {
		shvWarning() << "Corrupted meta data received:" << e.what() << "\n" << shv::chainpack::utils::hexDump(std::string_view(ba.constData(), std::min(ba.size(), static_cast<decltype(ba.size())>(64))));
		emit error(QAbstractSocket::SocketError::UnknownSocketError);
	}
}

void LocalSocket::flushWriteBuffer()
{
	frameWriter().flushToDevice(m_socket);
	m_socket->flush();
}


} // namespace shv::iotqt::rpc


