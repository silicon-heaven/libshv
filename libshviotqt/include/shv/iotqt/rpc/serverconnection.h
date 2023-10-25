#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcconnection.h"

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcmessage.h>

#include <shv/core/utils.h>

#include <QObject>

#include <string>

namespace shv {
namespace iotqt {
namespace rpc {

class Socket;

class SHVIOTQT_DECL_EXPORT ServerConnection : public SocketRpcConnection
{
	Q_OBJECT

	using Super = SocketRpcConnection;
public:
	explicit ServerConnection(Socket *socket, QObject *parent = nullptr);
	~ServerConnection() Q_DECL_OVERRIDE;

	const std::string& connectionName();
	void setConnectionName(const std::string &n);

	void close() Q_DECL_OVERRIDE;
	void abort() Q_DECL_OVERRIDE;

	void unregisterAndDeleteLater();
	Q_SIGNAL void aboutToBeDeleted(int connection_id);

	const shv::chainpack::RpcValue::Map& connectionOptions() const;

	const std::string& userName() const;

	virtual bool isConnectedAndLoggedIn() const;

	virtual bool isSlaveBrokerConnection() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, std::string &&msg_data) override;

	bool isLoginPhase() const;
	void processLoginPhase(const chainpack::RpcMessage &msg);

	virtual void processLoginPhase();
	virtual void setLoginResult(const shv::chainpack::UserLoginResult &result);

protected:
	std::string m_connectionName;
	shv::chainpack::UserLogin m_userLogin;
	shv::chainpack::UserLoginContext m_userLoginContext;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
	bool m_loginOk = false;

	shv::chainpack::RpcValue m_connectionOptions;
};

}}}

