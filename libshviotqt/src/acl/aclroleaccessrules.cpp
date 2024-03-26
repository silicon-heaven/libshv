#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/log.h>
#include <shv/core/utils/shvurl.h>
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
/*
void AclAccessRule::setAccess(const std::string &access_, std::optional<int> access_level)
{
	if (access_level) {
		accessLevel = MetaMethod::accessLevelFromInt(access_level.value()).value_or(MetaMethod::AccessLevel::None);
	}
	else {
		for (const auto &sv : shv::core::utils::split(access_, ',')) {
			std::string s(sv);
			shv::core::String::trim(s);
			if (auto level = MetaMethod::accessLevelFromString(s); level.has_value()) {
				accessLevel = level.value();
			}
		}
	}
	access = access_;
}
*/
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
	return shv::core::String::endsWith(path, SLASH_ASTERISKS);
}
}

bool AclAccessRule::isValid() const
{
	return !path.empty() && !access.empty();
}
/*
bool AclAccessRule::isMoreSpecificThan(const AclAccessRule &other) const
{
	if(!isValid())
		return false;
	if(!other.isValid())
		return true;

	const bool is_exact_path = !is_wild_card_pattern(path);
	const bool other_is_exact_path = !is_wild_card_pattern(other.path);
	const bool has_method = !signal.empty();
	const bool other_has_method = !other.signal.empty();
	if(is_exact_path && other_is_exact_path) {
		return has_method && !other_has_method;
	}
	if(is_exact_path && !other_is_exact_path) {
		return true;
	}
	if(!is_exact_path && other_is_exact_path) {
		return false;
	}
	// both path patterns with wild-card
	auto patt_len = path.length();
	auto other_patt_len = other.path.length();
	if(patt_len == other_patt_len) {
		return has_method && !other_has_method;
	}
	return patt_len > other_patt_len;
}
*/
bool AclAccessRule::isPathMethodMatch(const shv::core::utils::ShvUrl &shv_url, const string &method_) const
{
	// sevice check OK here
	bool is_exact_pattern_path = !is_wild_card_pattern(path);
	if(is_exact_pattern_path) {
		if(shv_url.pathPart() == path) {
			return (method.empty() || method == method_);
		}
		return false;
	}
	shv::core::StringView patt(path);
	// trim "**"
	patt = patt.substr(0, patt.length() - 2);
	if(patt.length() > 0)
		patt = patt.substr(0, patt.length() - 1); // trim '/'
	if(shv::core::utils::ShvPath::startsWithPath(shv_url.pathPart(), patt)) {
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
