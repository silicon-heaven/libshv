#include "rpc/clientconnectiononbroker.h"
#include <shv/broker/currentclientshvnode.h>
#include <shv/broker/brokerapp.h>

#include <shv/core/utils/shvurl.h>

#include <QCryptographicHash>

namespace cp = shv::chainpack;
namespace acl = shv::iotqt::acl;

using namespace std;

namespace {
string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
	hash.addData(QByteArrayView(s.data(), static_cast<int>(s.length())));
#else
	hash.addData(s.data(), static_cast<int>(s.length()));
#endif
	return std::string(hash.result().toHex().constData());
}
}

namespace {
const string M_CLIENT_ID = "clientId";
const string M_MOUNT_POINT = "mountPoint";
const string M_USER_ROLES = "userRoles";
const string M_USER_PROFILE = "userProfile";
const string M_CHANGE_PASSWORD = "changePassword";
const string M_ACCES_LEVEL_FOR_METHOD_CALL = "accesLevelForMethodCall"; // deprecated typo
const string M_ACCESS_LEVEL_FOR_METHOD_CALL = "accessLevelForMethodCall";
const string M_ACCESS_GRANT_FOR_METHOD_CALL = "accessGrantForMethodCall";
}

CurrentClientShvNode::CurrentClientShvNode(shv::iotqt::node::ShvNode *parent)
	: Super(NodeId, &m_metaMethods, parent)
	, m_metaMethods {
		{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{M_CLIENT_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_MOUNT_POINT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_ROLES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_PROFILE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_ACCESS_GRANT_FOR_METHOD_CALL, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read,
		  R"(params: ["shv_path", "method"])"},
		{M_ACCESS_LEVEL_FOR_METHOD_CALL, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read,
		  "deprecated, use accessGrantForMethodCall instead"},
		{M_ACCES_LEVEL_FOR_METHOD_CALL, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read,
		  "deprecated, use accessGrantForMethodCall instead"},
		{M_CHANGE_PASSWORD, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Write
		  , R"(params: ["old_password", "new_password"], old and new passwords can be plain or SHA1)"},
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
				auto user_def = shv::broker::BrokerApp::instance()->aclManager()->user(user_name);
				auto roles = app->aclManager()->userFlattenRoles(user_name, user_def.roles);
				cp::RpcValue::List ret;
				std::copy(roles.begin(), roles.end(), std::back_inserter(ret));
#if WITH_SHV_LDAP
				auto ldap_roles = app->aclManager()->ldapUserFlattenRoles(user_name, user_def.roles);
				std::copy(ldap_roles.begin(), ldap_roles.end(), std::back_inserter(ret));
#endif
				auto azure_roles = app->aclManager()->azureUserFlattenRoles(user_name, user_def.roles);
				std::copy(azure_roles.begin(), azure_roles.end(), std::back_inserter(ret));
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
		if(method == M_ACCESS_GRANT_FOR_METHOD_CALL) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const shv::chainpack::RpcValue plist = rq.params();
				const string &shv_path_param = plist.asList().valref(0).asString();
				if(shv_path_param.empty())
					SHV_EXCEPTION("Shv path not specified in params.");
				const string &method_param = plist.asList().valref(1).asString();
				if(method_param.empty())
					SHV_EXCEPTION("Method not specified in params.");
				auto shv_url = shv::core::utils::ShvUrl(shv_path_param);
				shv::chainpack::AccessGrant acg = app->aclManager()->accessGrantForShvPath(cli->loggedUserName(), shv_url, method_param, cli->isMasterBrokerConnection(), shv_url.isUpTreeMountPointRelative(), rq.accessGrant());
				return acg.isValid()? acg.toRpcValue(): nullptr;
			}
			return nullptr;
		}
		if(method == M_ACCESS_LEVEL_FOR_METHOD_CALL || method == M_ACCES_LEVEL_FOR_METHOD_CALL) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const shv::chainpack::RpcValue plist = rq.params();
				const string &shv_path_param = plist.asList().valref(0).asString();
				if(shv_path_param.empty())
					SHV_EXCEPTION("Shv path not specified in params.");
				const string &method_param = plist.asList().valref(1).asString();
				if(method_param.empty())
					SHV_EXCEPTION("Method not specified in params.");
				auto shv_url = shv::core::utils::ShvUrl(shv_path_param);
				shv::chainpack::AccessGrant acg = app->aclManager()->accessGrantForShvPath(cli->loggedUserName(), shv_url, method_param, cli->isMasterBrokerConnection(), shv_url.isUpTreeMountPointRelative(), rq.accessGrant());
				if(acg.isValid()) {
					auto level = shv::iotqt::node::ShvNode::basicGrantToAccessLevel(acg);
					if(level > 0) {
						std::string role = shv::chainpack::MetaMethod::accessLevelToString(level);
						if(role.empty())
							return level;
						return role;
					}
				}
			}
			return nullptr;
		}
		if(method == M_CHANGE_PASSWORD) {
			shv::chainpack::RpcValue params = rq.params();
			const shv::chainpack::RpcValue::List &param_lst = params.asList();
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
