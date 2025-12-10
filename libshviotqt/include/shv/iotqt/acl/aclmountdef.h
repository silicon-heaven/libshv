#pragma once

#include <shv/iotqt/shviotqt_export.h>

#include <string>

namespace shv::chainpack { class RpcValue; }

namespace shv::iotqt::acl {

struct LIBSHVIOTQT_EXPORT AclMountDef
{
	std::string mountPoint;
	std::string description;

	bool isValid() const;
	shv::chainpack::RpcValue toRpcValue() const;
	static AclMountDef fromRpcValue(const shv::chainpack::RpcValue &v);
};
} // namespace shv::iotqt::acl
