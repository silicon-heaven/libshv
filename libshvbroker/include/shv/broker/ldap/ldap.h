#pragma once
#include <ldap.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace shv::ldap {
class LdapError : public std::runtime_error {
public:
	LdapError(const std::string& err_string, int code);
	int code() const;
private:
	int m_code;
};

enum class Version : int {
	Version1 = LDAP_VERSION1,
	Version2 = LDAP_VERSION2,
	Version3 = LDAP_VERSION3,
};

struct Entry {
	std::map<std::string, std::vector<std::string>> keysAndValues;
};

class Ldap {
public:
	static std::unique_ptr<Ldap> create(const std::string& hostname);
	void setVersion(Version version);
	void connect();
	void bindSasl(const std::string& bind_dn, const std::string& bind_pw);
	void bindWithLastCreds();
	[[nodiscard]] std::vector<Entry> search(const std::string& base_dn, const std::string& filter, const std::vector<std::string> requested_attr);

private:
	Ldap(LDAP* conn);

	std::unique_ptr<LDAP, int(*)(LDAP*)> m_conn;
	std::optional<std::string> m_lastBindDn;
	std::optional<std::string> m_lastBindPw;
};

std::vector<std::string> getGroupsForUser(const std::unique_ptr<shv::ldap::Ldap>& my_ldap, const std::string& base_dn, const std::vector<std::string>& field_names, const std::string& user_name);
std::map<std::string, std::vector<std::string>> getAllUsersWithGroups(const std::unique_ptr<shv::ldap::Ldap>& my_ldap, const std::string& base_dn, const std::vector<std::string>& field_names);
}
