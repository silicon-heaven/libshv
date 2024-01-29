#include <shv/iotqt/rpc/localsocket.h>

#include <shv/iotqt/rpc/serialportsocket.h>

#include <QLocalSocket>
#include <QUrl>

namespace shv::iotqt::rpc {

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

LocalSocket::LocalSocket(QLocalSocket *socket, Protocol protocol, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	if (protocol == Protocol::Serial) {
		m_frameReader = new SerialFrameReader();
		m_frameWriter = new SerialFrameWriter();
	}
	else {
		m_frameReader = new StreamFrameReader();
		m_frameWriter = new StreamFrameWriter();
	}
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

LocalSocket::~LocalSocket()
{
	delete m_frameReader;
	delete m_frameWriter;
}

std::string LocalSocket::readFrameData()
{
	if(m_frameReader->isEmpty())
		return {};
	return m_frameReader->getFrame();
}

void LocalSocket::writeFrameData(std::string &&frame_data)
{
	m_frameWriter->addFrame(std::move(frame_data));
	flushWriteBuffer();
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

void LocalSocket::ignoreSslErrors()
{
}

void LocalSocket::onDataReadyRead()
{
	auto ba = m_socket->readAll();
	std::string_view escaped_data(ba.constData(), ba.size());
	m_frameReader->addData(escaped_data);
	if (!m_frameReader->isEmpty()) {
		emit readyRead();
	}
}

void LocalSocket::flushWriteBuffer()
{
	m_frameWriter->flushToDevice(m_socket);
	m_socket->flush();
}


} // namespace shv::iotqt::rpc


