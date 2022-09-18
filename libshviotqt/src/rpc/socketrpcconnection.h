#pragma once

#include "../shviotqtglobal.h"
#include "rpc.h"

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QSslError;
class QTcpSocket;
class QThread;

//namespace shv { namespace chainpack { class RpcRequest; class RpcResponse; }}

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
	bool hasSocket() const {return m_socket != nullptr;}
	void setProtocolTypeAsInt(int v) {shv::chainpack::RpcDriver::setProtocolType((shv::chainpack::Rpc::ProtocolType)v);}

	void connectToHost(const QUrl &url);

	Q_SIGNAL void rpcValueReceived(shv::chainpack::RpcValue rpc_val);
	Q_SLOT void sendRpcValue(const shv::chainpack::RpcValue &rpc_val) {shv::chainpack::RpcDriver::sendRpcValue(rpc_val);}

	void closeSocket();
	void abortSocket();

	bool isSocketConnected() const;
	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	Q_SIGNAL void socketError(const QString &error_msg);
	Q_SIGNAL void sslErrors(const QList<QSslError> &errors);
	void ignoreSslErrors();

	std::string peerAddress() const;
	int peerPort() const;
public:
	//Q_SLOT void sendRpcRequestSync_helper(const shv::chainpack::RpcRequest& request, shv::chainpack::RpcResponse *presponse, int time_out_ms);
protected:
	// RpcDriver interface
	bool isOpen() Q_DECL_OVERRIDE;
	int64_t writeBytes(const char *bytes, size_t length) Q_DECL_OVERRIDE;
	void writeMessageBegin() override;
	void writeMessageEnd() override;
	//bool flush() Q_DECL_OVERRIDE;

	Socket* socket();
	void onReadyRead();
	void onBytesWritten();

	void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val) override;
	void onParseDataException(const shv::chainpack::ParseException &e) override;
protected:
	Socket *m_socket = nullptr;
};

}}}

