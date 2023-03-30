#include "aclmanager.h"
#include "brokerapp.h"

#include <shv/chainpack/cponreader.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils.h>

#include <QCryptographicHash>

#include <fstream>

#define logAclManagerD() nCDebug("AclManager")
#define logAclManagerM() nCMessage("AclManager")
#define logAclManagerI() nCInfo("AclManager")

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
	if(!it->second.isValid()) {
		it->second = aclRole(role_name);
	}
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
		for(const auto &id : aclAccessRoles())
			m_cache.aclAccessRules[id];
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
		//acl.sortMostSpecificFirst();
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
	//std::string pwdsrv = std::get<0>(pwdt);
	//PasswordFormat pwdsrvfmt = std::get<1>(pwdt);
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
		//shvWarning() << "correct:" << login_context.serverNounce << "+" << acl_pwd.password;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6
		hash.addData(QByteArrayView(nonce.data(), static_cast<int>(nonce.length())));
#else
		hash.addData(nonce.data(), static_cast<int>(nonce.length()));
#endif
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		//shvWarning() << "correct:" << correct_sha1;
		//shvWarning() << "user   :" << correct_sha1;
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

std::map<std::string, AclManager::FlattenRole> AclManager::flattenRole_helper(const std::string &role_name, int nest_level)
{
	//shvInfo() << __FUNCTION__ << "role:" << role_name << "nest level:" << nest_level;
	std::map<std::string, FlattenRole> ret;
	shv::iotqt::acl::AclRole ar = aclRole(role_name);
	if(ar.isValid()) {
		FlattenRole fr{role_name, ar.weight, nest_level};
		ret[role_name] = std::move(fr);
		for(const auto &rl : ar.roles) {
			//shvInfo() << "\t child role:" << rl;
			auto it = ret.find(rl);
			if(it != ret.end()) {
				shvWarning() << "Cyclic reference in roles detected for name:" << rl;
				continue;
			}
			auto ret2 = flattenRole_helper(rl, ++nest_level);
			ret.insert(ret2.begin(), ret2.end());
		}
	}
	else {
		shvWarning() << "role:" << role_name << "is not defined";
	}
	//shvInfo() << "\t ret:";
	//for(auto kv : ret)
	//	shvInfo() << "\t\t" << kv.first << kv.second.name;
	return ret;
}

std::vector<AclManager::FlattenRole> AclManager::userFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles)
{
	if(m_cache.userFlattenRoles.find(user_name) == m_cache.userFlattenRoles.end()) {
		std::map<std::string, AclManager::FlattenRole> unique_roles;
		for(const auto &role : roles) {
			auto gg = flattenRole_helper(role, 1);
			unique_roles.insert(gg.begin(), gg.end());
		}
		std::vector<AclManager::FlattenRole> lst;
		for(const auto &kv : unique_roles)
			lst.push_back(kv.second);
		std::sort(lst.begin(), lst.end(), [](const FlattenRole &r1, const FlattenRole &r2) {
			if(r1.weight == r2.weight)
				return r1.nestLevel < r2.nestLevel;
			return r1.weight > r2.weight;
		});
		m_cache.userFlattenRoles[user_name] = lst;
	}
	return m_cache.userFlattenRoles[user_name];
}

std::vector<AclManager::FlattenRole> AclManager::flattenRole(const std::string &role)
{
	return userFlattenRoles("_Role#Key:" + role, {role});
}
/*
static cp::RpcValue merge_maps(const cp::RpcValue &m_base, const cp::RpcValue &m_over)
{
	shvDebug() << "merging:" << m_base << "and:" << m_over;
	if(m_over.isMap() && m_base.isMap()) {
		const shv::chainpack::RpcValue::Map &map_base = m_base.asMap();
		const shv::chainpack::RpcValue::Map &map_over = m_over.asMap();
		cp::RpcValue::Map map = map_base;
		for(const auto &kv : map_over) {
			map[kv.first] = merge_maps(map.value(kv.first), kv.second);
		}
		return cp::RpcValue(map);
	}
	else if(m_over.isValid())
		return m_over;
	return m_base;
}
*/
chainpack::RpcValue AclManager::userProfile(const std::string &user_name)
{
	chainpack::RpcValue ret;
	for(const auto &rn : userFlattenRoles(user_name, user(user_name).roles)) {
		shv::iotqt::acl::AclRole r = role(rn.name);
		//shvDebug() << "--------------------------merging:" << rn.name << r.toRpcValueMap();
		ret = chainpack::Utils::mergeMaps(ret, r.profile);
	}
	return ret;
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
