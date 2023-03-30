#include "clientconnectionnode.h"
#include "brokerapp.h"
#include "rpc/clientconnectiononbroker.h"

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
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
	{M_USER_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_SERVICE},
	{M_USER_ROLES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
	{M_USER_PROFILE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
	{M_MOUNT_POINT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_SERVICE},
	{M_DROP_CLIENT, cp::MetaMethod::Signature::VoidVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
	{M_IDLE_TIME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE, "Connection inactivity time in msec."},
	{M_IDLE_TIME_MAX, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE, "Maximum connection inactivity time in msec, before it is closed by server."},
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
				std::vector<shv::broker::AclManager::FlattenRole> roles = BrokerApp::instance()->aclManager()->userFlattenRoles(user_name, user_def.roles);
				cp::RpcValue::List ret;
				std::transform(roles.begin(), roles.end(), std::back_inserter(ret), [](const shv::broker::AclManager::FlattenRole &r) -> cp::RpcValue {
					return r.name;
				});
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
