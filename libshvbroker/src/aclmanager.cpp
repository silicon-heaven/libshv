#include "aclmanager.h"
#include "brokerapp.h"
#include "currentclientshvnode.h"

#include <shv/chainpack/cponreader.h>
#include <shv/core/utils/shvurl.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils.h>

#include <QCryptographicHash>
#include <QQueue>

#include <fstream>

#define logAclManagerD() nCDebug("AclManager")
#define logAclManagerM() nCMessage("AclManager")
#define logAclManagerI() nCInfo("AclManager")

#define logAclResolveW() nCWarning("AclResolve")
#define logAclResolveM() nCMessage("AclResolve")

namespace cp = shv::chainpack;

namespace shv::broker {
//================================================================
// AclManagerBase
//================================================================
shv::broker::AclManager::AclManager(shv::broker::BrokerApp *broker_app)
	: QObject(broker_app)
	, m_brokerApp(broker_app)
{
}

std::vector<std::string> AclManager::mountDeviceIds()
{
	if(m_cache.aclMountDefs.empty()) {
		for(const auto &id : aclMountDeviceIds())
			m_cache.aclMountDefs[id];
	}
	return cp::Utils::mapKeys(m_cache.aclMountDefs);
}

shv::iotqt::acl::AclMountDef AclManager::mountDef(const std::string &device_id)
{
	if(m_cache.aclMountDefs.empty())
		mountDeviceIds();
	auto it = m_cache.aclMountDefs.find(device_id);
	if(it == m_cache.aclMountDefs.end())
		return shv::iotqt::acl::AclMountDef();
	if(!it->second.isValid()) {
		it->second = aclMountDef(device_id);
	}
	return it->second;
}

void AclManager::setMountDef(const std::string &device_id, const shv::iotqt::acl::AclMountDef &v)
{
	aclSetMountDef(device_id, v);
	m_cache.aclMountDefs.clear();
}

std::vector<std::string> AclManager::users()
{
	if(m_cache.aclUsers.empty()) {
		for(const auto &id : aclUsers())
			m_cache.aclUsers[id];
	}
	return cp::Utils::mapKeys(m_cache.aclUsers);
}

shv::iotqt::acl::AclUser AclManager::user(const std::string &user_name)
{
	if(m_cache.aclUsers.empty())
		users();
	auto it = m_cache.aclUsers.find(user_name);
	if(it == m_cache.aclUsers.end())
		return shv::iotqt::acl::AclUser();
	if(!it->second.isValid()) {
		it->second = aclUser(user_name);
	}
	return it->second;
}

void AclManager::setUser(const std::string &user_name, const shv::iotqt::acl::AclUser &u)
{
	aclSetUser(user_name, u);
	m_cache.aclUsers.clear();
	m_cache.userFlattenRoles.clear();
}

std::vector<std::string> AclManager::roles()
{
	if(m_cache.aclRoles.empty()) {
		for(const auto &id : aclRoles())
			m_cache.aclRoles[id];
	}
	return cp::Utils::mapKeys(m_cache.aclRoles);
}

shv::iotqt::acl::AclRole AclManager::role(const std::string &role_name)
{
	if(m_cache.aclRoles.empty())
		roles();
	auto it = m_cache.aclRoles.find(role_name);
	if(it == m_cache.aclRoles.end())
		return shv::iotqt::acl::AclRole();

	it->second = aclRole(role_name);
	return it->second;
}

void AclManager::setRole(const std::string &role_name, const shv::iotqt::acl::AclRole &v)
{
	aclSetRole(role_name, v);
	m_cache.aclRoles.clear();
	m_cache.userFlattenRoles.clear();
}

std::vector<std::string> AclManager::accessRoles()
{
	if(m_cache.aclAccessRules.empty()) {
		for(const auto &id : aclAccessRoles()) {
			m_cache.aclAccessRules.emplace(id, std::pair<shv::iotqt::acl::AclRoleAccessRules, bool>({}, false));
		}
	}
	return cp::Utils::mapKeys(m_cache.aclAccessRules);
}

shv::iotqt::acl::AclRoleAccessRules AclManager::accessRoleRules(const std::string &role_name)
{
	if(m_cache.aclAccessRules.empty())
		accessRoles();
	auto it = m_cache.aclAccessRules.find(role_name);
	if(it == m_cache.aclAccessRules.end())
		return shv::iotqt::acl::AclRoleAccessRules();
	if(std::get<1>(it->second) == false) {
		shv::iotqt::acl::AclRoleAccessRules acl = aclAccessRoleRules(role_name);
		it->second = std::pair<shv::iotqt::acl::AclRoleAccessRules, bool>(acl, true);
	}
	return std::get<0>(it->second);
}

void AclManager::setAccessRoleRules(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &v)
{
	aclSetAccessRoleRules(role_name, v);
	m_cache.aclAccessRules.clear();
}

chainpack::UserLoginResult AclManager::checkPassword(const chainpack::UserLoginContext &login_context)
{
	using LoginType = cp::UserLogin::LoginType;
	chainpack::UserLogin login = login_context.userLogin();
	shv::iotqt::acl::AclUser acl_user = user(login.user);
	if(!acl_user.isValid()) {
		shvError() << "Invalid user name:" << login.user;
		return cp::UserLoginResult(false, "Invalid user name.");
	}
	auto acl_pwd = acl_user.password;
	if(acl_pwd.password.empty()) {
		shvError() << "Empty password not allowed:" << login.user;
		return cp::UserLoginResult(false, "Empty password not allowed.");
	}
	auto usr_login_type = login.loginType;
	if(usr_login_type == LoginType::Invalid)
		usr_login_type = LoginType::Sha1;

	std::string usr_pwd = login.password;
	if(usr_login_type == LoginType::Plain) {
		logAclManagerM() << "user_login_type: PLAIN";
		if(acl_pwd.format == shv::iotqt::acl::AclPassword::Format::Plain) {
			if(acl_pwd.password == usr_pwd)
				return cp::UserLoginResult(true);
			logAclManagerM() << "\t Invalid password.";
			return cp::UserLoginResult(false, "Invalid password.");
		}
		if(acl_pwd.format == shv::iotqt::acl::AclPassword::Format::Sha1) {
			if(acl_pwd.password == shv::iotqt::utils::sha1Hex(usr_pwd))
				return cp::UserLoginResult(true);
			logAclManagerM() << "\t Invalid password.";
			return cp::UserLoginResult(false, "Invalid password.");
		}
	}
	if(usr_login_type == LoginType::Sha1) {
		/// login_type == "SHA1" is default
		logAclManagerM() << "user_login_type: SHA1";
		if(acl_pwd.format == shv::iotqt::acl::AclPassword::Format::Plain)
			acl_pwd.password = shv::iotqt::utils::sha1Hex(acl_pwd.password);

		std::string nonce = login_context.serverNounce + acl_pwd.password;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
		hash.addData(QByteArrayView(nonce.data(), static_cast<int>(nonce.length())));
#else
		hash.addData(nonce.data(), static_cast<int>(nonce.length()));
#endif
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		if(usr_pwd == correct_sha1)
			return cp::UserLoginResult(true);
		logAclManagerM() << "\t Invalid password.";
		return cp::UserLoginResult(false, "Invalid password.");
	}
	shvError() << "Unsupported login type" << cp::UserLogin::loginTypeToString(login.loginType) << "for user:" << login.user;
	return cp::UserLoginResult(false, "Invalid login type.");
}

std::string AclManager::mountPointForDevice(const chainpack::RpcValue &device_id)
{
	return mountDef(device_id.asString()).mountPoint;
}

void AclManager::reload()
{
	clearCache();
}

void AclManager::aclSetMountDef(const std::string &device_id, const shv::iotqt::acl::AclMountDef &md)
{
	Q_UNUSED(device_id)
	Q_UNUSED(md)
	SHV_EXCEPTION("Mount points definition is read only.");
}

void AclManager::aclSetUser(const std::string &user_name, const shv::iotqt::acl::AclUser &u)
{
	Q_UNUSED(user_name)
	Q_UNUSED(u)
	SHV_EXCEPTION("Users definition is read only.");
}

void AclManager::aclSetRole(const std::string &role_name, const shv::iotqt::acl::AclRole &r)
{
	Q_UNUSED(role_name)
	Q_UNUSED(r)
	SHV_EXCEPTION("Roles definition is read only.");
}

void AclManager::aclSetAccessRoleRules(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &rp)
{
	Q_UNUSED(role_name)
	Q_UNUSED(rp)
	SHV_EXCEPTION("Role paths definition is read only.");
}

std::vector<std::string> AclManager::userFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles)
{
	if(m_cache.userFlattenRoles.find(user_name) == m_cache.userFlattenRoles.end()) {
		auto& flattenRoles = m_cache.userFlattenRoles.emplace(user_name, std::vector<std::string>{}).first->second;
		QQueue<std::string> role_q;
		auto enqueue = [&role_q, &flattenRoles] (const auto& role) {
			if (std::ranges::find(flattenRoles, role) != flattenRoles.end() || role_q.contains(role)) {
				shvDebug() << "Duplicate role detected:" << role;
				return;
			}
			role_q.enqueue(role);
		};
		std::ranges::for_each(roles, enqueue);
		while (!role_q.empty()) {
			auto cur = role_q.dequeue();
			flattenRoles.emplace_back(cur);
			auto subroles = aclRole(cur).roles;
			for (const auto& subrole : subroles) {
				enqueue(subrole);
			}
		}
	}
	return m_cache.userFlattenRoles[user_name];
}

