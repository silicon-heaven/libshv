#pragma once

#include <shv/broker/shvbrokerglobal.h>

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
	shv::chainpack::AccessGrant accessGrantForShvPath(const std::string& user_name, const shv::core::utils::ShvUrl &shv_url, const std::string &method, bool is_request_from_master_broker, bool is_service_provider_mount_point_relative_call, const shv::chainpack::RpcValue &rq_grant);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);

	std::vector<std::string> userFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles);
	std::vector<std::string> flattenRole(const std::string &role);

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

protected:
	virtual void clearCache()
	{
		m_cache = Cache();
	}
protected:
	BrokerApp * m_brokerApp;
	struct Cache
	{
		std::map<std::string, shv::iotqt::acl::AclMountDef> aclMountDefs;
		std::map<std::string, shv::iotqt::acl::AclUser> aclUsers;
		std::map<std::string, shv::iotqt::acl::AclRole> aclRoles;
		std::map<std::string, std::pair<shv::iotqt::acl::AclRoleAccessRules, bool>> aclAccessRules;

		std::map<std::string, std::vector<std::string>> userFlattenRoles;
	} m_cache;

	std::map<std::string, std::vector<std::string>> m_azureUserGroups;
public:
	std::vector<std::string> azureUserFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles);
	void setGroupForAzureUser(const std::string_view& user_name, const std::vector<std::string>& group_name);

#ifdef WITH_SHV_LDAP
	std::map<std::string, std::vector<std::string>> m_ldapUserGroups;
public:
	std::vector<std::string> ldapUserFlattenRoles(const std::string &user_name, const std::vector<std::string>& roles);
	void setGroupForLdapUser(const std::string_view& user_name, const std::vector<std::string>& group_name);
#endif
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

