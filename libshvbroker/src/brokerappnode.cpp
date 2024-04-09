#include "brokerappnode.h"
#include "rpc/masterbrokerconnection.h"

#include <shv/broker/brokerapp.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/core/log.h>
#include <shv/iotqt/rpc/rpccall.h>

#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)

namespace cp = shv::chainpack;

namespace shv::broker {

namespace {
const auto M_GET_VERBOSITY = "verbosity";
const auto M_SET_VERBOSITY = "setVerbosity";
const auto M_GET_SEND_LOG_SIGNAL_ENABLED = "getSendLogAsSignalEnabled";
const auto M_SET_SEND_LOG_SIGNAL_ENABLED = "setSendLogAsSignalEnabled";
class BrokerLogNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	BrokerLogNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("log", &m_metaMethods, parent)
		, m_metaMethods {
			shv::chainpack::methods::DIR,
			shv::chainpack::methods::LS,
			{M_GET_SEND_LOG_SIGNAL_ENABLED, cp::MetaMethod::Flag::IsGetter, {}, "Bool", shv::chainpack::AccessLevel::Read},
			{M_SET_SEND_LOG_SIGNAL_ENABLED, cp::MetaMethod::Flag::IsSetter, "Bool", "Bool", shv::chainpack::AccessLevel::Write},
			{M_GET_VERBOSITY, cp::MetaMethod::Flag::IsGetter, {}, "String", shv::chainpack::AccessLevel::Read},
			{M_SET_VERBOSITY, cp::MetaMethod::Flag::IsSetter, "Bool", "String", shv::chainpack::AccessLevel::Command},
		}
	{ }

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override
	{
		if(shv_path.empty()) {
			if(method == M_GET_SEND_LOG_SIGNAL_ENABLED) {
				return BrokerApp::instance()->isSendLogAsSignalEnabled();
			}
			if(method == M_SET_SEND_LOG_SIGNAL_ENABLED) {
				BrokerApp::instance()->setSendLogAsSignalEnabled(params.toBool());
				return true;
			}
			if(method == M_GET_VERBOSITY) {
				return NecroLog::topicsLogThresholds();
			}
			if(method == M_SET_VERBOSITY) {
				const std::string &s = params.asString();
				NecroLog::setTopicsLogThresholds(s);
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params, user_id);
	}
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};
}

static const auto M_RELOAD_CONFIG = "reloadConfig";
static const auto M_RESTART = "restart";
static const auto M_APP_VERSION = "appVersion";
static const auto M_GIT_COMMIT = "gitCommit";
static const auto M_SHV_VERSION = "shvVersion";
static const auto M_SHV_GIT_COMMIT = "shvGitCommit";
static const auto M_BROKER_ID = "brokerId";
static const auto M_MASTER_BROKER_ID = "masterBrokerId";

BrokerAppNode::BrokerAppNode(shv::iotqt::node::ShvNode *parent)
	: Super("", &m_metaMethods, parent)
	, m_metaMethods {
		shv::chainpack::methods::DIR,
		shv::chainpack::methods::LS,
		{shv::chainpack::Rpc::METH_PING, shv::chainpack::MetaMethod::Flag::None, {}, {}, shv::chainpack::AccessLevel::Browse},
		{shv::chainpack::Rpc::METH_ECHO, shv::chainpack::MetaMethod::Flag::None, "RpcValue", "RpcValue", shv::chainpack::AccessLevel::Browse},
		{M_APP_VERSION, cp::MetaMethod::Flag::IsGetter, {}, "String", shv::chainpack::AccessLevel::Browse},
		{M_GIT_COMMIT, cp::MetaMethod::Flag::IsGetter, {}, "String", cp::AccessLevel::Read},
		{M_SHV_VERSION, cp::MetaMethod::Flag::IsGetter, {}, "String", cp::AccessLevel::Read},
		{M_SHV_GIT_COMMIT, cp::MetaMethod::Flag::IsGetter, {}, "String", cp::AccessLevel::Read},
		{M_BROKER_ID, cp::MetaMethod::Flag::IsGetter, {}, "String", cp::AccessLevel::Read},
		{M_MASTER_BROKER_ID, cp::MetaMethod::Flag::IsGetter, {}, "String", cp::AccessLevel::Read},
		{cp::Rpc::METH_SUBSCRIBE, cp::MetaMethod::Flag::None, "Bool", "RpcValue", cp::AccessLevel::Read},
		{cp::Rpc::METH_UNSUBSCRIBE, cp::MetaMethod::Flag::None, "Bool", "RpcValue", cp::AccessLevel::Read},
		{cp::Rpc::METH_REJECT_NOT_SUBSCRIBED, 0, "Bool", "RpcValue", cp::AccessLevel::Read},
		{M_RELOAD_CONFIG, cp::MetaMethod::Flag::None, {}, {}, cp::AccessLevel::Service},
		{M_RESTART, cp::MetaMethod::Flag::None, {}, {}, cp::AccessLevel::Service},
	}
{
	new BrokerLogNode(this);
}

chainpack::RpcValue BrokerAppNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	const cp::RpcValue::String shv_path = rq.shvPath().toString();
	if(shv_path.empty()) {
		const cp::RpcValue::String method = rq.method().toString();
		auto get_subscribe_params = [](const shv::chainpack::RpcValue::Map &pm) {
			std::string path;
			if(const auto &v = pm.value(cp::Rpc::PAR_PATHS); v.isString()) {
				path = v.asString();
			}
			else if(const auto &v1 = pm.value(cp::Rpc::PAR_PATH); v1.isString()) {
				path = v1.asString();
			}
			std::string signal_name;
			if(const auto &v = pm.value(cp::Rpc::PAR_SIGNAL); v.isString()) {
				signal_name = v.asString();
			}
			else if(const auto &v1 = pm.value(cp::Rpc::PAR_METHODS); v1.isString()) {
				signal_name = v1.asString();
			}
			else if(const auto &v2 = pm.value(cp::Rpc::PAR_METHOD); v2.isString()) {
				signal_name = v2.asString();
			}
			std::string source = pm.value(cp::Rpc::PAR_SOURCE).toString();
			return std::make_tuple(path, signal_name, source);
		};
		if(method == cp::Rpc::METH_SUBSCRIBE) {
			const shv::chainpack::RpcValue params = rq.params();
			const shv::chainpack::RpcValue::Map &pm = params.asMap();
			auto [path, signal_name, source] = get_subscribe_params(pm);
			int client_id = rq.peekCallerId();
			BrokerApp::instance()->addSubscription(client_id, path, signal_name, source);
			return true;
		}
		if(method == cp::Rpc::METH_UNSUBSCRIBE) {
			const shv::chainpack::RpcValue parms = rq.params();
			const shv::chainpack::RpcValue::Map &pm = parms.asMap();
			auto [path, signal_name, source] = get_subscribe_params(pm);
			int client_id = rq.peekCallerId();
			return BrokerApp::instance()->removeSubscription(client_id, path, signal_name, source);
		}
		if(method == cp::Rpc::METH_REJECT_NOT_SUBSCRIBED) {
			const shv::chainpack::RpcValue parms = rq.params();
			const shv::chainpack::RpcValue::Map &pm = parms.asMap();
			auto [path, signal_name, source] = get_subscribe_params(pm);
			int client_id = rq.peekCallerId();
			return BrokerApp::instance()->rejectNotSubscribedSignal(client_id, path, signal_name, source);
		}
		if (method == M_MASTER_BROKER_ID) {
			int client_id = rq.peekCallerId();
			auto *conn = BrokerApp::instance()->masterBrokerConnectionForClient(client_id);
			if (!conn) {
				return std::string();
			}
			auto *rpc_call = iotqt::rpc::RpcCall::create(conn)->setShvPath(".broker/app")->setMethod("brokerId");
			connect(rpc_call, &iotqt::rpc::RpcCall::maybeResult, this, [this, rq](const ::shv::chainpack::RpcValue &result, const shv::chainpack::RpcError &error) {
				cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
				if (error.isValid()) {
					resp.setError(error);
				}
				else {
					resp.setResult(result);
				}
				rootNode()->emitSendRpcMessage(resp);
			});
			rpc_call->start();
			return cp::RpcValue();
		}
	}
	return Super::callMethodRq(rq);
}

