#pragma once

#include <shv/iotqt/shviotqtglobal.h>

#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <vector>
#include <limits>

namespace shv::iotqt::acl {

struct SHVIOTQT_DECL_EXPORT AclRole
{
	std::vector<std::string> roles;
	shv::chainpack::RpcValue profile;

	AclRole();
	AclRole(std::vector<std::string> roles_);

	shv::chainpack::RpcValue toRpcValue() const;
	static std::optional<AclRole> fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace shv::iotqt::acl
