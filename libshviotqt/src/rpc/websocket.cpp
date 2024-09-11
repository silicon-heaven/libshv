#include <shv/iotqt/rpc/websocket.h>

#include <shv/coreqt/log.h>

#include <QWebSocket>

namespace shv::iotqt::rpc {

WebSocket::WebSocket(QWebSocket *socket, QObject *parent)
	: Super(parent)
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
	m_socket->close();
}

void WebSocket::abort()
{
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

std::vector<std::string> WebSocket::takeFrames()
{
	return m_frameReader.takeFrames();
}

void WebSocket::writeFrameData(const std::string &frame_data)
{
	m_frameWriter.addFrame(frame_data);
	flushWriteBuffer();
}

void WebSocket::ignoreSslErrors()
{
#ifndef QT_NO_SSL
	m_socket->ignoreSslErrors();
#endif
}

void WebSocket::flushWriteBuffer()
{
	m_frameWriter.flushToWebSocket(m_socket);
	m_socket->flush();
}

void WebSocket::onTextMessageReceived(const QString &message)
{
	shvDebug() << "text message received:" << message;
	auto ba = message.toUtf8();
	for (auto rqid : m_frameReader.addData(std::string_view(ba.constData(), ba.size()))) {
		emit responseMetaReceived(rqid);
	}
	emit dataChunkReceived();
	if (!m_frameReader.isEmpty()) {
		emit readyRead();
	}
}

void WebSocket::onBinaryMessageReceived(const QByteArray &message)
{
	shvDebug() << "binary message received:" << message;
	for (auto rqid : m_frameReader.addData(std::string_view(message.constData(), message.size()))) {
		emit responseMetaReceived(rqid);
	}
	emit dataChunkReceived();
	if (!m_frameReader.isEmpty()) {
		emit readyRead();
	}
}

} // namespace shv