shv::chainpack::RpcValue BrokerAppNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_PING) {
			return true;
		}
		if(method == cp::Rpc::METH_ECHO) {
			return params.isValid()? params: nullptr;
		}
		if(method == M_APP_VERSION) {
			return BrokerApp::applicationVersion().toStdString();
		}
		if(method == M_GIT_COMMIT) {
#ifdef GIT_COMMIT
			return SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#else
			return "N/A";
#endif
		}
		if(method == M_SHV_VERSION) {
#ifdef SHV_VERSION
			return SHV_EXPAND_AND_QUOTE(SHV_VERSION);
#else
			return "N/A";
#endif
		}
		if(method == M_SHV_GIT_COMMIT) {
#ifdef SHV_GIT_COMMIT
			return SHV_EXPAND_AND_QUOTE(SHV_GIT_COMMIT);
#else
			return "N/A";
#endif
		}
		if(method == M_BROKER_ID) {
			return BrokerApp::instance()->brokerId();
		}
		if(method == M_RELOAD_CONFIG) {
			QTimer::singleShot(500, BrokerApp::instance(), &BrokerApp::reloadConfigRemountDevices);
			return true;
		}
		if(method == M_RESTART) {
			shvInfo() << "Server restart requested via RPC.";
			QTimer::singleShot(500, BrokerApp::instance(), &BrokerApp::quit);
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

}
