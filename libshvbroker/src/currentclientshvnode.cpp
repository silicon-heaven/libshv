#include "currentclientshvnode.h"
#include "brokerapp.h"
#include "rpc/clientconnectiononbroker.h"

#include <shv/core/utils/shvurl.h>

#include <QCryptographicHash>

namespace cp = shv::chainpack;
namespace acl = shv::iotqt::acl;

using namespace std;

static string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
	return std::string(hash.result().toHex().constData());
}

static string M_CLIENT_ID = "clientId";
static string M_MOUNT_POINT = "mountPoint";
static string M_USER_ROLES = "userRoles";
static string M_USER_PROFILE = "userProfile";
static string M_CHANGE_PASSWORD = "changePassword";
static string M_ACCES_LEVEL_FOR_METHOD_CALL = "accesLevelForMethodCall";
static string M_ACCESS_LEVEL_FOR_METHOD_CALL = "accessLevelForMethodCall";

const char *CurrentClientShvNode::NodeId = "currentClient";

CurrentClientShvNode::CurrentClientShvNode(shv::iotqt::node::ShvNode *parent)
	: Super(NodeId, &m_metaMethods, parent)
	, m_metaMethods {
		{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{M_CLIENT_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_MOUNT_POINT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_ROLES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_PROFILE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_ACCESS_LEVEL_FOR_METHOD_CALL, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read,
		  "params: [\"shv_path\", \"method\"]"},
		{M_ACCES_LEVEL_FOR_METHOD_CALL, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read,
		  "deprecated, use accessLevelForMethodCall instead"},
		{M_CHANGE_PASSWORD, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Write
		  , "params: [\"old_password\", \"new_password\"], old and new passwords can be plain or SHA1"},
	}
{
}

shv::chainpack::RpcValue CurrentClientShvNode::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	if(shv_path.asString().empty()) {
		const shv::chainpack::RpcValue::String method = rq.method().toString();
		if(method == M_CLIENT_ID) {
			int client_id = rq.peekCallerId();
			return client_id;
		}
		if(method == M_MOUNT_POINT) {
			int client_id = rq.peekCallerId();
			auto *cli = shv::broker::BrokerApp::instance()->clientById(client_id);
			if(cli) {
				return cli->mountPoint();
			}
			return nullptr;
		}
		if(method == M_USER_ROLES) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const string user_name = cli->userName();
				std::vector<shv::broker::AclManager::FlattenRole> roles = app->aclManager()->userFlattenRoles(user_name);
				cp::RpcValue::List ret;
				std::transform(roles.begin(), roles.end(), std::back_inserter(ret), [](const shv::broker::AclManager::FlattenRole &r) -> cp::RpcValue {
					return r.name;
				});
				return ret;
			}
			return nullptr;
		}
		if(method == M_USER_PROFILE) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const string user_name = cli->userName();
				cp::RpcValue ret = app->aclManager()->userProfile(user_name);
				return ret.isValid()? ret: nullptr;
			}
			return nullptr;
		}
		if(method == M_ACCESS_LEVEL_FOR_METHOD_CALL || method == M_ACCES_LEVEL_FOR_METHOD_CALL) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const shv::chainpack::RpcValue &plist = rq.params();
				const string &shv_path = plist.asList().valref(0).asString();
				if(shv_path.empty())
					SHV_EXCEPTION("Shv path not specified in params.");
				const string &method = plist.asList().valref(1).asString();
				if(method.empty())
					SHV_EXCEPTION("Method not specified in params.");
				shv::chainpack::AccessGrant acg = app->accessGrantForRequest(cli, shv::core::utils::ShvUrl(shv_path), method, rq.accessGrant());
				return acg.isValid()? acg.toRpcValue(): nullptr;
			}
			return nullptr;
		}
		if(method == M_CHANGE_PASSWORD) {
			shv::chainpack::RpcValue params = rq.params();
			const shv::chainpack::RpcValue::List &param_lst = params.toList();
			if(param_lst.size() == 2) {
				string old_password_sha1 = param_lst[0].toString();
				string new_password_sha1 = param_lst[1].toString();
				if(!old_password_sha1.empty() && !new_password_sha1.empty()) {
					int client_id = rq.peekCallerId();
					auto *app = shv::broker::BrokerApp::instance();
					auto *cli = app->clientById(client_id);
					if(cli) {
						shv::broker::AclManager *acl = app->aclManager();
						const string user_name = cli->userName();
						acl::AclUser acl_user = acl->user(user_name);
						string current_password_sha1 = acl_user.password.password;
						if(acl_user.password.format == acl::AclPassword::Format::Plain) {
							current_password_sha1 = sha1_hex(current_password_sha1);
						}
						if(old_password_sha1.size() != current_password_sha1.size()) {
							old_password_sha1 = sha1_hex(old_password_sha1);
						}
						if(old_password_sha1 != current_password_sha1) {
							SHV_EXCEPTION("Old password does not match.");
						}
						if(new_password_sha1.size() != current_password_sha1.size()) {
							new_password_sha1 = sha1_hex(new_password_sha1);
						}
						acl_user.password.password = new_password_sha1;
						acl_user.password.format = acl::AclPassword::Format::Sha1;
						acl->setUser(user_name, acl_user);
						return true;
					}
					SHV_EXCEPTION("Invalid client ID");
				}
			}
			SHV_EXCEPTION("Params must have format: [\"old_pwd\",\"new_pwd\"]");
			return nullptr;
		}
	}
	return Super::callMethodRq(rq);
}
