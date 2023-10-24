#ifndef SHV_BROKER_ACLMANAGERSQLITE_H
#define SHV_BROKER_ACLMANAGERSQLITE_H

#include <shv/broker/aclmanager.h>

class QSqlQuery;

namespace shv {
namespace broker {

class SHVBROKER_DECL_EXPORT AclManagerSqlite : public AclManager
{
	Q_OBJECT
	using Super = AclManager;
public:
	AclManagerSqlite(BrokerApp *broker_app);
	~AclManagerSqlite() override;

	void init();

	// AclManager interface
protected:
	std::vector<std::string> aclMountDeviceIds() override;
	shv::iotqt::acl::AclMountDef aclMountDef(const std::string &device_id) override;
	void aclSetMountDef(const std::string &device_id, const shv::iotqt::acl::AclMountDef &md) override;

	std::vector<std::string> aclUsers() override;
	shv::iotqt::acl::AclUser aclUser(const std::string &user_name) override;
	void aclSetUser(const std::string &user_name, const shv::iotqt::acl::AclUser &u) override;

	std::vector<std::string> aclRoles() override;
	shv::iotqt::acl::AclRole aclRole(const std::string &role_name) override;
	void aclSetRole(const std::string &role_name, const shv::iotqt::acl::AclRole &r) override;

	std::vector<std::string> aclAccessRoles() override;
	shv::iotqt::acl::AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) override;
	void aclSetAccessRoleRules(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &rules) override;
private:
	void checkAclTables();
	void createAclSqlTables();
	void importAclConfigFiles();

	void checkTableColumn(const QString &table_name, const QString &column_name);

	QSqlQuery execSql(const QString &query_str, bool throw_exc = shv::core::Exception::Throw);
	std::vector<std::string> sqlLoadFields(const QString &table, const QString &column);
	QSqlQuery sqlLoadRow(const QString &table, const QString &key_name, const QString &key_value);
};

} // namespace broker
} // namespace shv

#endif // SHV_BROKER_ACLMANAGERSQLITE_H
