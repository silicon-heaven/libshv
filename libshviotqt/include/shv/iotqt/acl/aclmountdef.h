#pragma once

#include "../shviotqtglobal.h"

#include <string>

namespace shv { namespace chainpack { class RpcValue; } }

namespace shv {
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclMountDef
{
	std::string mountPoint;
	std::string description;

	bool isValid() const;
	shv::chainpack::RpcValue toRpcValue() const;
	static AclMountDef fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

