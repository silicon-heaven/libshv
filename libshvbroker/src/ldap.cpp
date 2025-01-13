#include "openldap_dynamic.h"

#include <shv/broker/ldap/ldap.h>
#include <shv/core/log.h>

#include <QScopeGuard>
#include <QString>
#include <QStringLiteral>

#define ldapI() shvCInfo("ldap")
#define ldapE() shvCError("ldap")

namespace shv::ldap {
std::vector<std::string> getGroupsForUser(const std::unique_ptr<shv::ldap::Ldap>& my_ldap, const std::string& base_dn, const std::vector<std::string>& field_names, const std::string& user_name) {
	std::vector<std::string> res;
	auto filter = QStringLiteral("(|");
	for (const auto& field_name : field_names) {
		filter += QStringLiteral(R"((%1=%2))").arg(field_name.data(), user_name.data());
	}
	filter += ')';
	auto entries = my_ldap->search(base_dn, qPrintable(filter), {"memberOf"});
	for (const auto& entry : entries) {
		for (const auto& [key, values]: entry.keysAndValues) {
			for (const auto& v : values) {
				res.emplace_back(v);
				auto derived_groups = getGroupsForUser(my_ldap, v, {"objectClass"}, "*");
				std::copy(derived_groups.begin(), derived_groups.end(), std::back_inserter(res));
			}
		}
	}

	return res;
}

std::map<std::string, std::vector<std::string>> getAllUsersWithGroups(const std::unique_ptr<shv::ldap::Ldap>& my_ldap, const std::string& base_dn, const std::vector<std::string>& field_names)
{
	std::map<std::string, std::vector<std::string>> res;
	for (const auto& user : my_ldap->search(base_dn, "objectClass=person", field_names)) {
		for (const auto& [key, values] : user.keysAndValues) {
			for (const auto& value : values) {
				res[value] = getGroupsForUser(my_ldap, base_dn, field_names, value);
			}
		}
	}

	return res;
}

namespace {
template <typename Type, typename DeleteFunc>
auto wrap(Type* ptr, DeleteFunc delete_func)
{
	return std::unique_ptr<Type, decltype(delete_func)>(ptr, delete_func);
}

void throwIfError(int code, const std::string& err_msg)
{
	if (code) {
		throw LdapError(err_msg + ": " + OpenLDAP::ldap_err2string(code) + " (" + std::to_string(code) + ")", code);
	}
}
}

LdapError::LdapError(const std::string& err_string, int code)
	: std::runtime_error(err_string)
	, m_code(code)
{
	ldapE() << err_string;
}

int LdapError::code() const
{
	return m_code;
}

namespace {
int rebind_proc(LDAP* /*ld*/, const char* /*url*/, ber_tag_t /*request*/, ber_int_t /*msgid*/, void* user_data)
{
	auto ldap = static_cast<Ldap*>(user_data);
	try {
		ldap->bindWithLastCreds();
	} catch (LdapError& ex) {
		return ex.code();
	} catch (std::exception& ex) {
		ldapE() << ex.what();
		return LDAP_OTHER;
	}

	return LDAP_SUCCESS;
}
}

Ldap::Ldap(LDAP* conn)
	: m_conn(conn, OpenLDAP::ldap_destroy)
{
	auto ret = OpenLDAP::ldap_set_rebind_proc(m_conn.get(), rebind_proc, this);
	throwIfError(ret, "Couldn't set rebind procedure");
}

std::unique_ptr<Ldap> Ldap::create(const std::string& hostname)
{
	LDAP* conn;
	auto ret = OpenLDAP::ldap_initialize(&conn, hostname.data());
	throwIfError(ret, "Couldn't initialize LDAP context");
	return std::unique_ptr<Ldap>(new Ldap(conn));
}

void Ldap::setVersion(Version version)
{
	int ver_int = static_cast<int>(version);
	auto ret = OpenLDAP::ldap_set_option(m_conn.get(), LDAP_OPT_PROTOCOL_VERSION, &ver_int);
	throwIfError(ret, "Couldn't set LDAP version");
}

void Ldap::connect()
{
	auto ret = OpenLDAP::ldap_connect(m_conn.get());
	throwIfError(ret, "Couldn't connect to the LDAP server");
}

void Ldap::bindSasl(const std::string& bind_dn, const std::string& bind_pw)
{
	auto pw_copy = wrap(strdup(bind_pw.data()), std::free); // Yes, you have to make a copy.
	berval cred {
		.bv_len = bind_pw.size(),
		.bv_val = pw_copy.get(),
	};
	auto ret = OpenLDAP::ldap_sasl_bind_s(m_conn.get(), bind_dn.data(), LDAP_SASL_SIMPLE, &cred, nullptr, nullptr, nullptr);
	throwIfError(ret, "Couldn't authenticate to the LDAP server");

	m_lastBindDn = bind_dn;
	m_lastBindPw = bind_pw;
}

void Ldap::bindWithLastCreds()
{
	if (!m_lastBindDn.has_value() || !m_lastBindPw.has_value()) {
		throw std::runtime_error("bindWithLastCreds called with no last creds");
	}
	bindSasl(m_lastBindDn.value(), m_lastBindPw.value());
}

std::vector<Entry> Ldap::search(const std::string& base_dn, const std::string& filter, const std::vector<std::string> requested_attr)
{
	ldapI() << "search, base_dn:" << base_dn << "filter:" << filter << "requested_attr:" << [&requested_attr] {
		std::string res;
		for (const auto& attr : requested_attr) {
			res += attr + ", ";
		}
		return res;
	}();
	// I can't think of a better way of doing this while keeping the input arguments the same.
	// NOLINTNEXTLINE(modernize-avoid-c-arrays) - we have to make an array for the C api
	auto attr_array = std::make_unique<char*[]>(requested_attr.size() + 1);
	std::transform(requested_attr.begin(), requested_attr.end(), attr_array.get(), [] (const std::string& str) {
		return strdup(str.c_str());
	});
	auto attr_array_deleter = qScopeGuard([&] {
		std::for_each(attr_array.get(), attr_array.get() + requested_attr.size() + 1, std::free);
	});

	LDAPMessage* msg;
	auto ret = OpenLDAP::ldap_search_ext_s(m_conn.get(), base_dn.data(), LDAP_SCOPE_SUBTREE, filter.data(), attr_array.get(), 0, nullptr, nullptr, nullptr, 0, &msg);
	throwIfError(ret, "Couldn't complete search");
	auto msg_deleter = wrap(msg, OpenLDAP::ldap_msgfree);

	std::vector<Entry> res;
	for (auto entry = OpenLDAP::ldap_first_entry(m_conn.get(), msg); entry != nullptr; entry = OpenLDAP::ldap_next_entry(m_conn.get(), entry)) {
		auto& res_entry = res.emplace_back();
		BerElement* ber;
		for (auto attr = wrap(OpenLDAP::ldap_first_attribute(m_conn.get(), entry, &ber), std::free); attr != nullptr; attr = wrap(OpenLDAP::ldap_next_attribute(m_conn.get(), entry, ber), std::free)) {
			auto& res_attr = res_entry.keysAndValues.try_emplace(attr.get(), std::vector<std::string>{}).first->second;
			auto values = wrap(OpenLDAP::ldap_get_values_len(m_conn.get(), entry, attr.get()), OpenLDAP::ldap_value_free_len);
			for (auto value = values.get(); *value; value++) {
				auto value_deref = *value;
				res_attr.emplace_back(value_deref->bv_val, value_deref->bv_len);
			}
		}
		OpenLDAP::ber_free(ber, 0);
	}
	return res;
}
}
