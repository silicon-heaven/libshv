#pragma once

#include "shvbrokerglobal.h"

#include <shv/iotqt/acl/aclrole.h>
#include <shv/iotqt/acl/aclroleaccessrules.h>
#include <shv/iotqt/acl/aclmountdef.h>
#include <shv/iotqt/acl/acluser.h>
#include <shv/iotqt/acl/aclpassword.h>
#include <shv/core/exception.h>

#include <QObject>

#include <string>
#include <set>

namespace shv {
namespace broker {

class BrokerApp;

class SHVBROKER_DECL_EXPORT AclManager : public QObject
{
	Q_OBJECT
public:
	AclManager(BrokerApp *broker_app);

	std::vector<std::string> mountDeviceIds();
	shv::iotqt::acl::AclMountDef mountDef(const std::string &device_id);
	void setMountDef(const std::string &device_id, const shv::iotqt::acl::AclMountDef &v);

	std::vector<std::string> users();
	shv::iotqt::acl::AclUser user(const std::string &user_name);
	void setUser(const std::string &user_name, const shv::iotqt::acl::AclUser &u);

	std::vector<std::string> roles();
	shv::iotqt::acl::AclRole role(const std::string &role_name);
	void setRole(const std::string &role_name, const shv::iotqt::acl::AclRole &v);

	std::vector<std::string> accessRoles();
	shv::iotqt::acl::AclRoleAccessRules accessRoleRules(const std::string &role_name);
	void setAccessRoleRules(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &v);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);

	struct SHVBROKER_DECL_EXPORT FlattenRole
	{
		std::string name;
		int weight = 0;
		int nestLevel = 0;

		FlattenRole() = default;
		FlattenRole(const std::string &n, int w = 0, int nl = 0) : name(n), weight(w), nestLevel(nl) {}
	};
	// all roles sorted by weight DESC, nest_level ASC
	std::vector<FlattenRole> userFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles);
	std::vector<FlattenRole> flattenRole(const std::string &role);

	chainpack::RpcValue userProfile(const std::string &user_name);

	virtual chainpack::UserLoginResult checkPassword(const chainpack::UserLoginContext &login_context);

	virtual void reload();
protected:
	virtual std::vector<std::string> aclMountDeviceIds() = 0;
	virtual shv::iotqt::acl::AclMountDef aclMountDef(const std::string &device_id) = 0;
	virtual void aclSetMountDef(const std::string &device_id, const shv::iotqt::acl::AclMountDef &md);

	virtual std::vector<std::string> aclUsers() = 0;
	virtual shv::iotqt::acl::AclUser aclUser(const std::string &user_name) = 0;
	virtual void aclSetUser(const std::string &user_name, const shv::iotqt::acl::AclUser &u);

	virtual std::vector<std::string> aclRoles() = 0;
	virtual shv::iotqt::acl::AclRole aclRole(const std::string &role_name) = 0;
	virtual void aclSetRole(const std::string &role_name, const shv::iotqt::acl::AclRole &r);

	virtual std::vector<std::string> aclAccessRoles() = 0;
	virtual shv::iotqt::acl::AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) = 0;
	virtual void aclSetAccessRoleRules(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &rp);

	//virtual AclPassword aclUserPassword(const std::string &user) = 0;
protected:
	virtual void clearCache()
	{
		m_cache = Cache();
	}
	std::map<std::string, FlattenRole> flattenRole_helper(const std::string &role_name, int nest_level);
protected:
	BrokerApp * m_brokerApp;
	struct Cache
	{
		std::map<std::string, shv::iotqt::acl::AclMountDef> aclMountDefs;
		std::map<std::string, shv::iotqt::acl::AclUser> aclUsers;
		std::map<std::string, shv::iotqt::acl::AclRole> aclRoles;
		std::map<std::string, std::pair<shv::iotqt::acl::AclRoleAccessRules, bool>> aclAccessRules;

		std::map<std::string, std::vector<FlattenRole>> userFlattenRoles;
	} m_cache;
};

class SHVBROKER_DECL_EXPORT AclManagerConfigFiles : public AclManager
{
	using Super = AclManager;
public:
	AclManagerConfigFiles(BrokerApp *broker_app);

	std::string configDir() const;
protected:
	void clearCache() override;
	shv::chainpack::RpcValue aclConfig(const std::string &config_name, bool throw_exc = shv::core::Exception::Throw);
	shv::chainpack::RpcValue loadAclConfig(const std::string &config_name, bool throw_exc);

	std::vector<std::string> aclMountDeviceIds() override;
	shv::iotqt::acl::AclMountDef aclMountDef(const std::string &device_id) override;
	std::vector<std::string> aclUsers() override;
	shv::iotqt::acl::AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	shv::iotqt::acl::AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclAccessRoles() override;
	shv::iotqt::acl::AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) override;
protected:
	shv::chainpack::RpcValue::Map m_configFiles;
};

} // namespace broker
} // namespace shv

