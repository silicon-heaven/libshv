#pragma once

#include "socket.h"

class QWebSocket;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT WebSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	WebSocket(QWebSocket *socket, QObject *parent = nullptr);

	std::string readFrameData() override;
	void writeFrameData(const std::string &frame_data) override;

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
private:
	void flushWriteBuffer();
	void onTextMessageReceived(const QString &message);
	void onBinaryMessageReceived(const QByteArray &message);
private:
	QWebSocket *m_socket = nullptr;

	StreamFrameReader m_frameReader;
	StreamFrameWriter m_frameWriter;
};

} // namespace shv::iotqt::rpc
