#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::ClientAppCliOptions;

public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER(std::string, p, setP, ath)
	CLIOPTION_GETTER_SETTER(std::string, m, setM, ethod)
	CLIOPTION_GETTER_SETTER(std::string, p, setP, arams)
	CLIOPTION_GETTER_SETTER(bool, isC, setC, hainPackOutput)
	CLIOPTION_GETTER_SETTER(bool, isC, setC, ponOutput)
	CLIOPTION_GETTER_SETTER(bool, s, setS, houldSubscribe)
	CLIOPTION_GETTER_SETTER(std::string, s, setS, ubscribeFormat)

	CLIOPTION_GETTER_SETTER(bool, s, setS, houldEmit)
};
