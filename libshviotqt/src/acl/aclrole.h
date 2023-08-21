#pragma once

#include "../shviotqtglobal.h"

#include "shv/chainpack/rpcvalue.h"

#include <string>
#include <vector>
#include <limits>

namespace shv {
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclRole
{
	std::vector<std::string> roles;
	shv::chainpack::RpcValue profile;

	AclRole();
	AclRole(std::vector<std::string> roles_);

	shv::chainpack::RpcValue toRpcValue() const;
	static AclRole fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

