#pragma once

#include <shv/core/utils/clioptions.h>

#include <shv/iotqt/shviotqtglobal.h>

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT ClientAppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;
public:
	ClientAppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "login.user", u, setU, ser)
	CLIOPTION_GETTER_SETTER2(std::string, "login.password", p, setP, assword)
	CLIOPTION_GETTER_SETTER2(std::string, "login.passwordFile", p, setP, asswordFile)
	CLIOPTION_GETTER_SETTER2(std::string, "login.type", l, setL, oginType)
	CLIOPTION_GETTER_SETTER2(std::string, "server.host", s, setS, erverHost)
	CLIOPTION_GETTER_SETTER2(bool, "server.peerVerify", s, setS, erverPeerVerify)

	CLIOPTION_GETTER_SETTER2(int, "rpc.rpcTimeout", r, setR, pcTimeout)
	CLIOPTION_GETTER_SETTER2(int, "rpc.reconnectInterval", r, setR, econnectInterval)
	CLIOPTION_GETTER_SETTER2(int, "rpc.heartbeatInterval", h, setH, eartBeatInterval)
};

}

