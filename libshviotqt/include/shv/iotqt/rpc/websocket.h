#pragma once

#include <shv/iotqt/rpc/socket.h>

class QWebSocket;

namespace shv::iotqt::rpc {

class LIBSHVIOTQT_EXPORT WebSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	WebSocket(QWebSocket *socket, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
private:
	void flushWriteBuffer() override;
	void onTextMessageReceived(const QString &message);
	void onBinaryMessageReceived(const QByteArray &message);
private:
	QWebSocket *m_socket = nullptr;
};

} // namespace shv::iotqt::rpc
