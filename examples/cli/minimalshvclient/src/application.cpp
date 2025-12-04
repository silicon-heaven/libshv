#include "application.h"

#include <shv/chainpack/rpc.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/clientappclioptions.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/coreqt/log.h>

#include <QTimer>

using namespace std;
using namespace shv::chainpack;
using namespace shv::iotqt;
using namespace shv::iotqt::rpc;

Application::Application(int &argc, char **argv, const shv::iotqt::rpc::ClientAppCliOptions &cli_opts)
	: Super(argc, argv)
{
	m_rpcConnection = new ClientConnection(shv::core::utils::makeUserAgent("minimalshvclient"), this);

	connect(m_rpcConnection, &ClientConnection::brokerConnectedChanged, this, [this](bool is_connected) {
		if(is_connected) {
			shvInfo() << "Subscribing INT property changes.";
			subscribeChanges("test/someInt");
			shvInfo() << "Changing INT property value, you should get notification.";
			callShvMethod("test/someInt", "set", 123);
			shvInfo() << "Reading INT property new value back.";
			callShvMethod("test/someInt", "get", {}, this, [this](const RpcValue &result) {
				shvInfo() << "Value set previously:" << result.toInt();
				callShvMethod("test/someInt", "set", 42);
			});
		}
	});
	connect(m_rpcConnection, &ClientConnection::rpcMessageReceived, this, &Application::onRpcMessageReceived);

	if(cli_opts.serverHost_isset() || cli_opts.user_isset() || cli_opts.password_isset())
		m_rpcConnection->setCliOptions(&cli_opts);
	else
		m_rpcConnection->setConnectionString("tcp://test:test@localhost");
	m_rpcConnection->open();
}

void Application::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		RpcRequest rq(msg);
		shvMessage() << "[RQ ] ===> RPC request received:" << rq.toPrettyString();
	}
	else if(msg.isResponse()) {
		RpcResponse rp(msg);
		shvMessage() << "[RSP] ===> RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		RpcSignal nt(msg);
		shvMessage() << "[SIG] ===> RPC signal received:" << nt.toPrettyString();
	}
}

void Application::subscribeChanges(const string &shv_path)
{
	auto params = RpcValue::Map {
		{Rpc::PAR_PATH, shv_path},
		{Rpc::PAR_METHOD, Rpc::SIG_VAL_CHANGED},
	};
	callShvMethod(Rpc::DIR_BROKER_APP, Rpc::METH_SUBSCRIBE, params);
}

void Application::unsubscribeChanges(const string &shv_path)
{
	auto params = RpcValue::Map {
		{Rpc::PAR_PATH, shv_path},
		{Rpc::PAR_METHOD, Rpc::SIG_VAL_CHANGED},
	};
	callShvMethod(Rpc::DIR_BROKER_APP, Rpc::METH_UNSUBSCRIBE, params);
}

void Application::callShvMethod(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const QObject *context
								, const std::function<void (const shv::chainpack::RpcValue &)>& success_callback
								, const std::function<void (const shv::chainpack::RpcError &)>& error_callback)
{
	auto *rpcc = RpcCall::create(m_rpcConnection)
			->setShvPath(shv_path)
			->setMethod(method)
			->setParams(params);
	connect(rpcc, &shv::iotqt::rpc::RpcCall::maybeResult, context? context: this, [success_callback, error_callback, shv_path, method](const ::shv::chainpack::RpcValue &result, const shv::chainpack::RpcError &error) {
		if(error.isValid()) {
			if(error_callback)
				error_callback(error);
			else
				shvError() << "method:" << method << "on path:" << shv_path << "call error:" << error.toString();
		}
		else {
			if(success_callback)
				success_callback(result);
			else
				shvInfo() << "method:" << method << "on path:" << shv_path << "call success, resut:" << result.toCpon();
		}
	});
	// rpccc delete itself later, after maybeResult is emited
	rpcc->start();
}
