#pragma once

#include "commonrpcclienthandle.h"

#include <shv/iotqt/rpc/deviceconnection.h>

namespace shv {
namespace broker {
namespace rpc {

class MasterBrokerConnection : public shv::iotqt::rpc::DeviceConnection, public CommonRpcClientHandle
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::DeviceConnection;
public:
	MasterBrokerConnection(QObject *parent = nullptr);

	int connectionId() const override;

	/// master broker connection cannot have userName
	/// because it is connected in oposite direction than client connections
	std::string loggedUserName() override;

	bool isConnectedAndLoggedIn() const override;
	bool isSlaveBrokerConnection() const override;
	bool isMasterBrokerConnection() const override;

	void sendFrame(shv::chainpack::RpcFrame &&frame) override;
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;

	Subscription createSubscription(const std::string &shv_path, const std::string &method) override;
	std::string toSubscribedPath(const Subscription &subs, const std::string &signal_path) const override;

	std::string masterExportedToLocalPath(const std::string &master_path) const;
	std::string localPathToMasterExported(const std::string &local_path) const;
	const std::string& exportedShvPath() const;

	void setOptions(const shv::chainpack::RpcValue &slave_broker_options);
	shv::chainpack::RpcValue options();
protected:
	void onRpcFrameReceived(shv::chainpack::RpcFrame &&frame) override;
protected:
	std::string m_exportedShvPath;
	shv::chainpack::RpcValue m_options;
};

}}}

