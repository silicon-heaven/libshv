#include "rpc/clientconnectiononbroker.h"
#include <shv/broker/clientconnectionnode.h>
#include <shv/broker/brokerapp.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

namespace shv::broker {

static const auto M_MOUNT_POINT = "mountPoint";
static const auto M_DROP_CLIENT = "dropClient";
static const auto M_USER_NAME = "userName";
static const auto M_USER_ROLES = "userRoles";
static const auto M_USER_PROFILE = "userProfile";
static const auto M_IDLE_TIME = "idleTime";
static const auto M_IDLE_TIME_MAX = "idleTimeMax";

//=================================================================================
// MasterBrokerConnectionNode
//=================================================================================
const static std::vector<cp::MetaMethod> meta_methods {
	{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Flag::None, "param", "ret", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Flag::None, "param", "ret", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{M_USER_NAME, cp::MetaMethod::Flag::IsGetter, "void", "ret", cp::MetaMethod::AccessLevel::Service},
	{M_USER_ROLES, cp::MetaMethod::Flag::None, "void", "ret", cp::MetaMethod::AccessLevel::Service},
	{M_USER_PROFILE, cp::MetaMethod::Flag::None, "void", "ret", cp::MetaMethod::AccessLevel::Service},
	{M_MOUNT_POINT, cp::MetaMethod::Flag::IsGetter, "void", "ret", cp::MetaMethod::AccessLevel::Service},
	{M_DROP_CLIENT, cp::MetaMethod::Flag::None, "void", "void", cp::MetaMethod::AccessLevel::Service},
	{M_IDLE_TIME, cp::MetaMethod::Flag::None, "ret", "void", cp::MetaMethod::AccessLevel::Service, {}, "Connection inactivity time in msec."},
	{M_IDLE_TIME_MAX, cp::MetaMethod::Flag::None, "void", "ret", cp::MetaMethod::AccessLevel::Service, {}, "Maximum connection inactivity time in msec, before it is closed by server."},
};

ClientConnectionNode::ClientConnectionNode(int client_id, shv::iotqt::node::ShvNode *parent)
	: Super(std::to_string(client_id), &meta_methods, parent)
	, m_clientId(client_id)
{
}

shv::chainpack::RpcValue ClientConnectionNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_USER_NAME) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				return cli->loggedUserName();
			}
			return nullptr;
		}
		if(method == M_USER_ROLES) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				const std::string user_name = cli->loggedUserName();
				auto user_def = BrokerApp::instance()->aclManager()->user(user_name);
				auto roles = BrokerApp::instance()->aclManager()->userFlattenRoles(user_name, user_def.roles);
				cp::RpcValue::List ret;
				std::copy(roles.begin(), roles.end(), std::back_inserter(ret));
				return ret;
			}
			return nullptr;
		}
		if(method == M_USER_PROFILE) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				auto ret = BrokerApp::instance()->aclManager()->userProfile(cli->loggedUserName());
				return ret.isValid()? ret: nullptr;
			}
			return nullptr;
		}
		if(method == M_MOUNT_POINT) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				return cli->mountPoint();
			}
			return nullptr;
		}
		if(method == M_IDLE_TIME) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli)
				return cli->idleTime();
			SHV_EXCEPTION("Invalid client id: " + std::to_string(m_clientId));
		}
		if(method == M_IDLE_TIME_MAX) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli)
				return cli->idleTimeMax();
			SHV_EXCEPTION("Invalid client id: " + std::to_string(m_clientId));
		}
		if(method == M_DROP_CLIENT) {
			rpc::ClientConnectionOnBroker *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				cli->close();
				return true;
			}
			return false;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

}
