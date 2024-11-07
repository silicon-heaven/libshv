#include <shv/core/utils/shvalarm.h>
#include <shv/core/utils/shvtypeinfo.h>

#include <shv/chainpack/rpcvalue.h>
#include <necrolog.h>

using namespace std;

namespace shv::core::utils {

ShvAlarm::Severity ShvAlarm::severityFromString(const std::string &lvl)
{
	return NecroLog::stringToLevel(lvl.c_str());
}

const char* ShvAlarm::severityName() const
{
	return NecroLog::levelToString(severity);
}

bool ShvAlarm::isLessSevere(const ShvAlarm &a) const
{
	if (severity == a.severity)
		return level < a.level;
	if(severity == Severity::Invalid)
		return true;
	return (severity > a.severity);
}

bool ShvAlarm::operator==(const ShvAlarm &a) const
{
	return severity == a.severity
			&& isActive == a.isActive
			&& level == a.level
			&& path == a.path;
}

bool ShvAlarm::isValid() const
{
	return !path.empty();
}


shv::chainpack::RpcValue ShvAlarm::toRpcValue(bool all_fields_if_not_active) const
{
	shv::chainpack::RpcValue::Map ret;
	ret["path"] = path;
	ret["isActive"] = isActive;
	if(all_fields_if_not_active || isActive) {
		if(severity != Severity::Invalid) {
			ret["severity"] = static_cast<int>(severity);
			ret["severityName"] = severityName();
		}
		if(level > 0)
			ret["alarmLevel"] = level;
		if(!description.empty())
			ret["description"] = description;
		if(!label.empty())
			ret["label"] = label;
	}
	return ret;
}

ShvAlarm ShvAlarm::fromRpcValue(const chainpack::RpcValue &rv)
{
	const chainpack::RpcValue::Map &m = rv.asMap();
	ShvAlarm a {
		.path = m.value("path").asString(),
		.isActive = m.value("isActive").toBool(),
		.description = m.value("description").asString(),
		.label = m.value("label").asString(),
		.level = m.value("alarmLevel").toInt(),
		.severity = static_cast<Severity>(m.value("severity").toInt()),
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
			return {ShvAlarm{
				.path = shv_path,
				.isActive = value.toBool(),
				.description = path_info.propertyDescription.description(),
				.label = path_info.propertyDescription.label(),
				.level = path_info.propertyDescription.alarmLevel(),
				.severity = ShvAlarm::severityFromString(alarm),
			}};
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
					alarms.emplace_back(ShvAlarm {
						.path = shv_path + '/' + fld_descr.name(),
						.isActive = is_alarm,
						.description = fld_descr.description(),
						.label = fld_descr.label(),
						.level = fld_descr.alarmLevel(),
						.severity = ShvAlarm::severityFromString(alarm),
					});
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
			if(has_alarm_definition) {
				if(active_alarm_ix < flds.size()) {
					const ShvFieldDescr &fld_descr = flds[active_alarm_ix];
					return {ShvAlarm{
						.path = shv_path,
						.isActive = true,
						.description = fld_descr.description(),
						.label = fld_descr.label(),
						.level = fld_descr.alarmLevel(),
						.severity = ShvAlarm::severityFromString(fld_descr.alarm()),
					}};
				}

				return {ShvAlarm{
					.path = shv_path,
					.isActive = false,
					.description = {},
					.label = {},
					.level = 0,
					.severity = Severity::Invalid,
				}};
			}
		}
	}
	return {};
}


} // namespace shv
