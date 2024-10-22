#include <shv/core/log.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/rpcvalue.h>


NecroLog operator<<(NecroLog log, const shv::core::StringView &v)
{
	return log.operator<<(v);
}

NecroLog operator<<(NecroLog log, const shv::chainpack::RpcValue &v)
{
	return log.operator <<(v.isValid()? v.toCpon(): "Invalid");
}


NecroLog operator<<(NecroLog log, const shv::core::utils::ShvPath &v)
{
	return log.operator<<(v.asString());
}
