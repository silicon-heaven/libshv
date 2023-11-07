#include "aclmountdef.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace iotqt {
namespace acl {

shv::chainpack::RpcValue AclMountDef::toRpcValue() const
{
	shv::chainpack::RpcValue::Map m {
		{"mountPoint", mountPoint},
	};
	if(!description.empty())
		m["description"] = description;
	return m;
}

AclMountDef AclMountDef::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclMountDef ret;
	if(v.isString()) {
		ret.mountPoint = v.toString();
	}
	else if(v.isMap()) {
		const auto &m = v.asMap();
		ret.description = m.value("description").asString();
		ret.mountPoint = m.value("mountPoint").asString();
	}
	return ret;
}

} // namespace acl
} // namespace iotqt
} // namespace shv
