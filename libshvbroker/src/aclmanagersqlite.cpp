#include "aclmanagersqlite.h"
#include <shv/broker/brokerapp.h>

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlRecord>

#define logAclManagerD() nCDebug("AclManager")
#define logAclManagerM() nCMessage("AclManager")
#define logAclManagerI() nCInfo("AclManager")

namespace cp = shv::chainpack;
namespace acl = shv::iotqt::acl;

namespace shv::broker {

const auto TBL_ACL_MOUNTS = QStringLiteral("acl_mounts");
const auto TBL_ACL_USERS = QStringLiteral("acl_users");
const auto TBL_ACL_ROLES = QStringLiteral("acl_roles");
const auto TBL_ACL_ACCESS = QStringLiteral("acl_access");

AclManagerSqlite::AclManagerSqlite(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating Sqlite ACL Manager";
}

AclManagerSqlite::~AclManagerSqlite() = default;

void AclManagerSqlite::init()
{
	checkAclTables();
}

QSqlQuery AclManagerSqlite::execSql(const QString &query_str, bool throw_exc)
{
	QSqlDatabase db = m_brokerApp->sqlConfigConnection();
	QSqlQuery q(db);
	if(!q.exec(query_str) && throw_exc)
		SHV_EXCEPTION("SQL ERROR: " + q.lastError().text().toStdString() + "\nQuery: " + query_str.toStdString());
	return q;
}

std::vector<std::string> AclManagerSqlite::sqlLoadFields(const QString &table, const QString &column)
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT " + column + " FROM " + table + " ORDER BY " + column);
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

QSqlQuery AclManagerSqlite::sqlLoadRow(const QString &table, const QString &key_name, const QString &key_value)
{
	return execSql("SELECT * FROM " + table + " WHERE " + key_name + "='" + key_value + "'");
}

void AclManagerSqlite::checkAclTables()
{
	{
		QSqlQuery q = execSql("SELECT COUNT(*) FROM " + TBL_ACL_USERS, !shv::core::Exception::Throw);
		bool db_exist = q.isActive();
		if(!db_exist) {
			shvInfo() << "Table" << TBL_ACL_USERS << "does not exist.";
			execSql("BEGIN TRANSACTION");
			createAclSqlTables();
			importAclConfigFiles();
			execSql("COMMIT");
		}
	}
	checkTableColumn(TBL_ACL_ROLES, "profile");
	checkTableColumn(TBL_ACL_ACCESS, "method");
	checkTableColumn(TBL_ACL_ACCESS, "service");
}

void AclManagerSqlite::checkTableColumn(const QString &table_name, const QString &column_name)
{
	QSqlQuery q = execSql("SELECT " + column_name + " FROM " + table_name + " LIMIT 1", !shv::core::Exception::Throw);
	bool column_exist = q.isActive();
	if(!column_exist) {
		shvWarning() << "Table:" << table_name << "column:" << column_name << "does not exist.";
		shvInfo().nospace() << "Adding column: " << table_name << "." << column_name;
		execSql("BEGIN TRANSACTION");
		execSql(QStringLiteral(R"kkt(
				ALTER TABLE %1 ADD %2 varchar;
				)kkt").arg(table_name, column_name));
		execSql("COMMIT");
	}
}

void AclManagerSqlite::createAclSqlTables()
{
	shvInfo() << "Creating SQLite ACL tables";
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				deviceId character varying PRIMARY KEY,
				mountPoint character varying,
				description character varying
			);
			)kkt").arg(TBL_ACL_MOUNTS));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				name character varying PRIMARY KEY,
				password character varying,
				passwordFormat character varying,
				roles character varying
			);
			)kkt").arg(TBL_ACL_USERS));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				name character varying PRIMARY KEY,
				weight integer,
				roles character varying,
				profile character varying
			);
			)kkt").arg(TBL_ACL_ROLES));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				role character varying,
				service character varying,
				path character varying,
				method character varying,
				grantType character varying,
				accessLevel integer,
				accessRole character varying,
				user character varying,
				password character varying,
				loginType character varying,
				ruleNumber integer varying,
				PRIMARY KEY (role, path, method)
			);
			)kkt").arg(TBL_ACL_ACCESS));
}

