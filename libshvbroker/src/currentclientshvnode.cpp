#include "currentclientshvnode.h"
#include "brokerapp.h"
#include "rpc/clientconnectiononbroker.h"

#include <QCryptographicHash>

namespace cp = shv::chainpack;

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
		{M_CHANGE_PASSWORD, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Write
		  , "params: [\"old_password\", \"new_password\"], old and new passwords can be plain or SHA1"},
	}
{
}

shv::chainpack::RpcValue CurrentClientShvNode::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	if(shv_path.toString().empty()) {
		const shv::chainpack::RpcValue::String &method = rq.method().toString();
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
						shv::broker::AclUser acl_user = acl->user(user_name);
						string current_password_sha1 = acl_user.password.password;
						if(acl_user.password.format == shv::broker::AclPassword::Format::Plain) {
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
						acl_user.password.format = shv::broker::AclPassword::Format::Sha1;
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