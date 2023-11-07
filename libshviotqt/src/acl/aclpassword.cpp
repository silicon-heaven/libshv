#include "aclpassword.h"

#include <shv/chainpack/rpcvalue.h>

#include <regex>

namespace shv {
namespace iotqt {
namespace acl {

static bool str_eq(const std::string &s1, const char *s2)
{
	size_t i;
	for (i = 0; i < s1.size(); ++i) {
		char c2 = s2[i];
		if(!c2)
			return false;
		if(toupper(c2) != toupper(s1[i]))
			return false;
	}
	return s2[i] == 0;
}

shv::chainpack::RpcValue AclPassword::toRpcValue() const
{
	return shv::chainpack::RpcValue::Map {
		{"password", password},
		{"format", formatToString(format)},
	};
}

AclPassword AclPassword::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	const std::regex sha1_regex("[0-9a-f]{40}");
	AclPassword ret;
	if(v.isString()) {
		ret.password = v.toString();
		// ffee1086753ca76fe84b7fadd738d76caf2072d0
		ret.format = std::regex_match(ret.password, sha1_regex)? Format::Sha1: Format::Plain;
	}
	else if(v.isMap()) {
		const auto &m = v.asMap();
		ret.password = m.value("password").asString();
		ret.format = formatFromString(m.value("format").asString());
		if(ret.format == Format::Invalid && !ret.password.empty())
			ret.format = std::regex_match(ret.password, sha1_regex)? Format::Sha1: Format::Plain;
	}
	return ret;
}

const char* AclPassword::formatToString(AclPassword::Format f)
{
	switch(f) {
	case Format::Plain: return "PLAIN";
	case Format::Sha1: return "SHA1";
	default: return "INVALID";
	}
}

AclPassword::Format AclPassword::formatFromString(const std::string &s)
{
	if(str_eq(s, formatToString(Format::Plain)))
		return Format::Plain;
	if(str_eq(s, formatToString(Format::Sha1)))
		return Format::Sha1;
	return Format::Invalid;
}

} // namespace acl
} // namespace iotqt
} // namespace shv
