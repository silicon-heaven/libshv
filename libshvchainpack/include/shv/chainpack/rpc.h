#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Rpc
{
public:
	enum class ProtocolType {Invalid = 0, ChainPack, Cpon};
	static const char* protocolTypeToString(ProtocolType pv);

	static constexpr auto OPT_IDLE_WD_TIMEOUT = "idleWatchDogTimeOut";

	static constexpr auto KEY_OPTIONS = "options";
	static constexpr auto KEY_CLIENT_ID = "clientId";
	static constexpr auto KEY_MOUT_POINT = "mountPoint";
	static constexpr auto KEY_SHV_PATH = "shvPath";
	static constexpr auto KEY_DEVICE_ID = "deviceId";
	static constexpr auto KEY_DEVICE = "device";
	static constexpr auto KEY_BROKER = "broker";
	static constexpr auto KEY_TUNNEL = "tunnel";
	static constexpr auto KEY_LOGIN = "login";
	static constexpr auto KEY_SECRET = "secret";
	static constexpr auto KEY_SHV_USER = "shvUser";
	static constexpr auto KEY_BROKER_ID = "brokerId";

	static constexpr auto METH_HELLO = "hello";
	static constexpr auto METH_LOGIN = Rpc::KEY_LOGIN;

	static constexpr auto METH_GET = "get";
	static constexpr auto METH_SET = "set";
	static constexpr auto METH_DIR = "dir";
	static constexpr auto METH_LS = "ls";
	static constexpr auto METH_TAGS = "tags";
	static constexpr auto METH_PING = "ping";
	static constexpr auto METH_ECHO = "echo";
	static constexpr auto METH_APP_NAME = "appName";
	static constexpr auto METH_APP_VERSION = "appVersion";
	static constexpr auto METH_GIT_COMMIT = "gitCommit";
	static constexpr auto METH_DEVICE_ID = "deviceId";
	static constexpr auto METH_DEVICE_TYPE = "deviceType";
	static constexpr auto METH_CLIENT_ID = "clientId";
	static constexpr auto METH_MOUNT_POINT = "mountPoint";
	static constexpr auto METH_SUBSCRIBE = "subscribe";
	static constexpr auto METH_UNSUBSCRIBE = "unsubscribe";
	static constexpr auto METH_REJECT_NOT_SUBSCRIBED = "rejectNotSubscribed";
	static constexpr auto METH_RUN_CMD = "runCmd";
	static constexpr auto METH_RUN_SCRIPT = "runScript";
	static constexpr auto METH_LAUNCH_REXEC = "launchRexec";
	static constexpr auto METH_HELP = "help";
	static constexpr auto METH_GET_LOG = "getLog";

	static constexpr auto PAR_PATHS = "paths";
	static constexpr auto PAR_METHODS = "methods";

	static constexpr auto PAR_PATH = "path";
	static constexpr auto PAR_METHOD = "method";
	static constexpr auto PAR_PARAMS = "params";
	static constexpr auto PAR_SOURCE = "source";

	static constexpr auto SIG_VAL_CHANGED = "chng";
	static constexpr auto SIG_VAL_FASTCHANGED = "fastchng";
	static constexpr auto SIG_SERVICE_VAL_CHANGED = "svcchng";
	static constexpr auto SIG_MOUNTED_CHANGED = "mntchng";
	static constexpr auto SIG_COMMAND_LOGGED = "cmdlog";

	static constexpr auto ROLE_BROWSE = "bws";
	static constexpr auto ROLE_READ = "rd";
	static constexpr auto ROLE_WRITE = "wr";
	static constexpr auto ROLE_COMMAND = "cmd";
	static constexpr auto ROLE_CONFIG = "cfg";
	static constexpr auto ROLE_SERVICE = "srv";
	static constexpr auto ROLE_SUPER_SERVICE = "ssrv";
	static constexpr auto ROLE_DEVEL = "dev";
	static constexpr auto ROLE_ADMIN = "su";

	static constexpr auto ROLE_MASTER_BROKER = "masterBroker";

	static constexpr auto DIR_APP = ".app";
	static constexpr auto DIR_BROKER = ".broker";
	static constexpr auto DIR_BROKER_APP = ".broker/app";
	static constexpr auto DIR_CLIENTS = "clients";
	static constexpr auto DIR_MASTERS = "masters";

	static constexpr auto DIR_APP_BROKER = ".app/broker";
	static constexpr auto DIR_APP_BROKER_CURRENTCLIENT = ".app/broker/currentClient";

	static constexpr auto JSONRPC_REQUEST_ID = "id";
	static constexpr auto JSONRPC_METHOD = PAR_METHOD;
	static constexpr auto JSONRPC_PARAMS = PAR_PARAMS;
	static constexpr auto JSONRPC_RESULT = "result";
	static constexpr auto JSONRPC_ERROR = "error";
	static constexpr auto JSONRPC_SHV_PATH = PAR_PATH;
	static constexpr auto JSONRPC_CALLER_ID = "cid";
	static constexpr auto JSONRPC_REV_CALLER_ID = "rcid";
};

} // namespace chainpack
} // namespace shv
