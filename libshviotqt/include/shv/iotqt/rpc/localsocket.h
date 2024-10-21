#pragma once

#include <shv/iotqt/rpc/socket.h>

class QLocalSocket;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT LocalSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	enum class Protocol {Stream, Serial};
	LocalSocket(QLocalSocket *socket, Protocol protocol, QObject *parent = nullptr);
	~LocalSocket() override;

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;

	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
protected:
	void onDataReadyRead();
	void flushWriteBuffer() override;
protected:
	QLocalSocket *m_socket = nullptr;
};

} // namespace shv::iotqt::rpc
