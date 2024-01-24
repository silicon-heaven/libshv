#include "openldap_dynamic.h"

namespace shv::ldap::OpenLDAP {
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
decltype(&::ber_free) ber_free = nullptr;
decltype(&::ldap_connect) ldap_connect = nullptr;
decltype(&::ldap_destroy) ldap_destroy = nullptr;
decltype(&::ldap_err2string) ldap_err2string = nullptr;
decltype(&::ldap_first_attribute) ldap_first_attribute = nullptr;
decltype(&::ldap_first_entry) ldap_first_entry = nullptr;
decltype(&::ldap_get_values_len) ldap_get_values_len = nullptr;
decltype(&::ldap_initialize) ldap_initialize = nullptr;
decltype(&::ldap_msgfree) ldap_msgfree = nullptr;
decltype(&::ldap_next_attribute) ldap_next_attribute = nullptr;
decltype(&::ldap_next_entry) ldap_next_entry = nullptr;
decltype(&::ldap_sasl_bind_s) ldap_sasl_bind_s = nullptr;
decltype(&::ldap_search_ext_s) ldap_search_ext_s = nullptr;
decltype(&::ldap_set_option) ldap_set_option = nullptr;
decltype(&::ldap_set_rebind_proc) ldap_set_rebind_proc = nullptr;
decltype(&::ldap_value_free_len) ldap_value_free_len = nullptr;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}
