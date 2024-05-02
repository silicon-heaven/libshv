#pragma once

#include "../shvcoreglobal.h"

#include <necrologlevel.h>

#include <string>
#include <vector>

namespace shv::chainpack { class RpcValue; }

namespace shv::core::utils {

class ShvTypeInfo;

class SHVCORE_DECL_EXPORT ShvAlarm {
public:
	using Severity = NecroLogLevel;
public:
	ShvAlarm();
	explicit ShvAlarm(const std::string &path, bool is_active = false, Severity severity = Severity::Invalid, int level = 0, const std::string &description = {}, const std::string &label = {});
public:
	static Severity severityFromString(const std::string &lvl);
	const char *severityName() const;
	bool isLessSevere(const ShvAlarm &a) const;
	bool operator==(const ShvAlarm &a) const;

	Severity severity() const;
	void setSeverity(Severity severity);

	const std::string& path() const;
	void setPath(const std::string &path);

	const std::string& description() const;
	void setDescription(const std::string &description);

	const std::string& label() const;
	void setLabel(const std::string &label);

	int level() const;
	void setLevel(int level);

	bool isActive() const;
	void setIsActive(bool is_active);

	bool isValid() const;

	shv::chainpack::RpcValue toRpcValue(bool all_fields_if_not_active = false) const;
	static ShvAlarm fromRpcValue(const shv::chainpack::RpcValue &rv);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const shv::chainpack::RpcValue &value);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const std::string &type_name, const shv::chainpack::RpcValue &value);
protected:
	std::string m_path;
	bool m_isActive = false;
	std::string m_description;
	std::string m_label;
	int m_level = 0;
	Severity m_severity = Severity::Invalid;
};
} // namespace shv::core::utils
