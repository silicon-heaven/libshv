#pragma once

#include <shv/core/shvcoreglobal.h>

#include <necrolog/necrologlevel.h>

#include <string>
#include <vector>

namespace shv::chainpack { class RpcValue; }

namespace shv::core::utils {

class ShvTypeInfo;

class SHVCORE_DECL_EXPORT ShvAlarm {
public:
	using Severity = NecroLogLevel;
	std::string path;
	bool isActive = false;
	std::string description;
	std::string label;
	int level = 0;
	Severity severity = Severity::Invalid;
public:
	static Severity severityFromString(const std::string &lvl);
	const char *severityName() const;
	bool isLessSevere(const ShvAlarm &a) const;
	bool operator==(const ShvAlarm &a) const;

	bool isValid() const;

	shv::chainpack::RpcValue toRpcValue(bool all_fields_if_not_active = false) const;
	static ShvAlarm fromRpcValue(const shv::chainpack::RpcValue &rv);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const shv::chainpack::RpcValue &value);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const std::string &type_name, const shv::chainpack::RpcValue &value);
	static std::vector<ShvAlarm> checkStateAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const shv::chainpack::RpcValue &value);
};
} // namespace shv::core::utils