std::vector<std::string> AclManager::flattenRole(const std::string &role)
{
	return userFlattenRoles("_Role#Key:" + role, {role});
}

chainpack::RpcValue AclManager::userProfile(const std::string &user_name)
{
	chainpack::RpcValue ret;
	for(const auto &rn : userFlattenRoles(user_name, user(user_name).roles)) {
		shv::iotqt::acl::AclRole r = role(rn);
		ret = chainpack::Utils::mergeMaps(ret, r.profile);
	}
	return ret;
}

void AclManager::setGroupForAzureUser(const std::string_view& user_name, const std::vector<std::string>& group_name)
{
	m_azureUserGroups.emplace(user_name, group_name);
}

#ifdef WITH_SHV_LDAP
void AclManager::setGroupForLdapUser(const std::string_view& user_name, const std::vector<std::string>& group_name)
{
	m_ldapUserGroups.emplace(user_name, group_name);
}
#endif

cp::AccessGrant AclManager::accessGrantForShvPath(const std::string& user_name, const shv::core::utils::ShvUrl &shv_url, const std::string &method, bool is_request_from_master_broker, bool is_service_provider_mount_point_relative_call, const shv::chainpack::RpcValue &rq_grant)
{
	logAclResolveM() << "==== accessGrantForShvPath user:" << user_name << "requested path:" << shv_url.toString() << "method:" << method << "request grant:" << rq_grant.toCpon();
	if(is_service_provider_mount_point_relative_call) {
		auto ret = cp::AccessGrant(cp::Rpc::ROLE_WRITE);
		logAclResolveM() << "==== resolved path:" << shv_url.toString() << "grant:" << ret.toRpcValue().toCpon();
		return ret;
	}
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	PathGrantCache *user_path_grants = m_userPathGrantCache.object(user_name);
	if(user_path_grants) {
		cp::Rpc::AccessGrant *pg = user_path_grants->object(shv_path);
		if(pg) {
			logAclD() << "\t cache hit:" << pg->grant << "weight:" << pg->weight;
			return *pg;
		}
	}
#endif
	auto request_grant = cp::AccessGrant::fromRpcValue(rq_grant);
	if(is_request_from_master_broker) {
		if(request_grant.isValid()) {
			// access resolved by master broker already, forward use this
			logAclResolveM() << "\t Resolved on master broker already.";
			return request_grant;
		}
	}
	else {
		if(request_grant.isValid()) {
			logAclResolveM() << "Client defined grants in RPC request are not implemented yet and will be ignored.";
		}
	}
	std::vector<std::string> flatten_user_roles;
	if(is_request_from_master_broker) {
		// set masterBroker role to requests from master broker without access grant specified
		// This is used mainly for service calls as (un)subscribe propagation to slave brokers etc.
		if(shv_url.pathPart() == cp::Rpc::DIR_BROKER_APP) {
			// master broker has always rd grant to .broker/app path
			return cp::AccessGrant(cp::Rpc::ROLE_WRITE);
		}
		flatten_user_roles = flattenRole(cp::Rpc::ROLE_MASTER_BROKER);
	}
	else {
		if (auto user_def = user(user_name); user_def.isValid()) {
			flatten_user_roles = userFlattenRoles(user_name, user_def.roles);
		}
#ifdef WITH_SHV_LDAP
		// I don't have to check if ldap is enabled - if m_ldapUserGroups is non-empty, it must've been enabled.
		else if (auto ldap_it = m_ldapUserGroups.find(user_name); ldap_it != m_ldapUserGroups.end()) {
			flatten_user_roles = userFlattenRoles(user_name, ldap_it->second);
		}
#endif
		else if (auto azure_it = m_azureUserGroups.find(user_name); azure_it != m_azureUserGroups.end()) {
			flatten_user_roles = userFlattenRoles(user_name, azure_it->second);
		}
	}
	logAclResolveM() << "searched rules:" << [this, &flatten_user_roles]()
	{
		auto to_str = [](const QVariant &v, int len) {
			bool right = false;
			if(len < 0) {
				right = true;
				len = -len;
			}
			QString s = v.toString();
			if(s.length() < len) {
				QString spaces = QString(len - s.length(), ' ');
				if(right)
					s = spaces + s;
				else
					s = s + spaces;
			}
			return s;
		};
		std::vector<int> cols = {15, 10, 20, 15, 1};
		const int row_len = 70;
		QString tbl = "\n" + QString(row_len, '-') + '\n';
		tbl += to_str("role", cols[0]);
		tbl += to_str("service", cols[1]);
		tbl += to_str("pattern", cols[2]);
		tbl += to_str("method", cols[3]);
		tbl += to_str("grant", cols[4]);
		tbl += "\n" + QString(row_len, '-');
		for(const std::string &role : flatten_user_roles) {
			const iotqt::acl::AclRoleAccessRules &role_rules = accessRoleRules(role);
			for(const iotqt::acl::AclAccessRule &access_rule : role_rules) {
				tbl += "\n";
				tbl += to_str(role.c_str(), cols[0]);
				tbl += to_str(access_rule.service.c_str(), cols[1]);
				tbl += to_str(access_rule.pathPattern.c_str(), cols[2]);
				tbl += to_str(access_rule.method.c_str(), cols[3]);
				tbl += to_str(access_rule.grant.toRpcValue().toCpon().c_str(), cols[4]);
			}
		}
		tbl += "\n" + QString(row_len, '-');
		return tbl;
	}();

	// find first matching rule
	if(shv_url.pathPart() == BROKER_CURRENT_CLIENT_SHV_PATH) {
		// client has WR grant on currentClient node
		return cp::AccessGrant{cp::Rpc::ROLE_WRITE};
	}

	for (const std::string& flatten_role : flatten_user_roles) {
		logAclResolveM() << "----- checking role:" << flatten_role;
		auto role_rules = accessRoleRules(flatten_role);
		if (role_rules.empty()) {
			logAclResolveM() << "\t no paths defined.";
		}

		for (const auto& access_rule : role_rules) {
			logAclResolveM() << "rule:" << access_rule.toRpcValue().toCpon();
			if (access_rule.isPathMethodMatch(shv_url, method)) {
				logAclResolveM() << "access user:" << user_name
					<< "shv_path:" << shv_url.toString()
					<< "rq_grant:" << (rq_grant.isValid()? rq_grant.toCpon(): "<none>")
					<< "==== path:" << access_rule.pathPattern << "method:" << access_rule.method << "grant:" << access_rule.grant.toRpcValue().toCpon();
				return access_rule.grant;
			}
		}
	}

	return cp::AccessGrant{};
}

