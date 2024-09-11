#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QSslError;
class QTcpSocket;
class QThread;

namespace shv::iotqt::rpc {

class Socket;

class SHVIOTQT_DECL_EXPORT SocketRpcConnection : public QObject, public shv::chainpack::IRpcConnection, public shv::chainpack::RpcDriver
{
	Q_OBJECT

public:
	explicit SocketRpcConnection(QObject *parent = nullptr);
	~SocketRpcConnection() Q_DECL_OVERRIDE;

	void setSocket(Socket *socket);
	bool hasSocket() const;

	void connectToHost(const QUrl &url);

	void sendRpcMessage(const chainpack::RpcMessage &rpc_msg) override;

	void closeSocket();
	void abortSocket();

	bool isSocketConnected() const;
	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	Q_SIGNAL void socketError(const QString &error_msg);
	Q_SIGNAL void sslErrors(const QList<QSslError> &errors);

	// following two signals allows correct RPC call timeout implementation
	Q_SIGNAL void responseMetaReceived(int request_id);
	Q_SIGNAL void dataChunkReceived();

	void ignoreSslErrors();

	std::string peerAddress() const;
	int peerPort() const;
protected:
	// RpcDriver interface
	bool isOpen() Q_DECL_OVERRIDE;
	void writeFrameData(const std::string &frame_data) override;

	Socket* socket();
	void onReadyRead();

	void onParseDataException(const shv::chainpack::ParseException &e) override;
protected:
	Socket *m_socket = nullptr;
};

}