namespace {
QString join_str_vec(const std::vector<std::string> &lst)
{
	QStringList qlst;
	for(const std::string &s : lst)
		qlst << QString::fromStdString(s);
	return qlst.join(',');
}

std::vector<std::string> split_str_vec(const QString &ss)
{
	std::vector<std::string> ret;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	for(const QString &s : ss.split(',', QString::SkipEmptyParts))
		ret.push_back(s.trimmed().toStdString());
#else
	for(const QString &s : ss.split(',', Qt::SkipEmptyParts))
		ret.push_back(s.trimmed().toStdString());
#endif
	return ret;
}
}

void AclManagerSqlite::importAclConfigFiles()
{
	AclManagerConfigFiles facl(m_brokerApp);
	shvInfo() << "Importing ACL config files from:" << facl.configDir();
	for(const std::string &id : facl.mountDeviceIds()) {
		acl::AclMountDef md = facl.mountDef(id);
		if(!md.isValid())
			shvWarning() << "Cannot import invalid mount definition for device id:" << id;
		else
			aclSetMountDef(id, md);
	}
	for(const std::string &user : facl.users()) {
		acl::AclUser u = facl.user(user);
		aclSetUser(user, u);
	}
	for(const std::string &role : facl.roles()) {
		acl::AclRole r = facl.role(role);
		aclSetRole(role, r);
	}
	for(const std::string &role : facl.accessRoles()) {
		acl::AclRoleAccessRules rpt = facl.accessRoleRules(role);
		aclSetAccessRoleRules(role, rpt);
	}
}

std::vector<std::string> AclManagerSqlite::aclMountDeviceIds()
{
	return sqlLoadFields(TBL_ACL_MOUNTS, "deviceId");
}

acl::AclMountDef AclManagerSqlite::aclMountDef(const std::string &device_id)
{
	acl::AclMountDef ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_MOUNTS, "deviceId", QString::fromStdString(device_id));
	if(q.next()) {
		ret.mountPoint = q.value("mountPoint").toString().toStdString();
		ret.description = q.value("description").toString().toStdString();
	}
	return ret;
}

void AclManagerSqlite::aclSetMountDef(const std::string &device_id, const acl::AclMountDef &md)
{
	if(md.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_MOUNTS + " (deviceId, mountPoint, description) VALUES('%1', '%2', '%3')";
		qs = qs.arg(QString::fromStdString(device_id));
		qs = qs.arg(QString::fromStdString(md.mountPoint));
		qs = qs.arg(QString::fromStdString(md.description));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		QString qs = "DELETE FROM " + TBL_ACL_MOUNTS + " WHERE deviceId='" + QString::fromStdString(device_id) + "'";
		logAclManagerM() << qs;
		execSql(qs);
	}
}

std::vector<std::string> AclManagerSqlite::aclUsers()
{
	return sqlLoadFields(TBL_ACL_USERS, "name");
}

acl::AclUser AclManagerSqlite::aclUser(const std::string &user_name)
{
	acl::AclUser ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_USERS, "name", QString::fromStdString(user_name));
	if(q.next()) {
		ret.password.password = q.value("password").toString().toStdString();
		ret.password.format = acl::AclPassword::formatFromString(q.value("passwordFormat").toString().toStdString());
		ret.roles = split_str_vec(q.value("roles").toString());
	}
	return ret;
}

void AclManagerSqlite::aclSetUser(const std::string &user_name, const acl::AclUser &u)
{
	if(u.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_USERS + " (name, password, passwordFormat, roles) VALUES('%1', '%2', '%3', '%4')";
		qs = qs.arg(QString::fromStdString(user_name));
		qs = qs.arg(QString::fromStdString(u.password.password));
		qs = qs.arg(QString::fromStdString(acl::AclPassword::formatToString(u.password.format)));
		qs = qs.arg(join_str_vec(u.roles));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		QString qs = "DELETE FROM " + TBL_ACL_USERS + " WHERE name='" + QString::fromStdString(user_name) + "'";
		logAclManagerM() << qs;
		execSql(qs);
	}
}

std::vector<std::string> AclManagerSqlite::aclRoles()
{
	return sqlLoadFields(TBL_ACL_ROLES, "name");
}

