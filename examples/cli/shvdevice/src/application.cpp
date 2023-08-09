#include "application.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <QTimer>

using namespace std;

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

static const std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::applicationName().toStdString();
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().asString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			Application *app = Application::instance();
			const auto opts = app->rpcConnection()->connectionOptions();
			const cp::RpcValue::Map& dev = opts.asMap().valref(cp::Rpc::KEY_DEVICE).asMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		if(rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "SampleShvClient";
		}
	}
	return Super::processRpcRequest(rq);
}

Application::Application(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_rpcConnection = new si::rpc::ClientConnection(this);
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &Application::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &Application::onRpcMessageReceived);

	auto root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);
	//m_shvTree->mkdir("sys/rproc");

	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);
}

Application::~Application()
{
	shvInfo() << "destroying shv agent application";
}

Application *Application::instance()
{
	return qobject_cast<Application *>(QCoreApplication::instance());
}

void Application::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
	if(is_connected) {
		QTimer::singleShot(0, this, [this]() {
			subscribeChanges();
			testRpcCall();
		});
	}
}

void Application::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvInfo() << "RPC signal received:" << nt.toPrettyString();
	}
}

static constexpr int RPC_CALLBACK_TIMEOUT = 2000;

void Application::testRpcCall() const
{
	using namespace shv::iotqt::rpc;
	auto *rpc_call = RpcCall::create(rpcConnection())
			->setShvPath("test")
			->setMethod("ls")
			->setTimeout(RPC_CALLBACK_TIMEOUT);
	connect(rpc_call, &RpcCall::maybeResult, [](const ::shv::chainpack::RpcValue &result, const shv::chainpack::RpcError &error) {
		if(error.isValid())
			shvError() << "RPC call error:" << error.toString();
		else
			shvInfo() << "Got RPC response, result:" << result.toCpon();
	});
	rpc_call->start();
}

void Application::subscribeChanges()
{
	using namespace shv::iotqt::rpc;
	QString shv_path = "test";
	QString signal_name = shv::chainpack::Rpc::SIG_VAL_CHANGED;
	auto *rpc_call = RpcCall::createSubscriptionRequest(rpcConnection(), shv_path, signal_name);
	connect(rpc_call, &RpcCall::maybeResult, this, [this, shv_path, signal_name](const ::shv::chainpack::RpcValue &result, const shv::chainpack::RpcError &error) {
		if(error.isValid()) {
			shvError() << "Signal:" << signal_name << "on SHV path:" << shv_path << "subscribe error:" << error.toString();
		}
		else {
			shvInfo() << "Signal:" << signal_name << "on SHV path:" << shv_path << "subscribed successfully" << result.toCpon();
			// generate data change without ret value check
			rpcConnection()->callShvMethod((shv_path + "/someInt").toStdString(), "set", 123);
			QTimer::singleShot(500, this, [this, shv_path]() {
				rpcConnection()->callShvMethod((shv_path + "/someInt").toStdString(), "set", 321);
			});
		}
	});
	rpc_call->start();
}

