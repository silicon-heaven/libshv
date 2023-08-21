#include "acluser.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv::iotqt::acl {

AclUser::AclUser() = default;

AclUser::AclUser(AclPassword p, std::vector<std::string> roles_)
	: password(std::move(p))
	, roles(std::move(roles_))
{}

shv::chainpack::RpcValue AclUser::toRpcValue() const
{
	return shv::chainpack::RpcValue::Map {
		{"password", password.toRpcValue()},
		{"roles", shv::chainpack::RpcValue::List::fromStringList(roles)},
	};
}

AclUser AclUser::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclUser ret;
	if(v.isMap()) {
		const auto &m = v.asMap();
		ret.password = AclPassword::fromRpcValue(m.value("password"));
		std::vector<std::string> roles;
		for(const auto &lst : m.valref("roles").asList())
			roles.push_back(lst.toString());
		// legacy key for 'roles' was 'grants'
		for(const auto &lst : m.valref("grants").asList())
			roles.push_back(lst.toString());
		ret.roles = roles;
	}
	return ret;
}

bool AclUser::isValid() const
{
	return password.isValid();
}

} // namespace shv
