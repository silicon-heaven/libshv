#pragma once

#include "clientappclioptions.h"
#include "clientconnection.h"

namespace shv {
namespace iotqt {
namespace rpc {

class DeviceAppCliOptions;

class SHVIOTQT_DECL_EXPORT DeviceConnection : public ClientConnection
{
	Q_OBJECT
	using Super = ClientConnection;
public:
	DeviceConnection(QObject *parent = nullptr);

	const shv::chainpack::RpcValue::Map& deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setCliOptions(const DeviceAppCliOptions *cli_opts);
protected:
	//shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) override;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