acl::AclRole AclManagerSqlite::aclRole(const std::string &role_name)
{
	acl::AclRole ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_ROLES, "name", QString::fromStdString(role_name));
	if(q.next()) {
		ret.roles = split_str_vec(q.value("roles").toString());
		std::string profile_str = q.value("profile").toString().toStdString();
		if(!profile_str.empty()) {
			std::string err;
			ret.profile = shv::chainpack::RpcValue::fromCpon(profile_str, &err);
			if(!err.empty())
				shvError() << role_name << "invalid profile definition:" << profile_str;
		}
		if(!ret.profile.isMap())
			ret.profile = cp::RpcValue();
	}
	return ret;
}

void AclManagerSqlite::aclSetRole(const std::string &role_name, const acl::AclRole &r)
{
	QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_ROLES + " (name, roles, profile) VALUES('%1', '%2', '%3')";
	qs = qs.arg(QString::fromStdString(role_name));
	qs = qs.arg(join_str_vec(r.roles));
	qs = qs.arg(QString::fromStdString(r.profile.isValid()? r.profile.toCpon(): ""));
	logAclManagerM() << qs;
	execSql(qs);
}

std::vector<std::string> AclManagerSqlite::aclAccessRoles()
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT role FROM " + TBL_ACL_ACCESS + " GROUP BY role ORDER BY role");
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

acl::AclRoleAccessRules AclManagerSqlite::aclAccessRoleRules(const std::string &role_name)
{
	acl::AclRoleAccessRules ret;
	QSqlQuery q = execSql("SELECT * FROM " + TBL_ACL_ACCESS + " WHERE role='" + QString::fromStdString(role_name) + "' ORDER BY ruleNumber");
	while(q.next()) {
		acl::AclAccessRule ag;
		ag.service = q.value("service").toString().toStdString();
		ag.pathPattern = q.value("path").toString().toStdString();
		ag.method = q.value("method").toString().toStdString();
		std::string grant_type = q.value("grantType").toString().toStdString();
		if (grant_type == "AccessLevel") {
			ag.grant = static_cast<cp::MetaMethod::AccessLevel>(q.value("accessLevel").toInt());
		} else if (grant_type == "Role") {
			ag.grant = q.value("accessRole").toString().toStdString();
		} else {
			SHV_EXCEPTION("Invalid PathAccessGrant type: " + grant_type);
		}
		ret.push_back(std::move(ag));
	}
	return ret;
}

void AclManagerSqlite::aclSetAccessRoleRules(const std::string &role_name, const acl::AclRoleAccessRules &rules)
{
	QString qs = "DELETE FROM " + TBL_ACL_ACCESS + " WHERE role='" + QString::fromStdString(role_name) + "'";
	logAclManagerM() << qs;
	execSql(qs);
	if(!rules.empty()) {
		QSqlDatabase db = m_brokerApp->sqlConfigConnection();
		QSqlDriver *drv = db.driver();
		QSqlRecord rec = drv->record(TBL_ACL_ACCESS);
		int rule_number = 0;
		for(const auto &rule : rules) {
			rec.setValue("role", QString::fromStdString(role_name));
			rec.setValue("service", QString::fromStdString(rule.service));
			rec.setValue("path", QString::fromStdString(rule.pathPattern));
			rec.setValue("method", rule.method.empty()? QVariant(): QString::fromStdString(rule.method));
			if (std::holds_alternative<std::string>(rule.grant)) {
				rec.setValue("grantType", "Role");
				rec.setValue("accessLevel", QString::fromStdString(std::get<std::string>(rule.grant)));
			} else if (std::holds_alternative<chainpack::MetaMethod::AccessLevel>(rule.grant)) {
				rec.setValue("grantType", "AccessLevel");
				rec.setValue("accessLevel", static_cast<int>(std::get<chainpack::MetaMethod::AccessLevel>(rule.grant)));
			} else {
				SHV_EXCEPTION("Invalid PathAccessGrant type: " + std::to_string(rule.grant.index()));
			}
			rec.setValue("ruleNumber", rule_number);
			qs = drv->sqlStatement(QSqlDriver::InsertStatement, TBL_ACL_ACCESS, rec, false);
			logAclManagerM() << qs;
			execSql(qs);
			rule_number++;
		}
	}
}

} // namespace shv
