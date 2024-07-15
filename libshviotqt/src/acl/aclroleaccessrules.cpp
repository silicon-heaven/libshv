#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/log.h>
#include <shv/core/utils.h>

#include <QString>

using namespace shv::chainpack;
using namespace std;

namespace shv::iotqt::acl {

//================================================================
// PathAccessGrant
//================================================================
AclAccessRule::AclAccessRule() = default;

AclAccessRule::AclAccessRule(const std::string &path_, const std::string &method_, const string &access_)
	: path(path_)
	, method(method_)
	, access(access_)
{
}

RpcValue AclAccessRule::toRpcValue() const
{
	RpcValue::Map m;
	m["role"] = access;
	m["role"] = access;
	m["method"] = method;
	m["pathPattern"] = path;
	return m;
}

AclAccessRule AclAccessRule::fromRpcValue(const RpcValue &rpcval)
{
	AclAccessRule ret;
	shvDebug() << rpcval.toCpon();
	ret.access = (rpcval.at("role").asString());
	ret.method = rpcval.at("method").asString();
	ret.path = rpcval.at("pathPattern").asString();
	return ret;
}

namespace {
bool is_wild_card_pattern(const string path)
{
	static const string ASTERISKS = "**";
	static const string SLASH_ASTERISKS = "/**";
	if(path == ASTERISKS)
		return true;
	return path.ends_with(SLASH_ASTERISKS);
}
}

bool AclAccessRule::isValid() const
{
	return !path.empty() && !access.empty();
}

bool AclAccessRule::isPathMethodMatch(std::string_view shv_path, const string &method_) const
{
	bool is_exact_pattern_path = !is_wild_card_pattern(path);
	if(is_exact_pattern_path) {
		if(shv_path == path) {
			return (method.empty() || method == method_);
		}
		return false;
	}
	shv::core::StringView sub_path(path);
	// trim "**"
	sub_path = sub_path.substr(0, sub_path.length() - 2);
	if(!sub_path.empty())
		sub_path = sub_path.substr(0, sub_path.length() - 1); // trim '/'
	if(shv::core::utils::ShvPath::startsWithPath(shv_path, sub_path)) {
		return (method.empty() || method == method_);
	}
	return false;
}

//================================================================
// AclRolePaths
//================================================================
shv::chainpack::RpcValue AclRoleAccessRules::toRpcValue() const
{
	shv::chainpack::RpcValue::List ret;
	for(const auto &kv : *this) {
		ret.push_back(kv.toRpcValue());
	}
	return shv::chainpack::RpcValue(std::move(ret));
}

RpcValue AclRoleAccessRules::toRpcValue_legacy() const
{
	shv::chainpack::RpcValue::Map ret;
	for(const auto &kv : *this) {
		std::string key = kv.path;
		if(!kv.method.empty())
			key += shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM + kv.method;
		ret[key] = kv.toRpcValue();
	}
	return shv::chainpack::RpcValue(std::move(ret));
}

AclRoleAccessRules AclRoleAccessRules::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclRoleAccessRules ret;
	if(v.isMap()) {
		const auto &m = v.asMap();
		for(const auto &kv : m) {
			auto g = AclAccessRule::fromRpcValue(kv.second);
			auto i = kv.first.find_last_of(shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM);
			if(i == std::string::npos) {
				g.path = kv.first;
			}
			else {
				g.path = kv.first.substr(0, i);
				g.method = kv.first.substr(i + 1);
			}
			if(g.isValid())
				ret.push_back(std::move(g));
		}
	}
	else if(v.isList()) {
		const auto &l = v.asList();
		for(const auto &kv : l) {
			auto g = AclAccessRule::fromRpcValue(kv);
			if(g.isValid())
				ret.push_back(std::move(g));
		}
	}
	return ret;
}

} // namespace shv