//================================================================
// AclManagerConfigFiles
//================================================================
const auto FILE_ACL_MOUNTS = std::string("mounts.cpon");
const auto FILE_ACL_MOUNTS_DEPR = std::string("fstab.cpon");

const auto FILE_ACL_USERS = std::string("users.cpon");

const auto FILE_ACL_ROLES = std::string("roles.cpon");
const auto FILE_ACL_ROLES_DEPR = std::string("grants.cpon");

const auto FILE_ACL_ACCESS = std::string("access.cpon");
const auto FILE_ACL_ACCESS_DEPR = std::string("paths.cpon");

AclManagerConfigFiles::AclManagerConfigFiles(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating ConfigFiles ACL manager";
}

std::string AclManagerConfigFiles::configDir() const
{
	return m_brokerApp->cliOptions()->effectiveConfigDir();
}

void AclManagerConfigFiles::clearCache()
{
	Super::clearCache();
	m_configFiles.clear();
}

chainpack::RpcValue AclManagerConfigFiles::aclConfig(const std::string &config_name, bool throw_exc)
{
	shv::chainpack::RpcValue config = m_configFiles.value(config_name);
	if(!config.isValid()) {
		config = loadAclConfig(config_name, throw_exc);
		m_configFiles[config_name] = config;
		if(!config.isValid()) {
			if(throw_exc)
				throw std::runtime_error("Config " + config_name + " does not exist.");

			return chainpack::RpcValue();
		}
	}
	return config;
}

