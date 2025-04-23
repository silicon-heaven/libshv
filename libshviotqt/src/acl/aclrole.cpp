#include <shv/iotqt/acl/aclrole.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/exception.h>

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
		m["roles"] = shv::chainpack::RpcList::fromStringList(roles);
	if(profile.isMap())
		m["profile"] = profile;
	return m;
}

std::optional<AclRole> AclRole::fromRpcValue(const shv::chainpack::RpcValue &v)
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
	else if(v.isNull()) {
		return std::nullopt;
	}
	else {
		throw shv::core::Exception("Invalid parameter type: " + std::string(v.typeName()));
	}
	return ret;
}
} // namespace shv
