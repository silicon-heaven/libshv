#include <shv/iotqt/rpc/clientappclioptions.h>

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;

namespace shv::iotqt::rpc {

ClientAppCliOptions::ClientAppCliOptions()
{
	addOption("login.user").setType(cp::RpcValue::Type::String).setNames("-u", "--user").setComment("Login user");
	addOption("login.password").setType(cp::RpcValue::Type::String).setNames("--password").setComment("Login password");
	addOption("login.passwordFile").setType(cp::RpcValue::Type::String).setNames("--password-file").setComment("Login password file");
	addOption("login.type").setType(cp::RpcValue::Type::String).setNames("--lt", "--login-type").setComment("Login type [NONE | PLAIN | SHA1 | RSAOAEP]").setDefaultValue("SHA1");
	addOption("server.host").setType(cp::RpcValue::Type::String).setNames("-s", "--server-host").setComment("server host").setDefaultValue("localhost");
	addOption("server.peerVerify").setType(cp::RpcValue::Type::Bool).setNames("--peer-verify").setComment("Verify peer's identity when establishing secured connection").setDefaultValue(true);

	addOption("rpc.rpcTimeout").setType(cp::RpcValue::Type::Int).setNames("--rto", "--rpc-time-out").setComment("Set default RPC calls timeout [sec].").setDefaultValue(shv::chainpack::RpcDriver::defaultRpcTimeoutMsec() / 1000);
	addOption("rpc.reconnectInterval").setType(cp::RpcValue::Type::Int).setNames("--rci", "--rpc-reconnect-interval").setComment("Reconnect to broker if connection lost at least after recoonect-interval seconds. Disabled when set to 0").setDefaultValue(10);
	addOption("rpc.heartbeatInterval").setType(cp::RpcValue::Type::Int).setNames("--hbi", "--rpc-heartbeat-interval").setComment("Send heart beat to broker every n sec. Disabled when set to 0").setDefaultValue(60);
}

} // namespace shv