shv::chainpack::RpcValue AclManagerConfigFiles::loadAclConfig(const std::string &config_name, bool throw_exc)
{
	logAclManagerD() << "loadAclConfig:" << config_name;
	std::vector<std::string> files;
	if(config_name == FILE_ACL_MOUNTS)
		files = std::vector<std::string>{FILE_ACL_MOUNTS, FILE_ACL_MOUNTS_DEPR};
	else if(config_name == FILE_ACL_USERS)
		files = std::vector<std::string>{FILE_ACL_USERS};
	else if(config_name == FILE_ACL_ROLES)
		files = std::vector<std::string>{FILE_ACL_ROLES, FILE_ACL_ROLES_DEPR};
	else if(config_name == FILE_ACL_ACCESS)
		files = std::vector<std::string>{FILE_ACL_ACCESS, FILE_ACL_ACCESS_DEPR};
	std::ifstream fis;
	for(auto fn : files) {
		if(fn[0] != '/')
			fn = configDir() + '/' + fn;
		fis.open(fn);
		if (fis.good())
			break;
	}
	if (!fis.good()) {
		if(throw_exc)
			throw std::runtime_error("Cannot open config file for reading, attempts: "
									 + std::accumulate(files.begin(), files.end(), std::string(), [this](std::string a, std::string b) {
											return std::move(a) + (a.empty()? std::string(): std::string(", ")) + configDir() + '/' + std::move(b);
									}));

		return cp::RpcValue();
	}
	shv::chainpack::CponReader rd(fis);
	shv::chainpack::RpcValue rv;
	std::string err;
	rv = rd.read(throw_exc? nullptr: &err);
	return rv;
}

