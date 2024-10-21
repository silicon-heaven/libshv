#pragma once

#include <shv/iotqt/shviotqtglobal.h>

#include <shv/chainpack/accessgrant.h>
#include <shv/core/utils/shvpath.h>

namespace shv {
namespace core::utils { class ShvUrl; }
namespace iotqt::acl {

struct SHVIOTQT_DECL_EXPORT AclAccessRule
{
public:
	std::string path;
	std::string method;
	std::string access;

	AclAccessRule();
	AclAccessRule(const std::string &path_, const std::string &signal_, const std::string &access_);

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const;
	bool isPathMethodMatch(std::string_view shv_path, const std::string &signal_) const;
};

class SHVIOTQT_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
public:
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;

	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace iotqt::acl
} // namespace shv
