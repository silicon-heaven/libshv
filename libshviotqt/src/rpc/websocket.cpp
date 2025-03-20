#include <shv/iotqt/rpc/websocket.h>

#include <shv/chainpack/utils.h>
#include <shv/coreqt/log.h>

#include <QWebSocket>

namespace shv::iotqt::rpc {

WebSocket::WebSocket(QWebSocket *socket, QObject *parent)
	: Super(std::unique_ptr<StreamFrameReader>(), std::unique_ptr<StreamFrameWriter>(), parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QWebSocket::connected, this, &Socket::connected);
	connect(m_socket, &QWebSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessageReceived);
	connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessageReceived);
	connect(m_socket, &QWebSocket::stateChanged, this, &Socket::stateChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	connect(m_socket, &QWebSocket::errorOccurred, this, &Socket::error);
#else
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &Socket::error);
#endif
#ifndef QT_NO_SSL
	connect(m_socket, &QWebSocket::sslErrors, this, &Socket::sslErrors);
#endif
}

void WebSocket::connectToHost(const QUrl &url)
{
	shvInfo() << "connecting to:" << url.toString();
	m_socket->open(url);
}

void WebSocket::close()
{
	Super::close();
	m_socket->close();
}

void WebSocket::abort()
{
	Super::abort();
	m_socket->abort();
}

QAbstractSocket::SocketState WebSocket::state() const
{
	return m_socket->state();
}

QString WebSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress WebSocket::peerAddress() const
{
	return m_socket->peerAddress();
}

quint16 WebSocket::peerPort() const
{
	return m_socket->peerPort();
}

void WebSocket::ignoreSslErrors()
{
#ifndef QT_NO_SSL
	m_socket->ignoreSslErrors();
#endif
}

void WebSocket::flushWriteBuffer()
{
	frameWriter().flushToWebSocket(m_socket);
	m_socket->flush();
}

void WebSocket::onTextMessageReceived(const QString &message)
{
	shvDebug() << "text message received:" << message;
	auto data = message.toUtf8();
	try {
		for (auto rqid : frameReader().addData(std::string_view(data.constData(), data.size()))) {
			emit responseMetaReceived(rqid);
		}
		emit dataChunkReceived();
		if (!frameReader().isEmpty()) {
			emit readyRead();
		}
	}
	catch (const std::runtime_error &e) {
		shvWarning() << "Corrupted meta data received:" << e.what() << "\n" << shv::chainpack::utils::hexDump(std::string_view(data.constData(), std::min(data.size(), static_cast<decltype(data.size())>(64))));
	}
}

void WebSocket::onBinaryMessageReceived(const QByteArray &message)
{
	try {
		shvDebug() << "binary message received:" << message;
		for (auto rqid : frameReader().addData(std::string_view(message.constData(), message.size()))) {
			emit responseMetaReceived(rqid);
		}
		emit dataChunkReceived();
		if (!frameReader().isEmpty()) {
			emit readyRead();
		}
	}
	catch (const std::runtime_error &e) {
		shvWarning() << "Corrupted meta data received:" << e.what() << "\n" << shv::chainpack::utils::hexDump(std::string_view(message.constData(), std::min(message.size(), static_cast<decltype(message.size())>(64))));
	}
}

} // namespace shv
