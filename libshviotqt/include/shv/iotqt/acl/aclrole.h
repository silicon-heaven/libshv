#pragma once

#include <shv/iotqt/shviotqt_export.h>

#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <vector>
#include <limits>
#include <optional>
	
namespace shv::iotqt::acl {

struct LIBSHVIOTQT_EXPORT AclRole
{
	std::vector<std::string> roles;
	shv::chainpack::RpcValue profile;

	AclRole();
	AclRole(std::vector<std::string> roles_);

	shv::chainpack::RpcValue toRpcValue() const;
	static std::optional<AclRole> fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace shv::iotqt::acl
