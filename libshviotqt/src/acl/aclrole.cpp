#include "aclrole.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv::iotqt::acl {

AclRole::AclRole() = default;

AclRole::AclRole(std::vector<std::string> roles_)
	: roles(std::move(roles_))
{
}

shv::chainpack::RpcValue AclRole::toRpcValue() const
{
	shv::chainpack::RpcValue::Map m;
	if(!roles.empty())
		m["roles"] = shv::chainpack::RpcValue::List::fromStringList(roles);
	if(profile.isMap())
		m["profile"] = profile;
	return m;
}

AclRole AclRole::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclRole ret;
	if(v.isMap()) {
		const auto &m = v.asMap();
		std::vector<std::string> roles;
		for(const auto &lst : m.valref("roles").asList())
			roles.push_back(lst.toString());
		// legacy key for 'roles' was 'grants'
		for(const auto &lst : m.valref("grants").asList())
			roles.push_back(lst.toString());
		ret.roles = roles;
		ret.profile = m.value("profile");
		if(!ret.profile.isMap())
			ret.profile = shv::chainpack::RpcValue();
	}
	return ret;
}

} // namespace shv
