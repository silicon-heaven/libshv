#ifndef SHV_IOTQT_RPC_WEBSOCKET_H
#define SHV_IOTQT_RPC_WEBSOCKET_H

#include "socket.h"

class QWebSocket;

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT WebSocket : public Socket
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
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 data_size) override;
	void writeMessageBegin() override;
	void writeMessageEnd() override;
	void ignoreSslErrors() override;
private:
	void onTextMessageReceived(const QString &message);
	void onBinaryMessageReceived(const QByteArray &message);
private:
	QWebSocket *m_socket = nullptr;
	QByteArray m_readBuffer;
	QByteArray m_writeBuffer;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_RPC_WEBSOCKET_H