std::vector<std::string> AclManagerConfigFiles::aclMountDeviceIds()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_MOUNTS);
	return cp::Utils::mapKeys(cfg.asMap());
}

shv::iotqt::acl::AclMountDef AclManagerConfigFiles::aclMountDef(const std::string &device_id)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_MOUNTS).asMap().value(device_id);
	return shv::iotqt::acl::AclMountDef::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclUsers()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_USERS);
	shvDebug() << cfg.toCpon("\t");
	return cp::Utils::mapKeys(cfg.asMap());
}

shv::iotqt::acl::AclUser AclManagerConfigFiles::aclUser(const std::string &user_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_USERS).asMap().value(user_name);
	return shv::iotqt::acl::AclUser::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclRoles()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_ROLES);
	return cp::Utils::mapKeys(cfg.asMap());
}

shv::iotqt::acl::AclRole AclManagerConfigFiles::aclRole(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_ROLES).asMap().value(role_name);
	return shv::iotqt::acl::AclRole::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclAccessRoles()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_ACCESS);
	return cp::Utils::mapKeys(cfg.asMap());
}

shv::iotqt::acl::AclRoleAccessRules AclManagerConfigFiles::aclAccessRoleRules(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_ACCESS).asMap().value(role_name);
	return shv::iotqt::acl::AclRoleAccessRules::fromRpcValue(v);
}

} // namespace shv
