#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QSslError;
class QTcpSocket;
class QThread;

namespace shv {
namespace iotqt {
namespace rpc {

class Socket;

class SHVIOTQT_DECL_EXPORT SocketRpcConnection : public QObject, public shv::chainpack::IRpcConnection, public shv::chainpack::RpcDriver
{
	Q_OBJECT

public:
	explicit SocketRpcConnection(QObject *parent = nullptr);
	~SocketRpcConnection() Q_DECL_OVERRIDE;

	void setSocket(Socket *socket);
	bool hasSocket() const;
	void setProtocolTypeAsInt(int v);

	void connectToHost(const QUrl &url);

	Q_SLOT void sendRpcValue(const shv::chainpack::RpcValue &rpc_val);

	void closeSocket();
	void abortSocket();
	void clearSendBuffers() override;

	bool isSocketConnected() const;
	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	Q_SIGNAL void socketError(const QString &error_msg);
	Q_SIGNAL void sslErrors(const QList<QSslError> &errors);
	Q_SIGNAL void socketDataReadyRead();
	void ignoreSslErrors();

	std::string peerAddress() const;
	int peerPort() const;
protected:
	// RpcDriver interface
	bool isOpen() Q_DECL_OVERRIDE;
	int64_t writeBytes(const char *bytes, size_t length) Q_DECL_OVERRIDE;
	void writeMessageBegin() override;
	void writeMessageEnd() override;

	Socket* socket();
	void onReadyRead();
	void onBytesWritten();

	void onParseDataException(const shv::chainpack::ParseException &e) override;
protected:
	Socket *m_socket = nullptr;
};

}}}

