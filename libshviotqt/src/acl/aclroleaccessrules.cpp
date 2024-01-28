#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/log.h>
#include <shv/core/utils/shvurl.h>

#include <QString>

using namespace shv::chainpack;
using namespace std;

namespace shv::iotqt::acl {

//================================================================
// PathAccessGrant
//================================================================
AclAccessRule::AclAccessRule() = default;

AclAccessRule::AclAccessRule(const std::string &path_pattern_, const std::string &method_)
	: pathPattern(path_pattern_)
	, method(method_)
{
}

AclAccessRule::AclAccessRule(const std::string &path_pattern_, const std::string &method_, const shv::chainpack::AccessGrant &grant_)
	: pathPattern(path_pattern_)
	, method(method_)
	, grant(grant_)
{
}

RpcValue AclAccessRule::toRpcValue() const
{
	RpcValue::Map m = grant.toRpcValueMap().asMap();
	m["service"] = service;
	m["method"] = method;
	m["pathPattern"] = pathPattern;
	return m;
}

AclAccessRule AclAccessRule::fromRpcValue(const RpcValue &rpcval)
{
	AclAccessRule ret;
	shvDebug() << rpcval.toCpon();
	ret.grant = AccessGrant::fromRpcValue(rpcval);
	ret.service = rpcval.at("service").toString();
	ret.method = rpcval.at("method").toString();
	ret.pathPattern = rpcval.at("pathPattern").toString();
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
	return !pathPattern.empty() && grant.isValid();
}

bool AclAccessRule::isMoreSpecificThan(const AclAccessRule &other) const
{
	if(!isValid())
		return false;
	if(!other.isValid())
		return true;

	const bool has_any_service = service == ALL_SERVICES;
	const bool has_some_service = !service.empty() && !has_any_service;
	const bool other_has_any_service = other.service == ALL_SERVICES;
	const bool other_has_no_service = other.service.empty();
	if(has_some_service && (other_has_any_service || other_has_no_service))
		return true;
	if(has_any_service && other_has_no_service)
		return true;

	const bool is_exact_path = !is_wild_card_pattern(pathPattern);
	const bool other_is_exact_path = !is_wild_card_pattern(other.pathPattern);
	const bool has_method = !method.empty();
	const bool other_has_method = !other.method.empty();
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
	auto patt_len = pathPattern.length();
	auto other_patt_len = other.pathPattern.length();
	if(patt_len == other_patt_len) {
		return has_method && !other_has_method;
	}
	return patt_len > other_patt_len;
}

bool AclAccessRule::isPathMethodMatch(const shv::core::utils::ShvUrl &shv_url, const string &method_arg) const
{
	const bool any_service = this->service == ALL_SERVICES;
	const bool some_service = !this->service.empty() && !any_service;
	const bool no_service = this->service.empty();
	if(shv_url.service().empty()) {
		if(any_service || some_service)
			return false;
	}
	else {
		if(no_service)
			return false;
		if(some_service && !(this->service == service))
			return false;
	}
	// sevice check OK here
	bool is_exact_pattern_path = !is_wild_card_pattern(pathPattern);
	if(is_exact_pattern_path) {
		if(shv_url.pathPart() == pathPattern) {
			if(this->method.empty())
				return true;
			return this->method == method_arg;
		}
		return false;
	}
	shv::core::StringView patt(pathPattern);
	// trim "**"
	patt = patt.substr(0, patt.length() - 2);
	if(patt.length() > 0)
		patt = patt.substr(0, patt.length() - 1); // trim '/'
	if(shv::core::utils::ShvPath::startsWithPath(shv_url.pathPart(), patt)) {
		if(this->method.empty())
			return true;
		return this->method == method_arg;
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
		std::string key = kv.pathPattern;
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
				g.pathPattern = kv.first;
			}
			else {
				g.pathPattern = kv.first.substr(0, i);
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
