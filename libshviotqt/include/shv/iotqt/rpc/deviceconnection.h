#pragma once

#include <shv/iotqt/rpc/clientconnection.h>

namespace shv::iotqt::rpc {

class DeviceAppCliOptions;

class LIBSHVIOTQT_EXPORT DeviceConnection : public ClientConnection
{
	Q_OBJECT
	using Super = ClientConnection;
public:
	DeviceConnection(const std::string& user_agent, QObject *parent = nullptr);
	// User agent can't be constructed from nullptr.
	DeviceConnection(std::nullptr_t) = delete;

	const shv::chainpack::RpcValue::Map& deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setCliOptions(const DeviceAppCliOptions *cli_opts);
};

} // namespace shv::iotqt::rpc


