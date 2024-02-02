#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/clioptions.h>

#include <QSet>

namespace shv {
namespace broker {

class SHVBROKER_DECL_EXPORT AppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;
public:
	AppCliOptions();
	~AppCliOptions() override = default;

	CLIOPTION_GETTER_SETTER(std::string, l, setL, ocale)
	CLIOPTION_GETTER_SETTER2(std::string, "app.brokerId", b, setB, rokerId)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)
	CLIOPTION_GETTER_SETTER2(int, "server.sslPort", s, setS, erverSslPort)
	CLIOPTION_GETTER_SETTER2(int, "server.discoveryPort", d, setD, iscoveryPort)
#ifdef WITH_SHV_WEBSOCKETS
	CLIOPTION_GETTER_SETTER2(int, "server.websocket.port", s, setS, erverWebsocketPort)
	CLIOPTION_GETTER_SETTER2(int, "server.websocket.sslport", s, setS, erverWebsocketSslPort)
#endif
	CLIOPTION_GETTER_SETTER2(std::string, "server.ssl.key", s, setS, erverSslKeyFile)
	CLIOPTION_GETTER_SETTER2(std::string, "server.ssl.cert", s, setS, erverSslCertFiles)
	CLIOPTION_GETTER_SETTER2(std::string, "server.publicIP", p, setP, ublicIP)

	CLIOPTION_GETTER_SETTER2(bool, "sqlconfig.enabled", is, set, SqlConfigEnabled)
	CLIOPTION_GETTER_SETTER2(std::string, "sqlconfig.driver", s, setS, qlConfigDriver)
	CLIOPTION_GETTER_SETTER2(std::string, "sqlconfig.database", s, setS, qlConfigDatabase)

	CLIOPTION_GETTER_SETTER2(shv::chainpack::RpcValue, "masters.connections", m, setM, asterBrokersConnections)
	CLIOPTION_GETTER_SETTER2(bool, "masters.enabled", is, set, MasterBrokersEnabled)

	CLIOPTION_GETTER_SETTER2(std::string, "ldap.username", l, setL, dapUsername)
	CLIOPTION_GETTER_SETTER2(std::string, "ldap.password", l, setL, dapPassword)
	CLIOPTION_GETTER_SETTER2(std::string, "ldap.hostname", l, setL, dapHostname)
	CLIOPTION_GETTER_SETTER2(std::string, "ldap.searchBaseDN", l, setL, dapSearchBaseDN)
	CLIOPTION_GETTER_SETTER2(chainpack::RpcValue::List, "ldap.searchAttrs", l, setL, dapSearchAttrs)
	CLIOPTION_GETTER_SETTER2(chainpack::RpcValue::List, "ldap.groupMapping", l, setL, dapGroupMapping)
	CLIOPTION_GETTER_SETTER2(chainpack::RpcValue::List, "azure.groupMapping", a, setA, zureGroupMapping)
};

}}
