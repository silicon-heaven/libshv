#include "shvalarm.h"
#include "shvtypeinfo.h"

#include <shv/chainpack/rpcvalue.h>
#include <necrolog.h>

using namespace std;

namespace shv::core::utils {

ShvAlarm::ShvAlarm()
	: m_isActive(false)
	, m_severity(Severity::Invalid)
{}

ShvAlarm::ShvAlarm(const std::string &path,  bool is_active, Severity severity, int level, const std::string &description)
	: m_path(path)
	, m_isActive(is_active)
	, m_description(description)
	, m_level(level)
	, m_severity(severity)
{}

ShvAlarm::Severity ShvAlarm::severityFromString(const std::string &lvl)
{
	return NecroLog::stringToLevel(lvl.c_str());
}

const char* ShvAlarm::severityName() const
{
	return NecroLog::levelToString(m_severity);
}

bool ShvAlarm::isLessSevere(const ShvAlarm &a) const
{
	if (m_severity == a.m_severity)
		return m_level < a.m_level;
	if(m_severity == Severity::Invalid)
		return true;
	return (m_severity > a.m_severity);
}

bool ShvAlarm::operator==(const ShvAlarm &a) const
{
	return m_severity == a.m_severity
			&& m_isActive == a.m_isActive
			&& m_level == a.m_level
			&& m_path == a.m_path;
}

shv::chainpack::RpcValue ShvAlarm::toRpcValue(bool all_fields_if_not_active) const
{
	shv::chainpack::RpcValue::Map ret;
	ret["path"] = path();
	ret["isActive"] = isActive();
	if(all_fields_if_not_active || isActive()) {
		if(severity() != Severity::Invalid) {
			ret["severity"] = static_cast<int>(severity());
			ret["severityName"] = severityName();
		}
		if(level() > 0)
			ret["alarmLevel"] = level();
		if(!description().empty())
			ret["description"] = description();
	}
	return ret;
}

ShvAlarm ShvAlarm::fromRpcValue(const chainpack::RpcValue &rv)
{
	const chainpack::RpcValue::Map &m = rv.asMap();
	ShvAlarm a {
		m.value("path").asString(),
		m.value("isActive").toBool(),
		static_cast<Severity>(m.value("severity").toInt()),
		m.value("alarmLevel").toInt(),
		m.value("description").asString(),
	};
	return a;
}

vector<ShvAlarm> ShvAlarm::checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const chainpack::RpcValue &value)
{
	if(value.isNull()) {
		// value not available, keep previous alarms active
		return {};
	}
	if(auto path_info = type_info.pathInfo(shv_path); path_info.propertyDescription.isValid()) {
		nDebug() << shv_path << path_info.propertyDescription.toRpcValue().toCpon();
		if(string alarm = path_info.propertyDescription.alarm(); !alarm.empty()) {
			return {ShvAlarm(shv_path,
					value.toBool(), // is active
					ShvAlarm::severityFromString(alarm),
					path_info.propertyDescription.alarmLevel(),
					path_info.propertyDescription.alarmDescription()
				)};
		}

		return checkAlarms(type_info, shv_path, path_info.propertyDescription.typeName(), value);
	}
	return {};
}

std::vector<ShvAlarm> ShvAlarm::checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const std::string &type_name, const chainpack::RpcValue &value)
{
	if(ShvTypeDescr type_descr = type_info.findTypeDescription(type_name); type_descr.isValid()) {
		if (type_descr.type() == ShvTypeDescr::Type::BitField) {
			vector<ShvAlarm> alarms;
			auto flds = type_descr.fields();
			for (auto& fld_descr : flds) {
				if(string alarm = fld_descr.alarm(); !alarm.empty()) {
					bool is_alarm = fld_descr.bitfieldValue(value.toUInt64()).toBool();
					alarms.emplace_back(shv_path + '/' + fld_descr.name(),
						is_alarm,
						ShvAlarm::severityFromString(alarm),
						fld_descr.alarmLevel(),
						fld_descr.alarmDescription()
					);
				}
				else {
					auto alarms2 = checkAlarms(type_info, shv_path + '/' + fld_descr.name(), fld_descr.typeName(), fld_descr.bitfieldValue(value.toUInt64()));
					alarms.insert(alarms.end(), alarms2.begin(), alarms2.end());
				}
			}
			return alarms;
		}
		if (type_descr.type() == ShvTypeDescr::Type::Enum) {
			bool has_alarm_definition = false;
			auto flds = type_descr.fields();
			size_t active_alarm_ix = flds.size();
			for (size_t i = 0; i < flds.size(); ++i) {
				const ShvFieldDescr &fld_descr = flds[i];
				if(string alarm = fld_descr.alarm(); !alarm.empty()) {
					has_alarm_definition = true;
					if(value == fld_descr.value())
						active_alarm_ix = i;
				}
			}

			if (has_alarm_definition) {
				if (active_alarm_ix < flds.size()) {
					const ShvFieldDescr &fld_descr = flds[active_alarm_ix];
					return {ShvAlarm(shv_path,
							active_alarm_ix < flds.size(),
							ShvAlarm::severityFromString(fld_descr.alarm()),
							fld_descr.alarmLevel(),
							fld_descr.alarmDescription()
						)};
				}
				else {
					return {ShvAlarm(shv_path)};
				}
			}
		}
	}
	return {};
}


} // namespace shv
