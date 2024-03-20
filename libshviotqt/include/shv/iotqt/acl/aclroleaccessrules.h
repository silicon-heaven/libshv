#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/accessgrant.h>
#include <shv/core/utils/shvpath.h>

#include <variant>

namespace shv {
namespace core { namespace utils { class ShvUrl; } }
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclAccessRule
{
public:
	std::string service;
	std::string pathPattern;
	std::string method;
	std::variant<std::string, chainpack::MetaMethod::AccessLevel> grant;

	static constexpr auto ALL_SERVICES = "*";

	AclAccessRule();
	AclAccessRule(const std::string &path_pattern_, const std::string &method_ = std::string());
	AclAccessRule(const std::string &path_pattern_, const std::string &method_, const std::string &grant_);

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const;
	bool isMoreSpecificThan(const AclAccessRule &other) const;
	bool isPathMethodMatch(const shv::core::utils::ShvUrl &shv_url, const std::string &method) const;
};

class SHVIOTQT_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
public:
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;

	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

