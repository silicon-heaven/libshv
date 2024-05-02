#pragma once

#include "../shviotqtglobal.h"
#include "aclpassword.h"

#include <vector>

namespace shv::chainpack { class RpcValue; }
namespace shv::iotqt::acl {

struct SHVIOTQT_DECL_EXPORT AclUser
{
	AclPassword password;
	std::vector<std::string> roles;

	AclUser();
	AclUser(AclPassword p, std::vector<std::string> roles_);

	bool isValid() const;
	shv::chainpack::RpcValue toRpcValue() const;
	static AclUser fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace shv::iotqt::acl
