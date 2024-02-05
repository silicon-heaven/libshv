#pragma once

#include <ldap.h>
namespace shv::ldap::OpenLDAP {
	// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
	extern decltype(&ber_free) ber_free;
	extern decltype(&ldap_connect) ldap_connect;
	extern decltype(&ldap_destroy) ldap_destroy;
	extern decltype(&ldap_err2string) ldap_err2string;
	extern decltype(&ldap_first_attribute) ldap_first_attribute;
	extern decltype(&ldap_first_entry) ldap_first_entry;
	extern decltype(&ldap_get_values_len) ldap_get_values_len;
	extern decltype(&ldap_initialize) ldap_initialize;
	extern decltype(&ldap_msgfree) ldap_msgfree;
	extern decltype(&ldap_next_attribute) ldap_next_attribute;
	extern decltype(&ldap_next_entry) ldap_next_entry;
	extern decltype(&ldap_sasl_bind_s) ldap_sasl_bind_s;
	extern decltype(&ldap_search_ext_s) ldap_search_ext_s;
	extern decltype(&ldap_set_option) ldap_set_option;
	extern decltype(&ldap_set_rebind_proc) ldap_set_rebind_proc;
	extern decltype(&ldap_value_free_len) ldap_value_free_len;
	// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}
