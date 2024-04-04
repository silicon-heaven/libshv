#pragma once

#include "commonrpcclienthandle.h"

#include <shv/iotqt/rpc/serverconnection.h>

#include <QVector>

class QTimer;

namespace shv { namespace core { namespace utils { class ShvUrl; }}}
namespace shv { namespace iotqt { namespace rpc { class Socket; }}}
namespace shv { namespace iotqt { namespace node { class ShvNode; }}}

namespace shv {
namespace broker {
namespace rpc {

class ClientConnectionOnBroker : public shv::iotqt::rpc::ServerConnection, public CommonRpcClientHandle
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::ServerConnection;
public:
	ClientConnectionOnBroker(shv::iotqt::rpc::Socket* socket, QObject *parent = nullptr);
	~ClientConnectionOnBroker() override;

	int connectionId() const override;
	bool isConnectedAndLoggedIn() const override;
	bool isSlaveBrokerConnection() const override;
	bool isMasterBrokerConnection() const override;

	std::string loggedUserName() override;

	shv::chainpack::RpcValue tunnelOptions() const;
	shv::chainpack::RpcValue deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setMountPoint(const std::string &mp);
	const std::string& mountPoint() const;

	int idleTime() const;
	int idleTimeMax() const;

	void setIdleWatchDogTimeOut(int sec);

	void sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void sendRpcFrame(shv::chainpack::RpcFrame &&frame) override;

	Subscription createSubscription(const std::string &shv_path, const std::string &method, const std::string& source) override;
	std::string toSubscribedPath(const std::string &signal_path) const override;
	void propagateSubscriptionToSlaveBroker(const Subscription &subs);

	void setLoginResult(const chainpack::UserLoginResult &result) override;
private:
	void onSocketConnectedChanged(bool is_connected);
	void onRpcFrameReceived(chainpack::RpcFrame &&frame) override;
	bool checkTunnelSecret(const std::string &s);
	QVector<int> callerIdsToList(const shv::chainpack::RpcValue &caller_ids);

	void processLoginPhase() override;
private:
	QTimer *m_idleWatchDogTimer = nullptr;
	std::string m_mountPoint;
};

}}}
