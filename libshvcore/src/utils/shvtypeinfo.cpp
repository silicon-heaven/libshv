#include <shv/core/utils/shvtypeinfo.h>
#include <shv/core/utils/shvpath.h>
#include <shv/core/utils.h>
#include <shv/core/log.h>

#include <shv/chainpack/metamethod.h>

#include <algorithm>

using namespace shv::chainpack;

namespace shv::core::utils {

namespace {
constexpr auto KEY_DEVICE_TYPE = "deviceType";
constexpr auto KEY_TYPE_NAME = "typeName";
constexpr auto KEY_LABEL = "label";
constexpr auto KEY_DESCRIPTION = "description";
constexpr auto KEY_UNIT = "unit";
constexpr auto KEY_NAME = "name";
constexpr auto KEY_TYPE = "type";
constexpr auto KEY_VALUE = "value";
constexpr auto KEY_FIELDS = "fields";
constexpr auto KEY_SAMPLE_TYPE = "sampleType";
constexpr auto KEY_RESTRICTION_OF_DEVICE = "restrictionOfDevice";
constexpr auto KEY_RESTRICTION_OF_TYPE = "restrictionOfType";
constexpr auto KEY_SITE_SPECIFIC_LOCALIZATION = "siteSpecificLocalization";
constexpr auto KEY_TAGS = "tags";
constexpr auto KEY_METHODS = "methods";
constexpr auto KEY_BLACKLIST = "blacklist";
constexpr auto KEY_DEC_PLACES = "decPlaces";
constexpr auto KEY_VISUAL_STYLE = "visualStyle";
constexpr auto KEY_ALARM = "alarm";
constexpr auto KEY_ALARM_LEVEL = "alarmLevel";
}

//=====================================================================
// ShvTypeDescrBase
//=====================================================================
ShvDescriptionBase::ShvDescriptionBase() = default;

ShvDescriptionBase::ShvDescriptionBase(const chainpack::RpcValue &v)
	: m_data(v)
{
}

std::string ShvDescriptionBase::name() const
{
	return m_data.asMap().value(KEY_NAME).asString();
}

bool ShvDescriptionBase::isValid() const
{
	const auto has_name = hasName();
	const auto size = m_data.asMap().size();
	return size > 1 && has_name;
}

RpcValue ShvDescriptionBase::dataValue(const std::string &key, const chainpack::RpcValue &default_val) const
{
	const auto &m = m_data.asMap();
	return m.value(key, default_val);
}

bool ShvDescriptionBase::operator==(const ShvDescriptionBase &o) const
{
	return m_data == o.m_data;
}

bool ShvDescriptionBase::hasName() const
{
	const auto &m = m_data.asMap();
	return m.hasKey(KEY_NAME);
}

void ShvDescriptionBase::setName(const std::string &n)
{
	setDataValue(KEY_NAME, n);
}

bool ShvDescriptionBase::isEmpty() const
{
	return m_data.asMap().empty();
}

void ShvDescriptionBase::setDataValue(const std::string &key, const chainpack::RpcValue &val)
{
	if(!m_data.isMap()) {
		m_data = RpcValue::Map();
	}
	m_data.set(key, val);
}

void ShvDescriptionBase::setData(const chainpack::RpcValue &data)
{
	if(data.isMap()) {
		RpcValue::Map map = data.asMap();
		mergeTags(map);
		m_data = map;
	}
}

void ShvDescriptionBase::mergeTags(chainpack::RpcValue::Map &map)
{
	auto nh = map.extract(KEY_TAGS);
	if(!nh.empty()) {
		RpcValue::Map tags = nh.mapped().asMap();
		map.merge(tags);
	}
}
//=====================================================================
// ShvTypeDescrField
//=====================================================================
ShvFieldDescr::ShvFieldDescr() = default;

ShvFieldDescr::ShvFieldDescr(const std::string &name, const std::string &type_name, const chainpack::RpcValue &value, chainpack::RpcValue::Map &&tags)
{
	setData(std::move(tags));
	setName(name);
	setDataValue(KEY_TYPE_NAME, type_name);
	setDataValue(KEY_VALUE, value);
}

std::string ShvFieldDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

std::string ShvFieldDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

std::string ShvFieldDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

RpcValue ShvFieldDescr::value() const
{
	return dataValue(KEY_VALUE);
}

std::string ShvFieldDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE).asString();
}

std::string ShvFieldDescr::alarm() const
{
	return dataValue(KEY_ALARM).asString();
}

int ShvFieldDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

std::string ShvFieldDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

ShvFieldDescr &ShvFieldDescr::setTypeName(const std::string &type_name)
{
	setDataValue(KEY_TYPE_NAME, type_name);
	return *this;
}

ShvFieldDescr &ShvFieldDescr::setLabel(const std::string &label)
{
	setDataValue(KEY_LABEL, label);
	return *this;
}

ShvFieldDescr &ShvFieldDescr::setDescription(const std::string &description)
{
	setDataValue(KEY_DESCRIPTION, description);
	return *this;
}

ShvFieldDescr &ShvFieldDescr::setUnit(const std::string &unit)
{
	setDataValue(KEY_UNIT, unit);
	return *this;
}

ShvFieldDescr &ShvFieldDescr::setVisualStyleName(const std::string &visual_style_name)
{
	setDataValue(KEY_VISUAL_STYLE, visual_style_name);
	return *this;
}

ShvFieldDescr &ShvFieldDescr::setAlarm(const std::string &alarm)
{
	setDataValue(KEY_ALARM, alarm);
	return *this;
}

RpcValue ShvFieldDescr::toRpcValue() const
{
	RpcValue ret = m_data;
	if(description().empty() && ret.isMap())
		ret.set(KEY_DESCRIPTION, {});
	return ret;
}

ShvFieldDescr ShvFieldDescr::fromRpcValue(const RpcValue &v)
{
	ShvFieldDescr ret;
	ret.setData(v);
	return ret;
}

std::pair<unsigned, unsigned> ShvFieldDescr::bitRange() const
{
	unsigned bit_no1 = 0;
	unsigned bit_no2 = 0;
	if(value().isList()) {
		const auto val = value();
		const auto &lst = val.asList();
		bit_no1 = lst.value(0).toUInt();
		bit_no2 = lst.value(1).toUInt();
		if(bit_no2 < bit_no1) {
			shvError() << "Invalid bit specification:" << value().toCpon();
			bit_no2 = bit_no1;
		}
	}
	else {
		bit_no1 = bit_no2 = value().toUInt();
	}
	return std::pair<unsigned, unsigned>(bit_no1, bit_no2);
}

RpcValue ShvFieldDescr::bitfieldValue(uint64_t uval) const
{
	if(!isValid())
		return {};
	auto [bit_no1, bit_no2] = bitRange();
	uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 + 1));
	shvDebug() << "bits:" << bit_no1 << bit_no2 << (bit_no1 == bit_no2) << "uval:" << uval << "mask:" << mask;
	uval = (uval & mask) >> bit_no1;
	shvDebug() << "uval masked and rotated right by:" << bit_no1 << "bits, uval:" << uval;
	RpcValue result;
	if(bit_no1 == bit_no2)
		result = uval != 0;
	else
		result = uval;
	return result;
}

uint64_t ShvFieldDescr::setBitfieldValue(uint64_t bitfield, uint64_t uval) const
{
	if(!isValid())
		return uval;
	auto [bit_no1, bit_no2] = bitRange();
	uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 - bit_no1 + 1));
	uval &= mask;
	mask <<= bit_no1;
	uval <<= bit_no1;
	bitfield &= ~mask;
	bitfield |= uval;
	return bitfield;
}

//=====================================================================
// ShvTypeDescr
//=====================================================================
ShvTypeDescr::ShvTypeDescr() = default;

ShvTypeDescr::ShvTypeDescr(const std::string &type_name)
	: ShvTypeDescr(typeFromString(type_name))
{
}

ShvTypeDescr::ShvTypeDescr(Type t, SampleType st, chainpack::RpcValue::Map &&tags)
	: ShvTypeDescr(t, std::vector<ShvFieldDescr>(), st, std::move(tags))
{
}

ShvTypeDescr::ShvTypeDescr(Type t, std::vector<ShvFieldDescr> &&flds, SampleType st, chainpack::RpcValue::Map &&tags)
{
	m_data = RpcValue(std::move(tags));
	setType(t);
	setFields(flds);
	setSampleType(st);
}

ShvTypeDescr::Type ShvTypeDescr::type() const
{
	return static_cast<Type>(dataValue(KEY_TYPE).toInt());
}

ShvTypeDescr &ShvTypeDescr::setType(Type t)
{
	auto type_name = typeToString(t);
	setDataValue(KEY_TYPE, static_cast<int>(t));
	setName(type_name);
	return *this;
}

std::string ShvTypeDescr::typeName() const
{
	return name();
}

ShvTypeDescr &ShvTypeDescr::setTypeName(const std::string &type_name)
{
	auto t = typeFromString(type_name);
	setType(t);
	return *this;
}

std::vector<ShvFieldDescr> ShvTypeDescr::fields() const
{
	std::vector<ShvFieldDescr> ret;
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList())
		ret.push_back(ShvFieldDescr::fromRpcValue(rv));
	return ret;
}

ShvTypeDescr &ShvTypeDescr::setFields(const std::vector<ShvFieldDescr> &fields)
{
	RpcList cp_fields;
	for (const ShvFieldDescr &field : fields) {
		cp_fields.push_back(field.toRpcValue());
	}
	setDataValue(KEY_FIELDS, cp_fields);
	return *this;
}

ShvTypeDescr::SampleType ShvTypeDescr::sampleType() const
{
	auto i = dataValue(KEY_SAMPLE_TYPE).toInt();
	if(i == 0)
		return SampleType::Continuous;
	return static_cast<SampleType>(dataValue(KEY_SAMPLE_TYPE).toInt());
}

ShvTypeDescr &ShvTypeDescr::setSampleType(ShvTypeDescr::SampleType st)
{
	setDataValue(KEY_SAMPLE_TYPE, static_cast<int>(st));
	return *this;
}

std::string ShvTypeDescr::restrictionOfType() const
{
	return dataValue(KEY_RESTRICTION_OF_TYPE).asString();
}

bool ShvTypeDescr::isSiteSpecificLocalization() const
{
	return dataValue(KEY_SITE_SPECIFIC_LOCALIZATION).toBool();
}

bool ShvTypeDescr::isValid() const
{
	return type() != Type::Invalid;
}

ShvFieldDescr ShvTypeDescr::field(const std::string &field_name) const
{
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList()) {
		auto fld_descr = ShvFieldDescr::fromRpcValue(rv);
		if(fld_descr.name() == field_name) {
			return fld_descr;
		}
	}
	return {};
}

RpcValue ShvTypeDescr::fieldValue(const chainpack::RpcValue &val, const std::string &field_name) const
{
	switch (type()) {
	case Type::BitField:
		return field(field_name).bitfieldValue(val.toUInt64());
	case Type::Map:
		return val.asMap().value(field_name);
	case Type::Enum:
		return field(field_name).value().toInt();
	default:
		break;
	}
	return {};
}

std::string ShvTypeDescr::typeToString(ShvTypeDescr::Type t)
{
	switch (t) {
	case Type::Invalid: break;
	case Type::BitField: return "BitField";
	case Type::Enum: return "Enum";
	case Type::Bool: return "Bool";
	case Type::UInt: return "UInt";
	case Type::Int: return "Int";
	case Type::Decimal: return "Decimal";
	case Type::Double: return "Double";
	case Type::String: return "String";
	case Type::DateTime: return "DateTime";
	case Type::List: return "List";
	case Type::Map: return "Map";
	case Type::IMap: return "IMap";
	}
	return "";
}

ShvTypeDescr::Type ShvTypeDescr::typeFromString(const std::string &s)
{
	if(s == "BitField") return Type::BitField;
	if(s == "Enum") return Type::Enum;
	if(s == "Bool") return Type::Bool;
	if(s == "UInt") return Type::UInt;
	if(s == "Int") return Type::Int;
	if(s == "Decimal") return Type::Decimal;
	if(s == "Double") return Type::Double;
	if(s == "String") return Type::String;
	if(s == "DateTime") return Type::DateTime;
	if(s == "List") return Type::List;
	if(s == "Map") return Type::Map;
	if(s == "IMap") return Type::IMap;
	return Type::Invalid;
}

std::string ShvTypeDescr::sampleTypeToString(ShvTypeDescr::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: return "Discrete";
	case SampleType::Continuous: return "Continuous";
	case SampleType::Invalid: return "";
	}
	return std::string();
}

ShvTypeDescr::SampleType ShvTypeDescr::sampleTypeFromString(const std::string &s)
{
	if(s == "2" || s == "D" || s == "Discrete" || s == "discrete")
		return SampleType::Discrete;
	return SampleType::Continuous;
}

RpcValue ShvTypeDescr::defaultRpcValue() const
{
	using namespace chainpack;

	switch (type()) {
	case Type::Invalid: return RpcValue::fromType(RpcValue::Type::Invalid); break;
	case Type::BitField: return RpcValue::fromType(RpcValue::Type::UInt); break;
	case Type::Enum: return RpcValue::fromType(RpcValue::Type::Int); break;
	case Type::Bool: return RpcValue::fromType(RpcValue::Type::Bool); break;
	case Type::UInt: return RpcValue::fromType(RpcValue::Type::UInt); break;
	case Type::Int: return RpcValue::fromType(RpcValue::Type::Int); break;
	case Type::Decimal: return RpcValue::fromType(RpcValue::Type::Decimal); break;
	case Type::Double: return RpcValue::fromType(RpcValue::Type::Double); break;
	case Type::String: return RpcValue::fromType(RpcValue::Type::String); break;
	case Type::DateTime: return RpcValue::fromType(RpcValue::Type::DateTime); break;
	case Type::List: return RpcValue::fromType(RpcValue::Type::List); break;
	case Type::Map: return RpcValue::fromType(RpcValue::Type::Map); break;
	case Type::IMap: return RpcValue::fromType(RpcValue::Type::IMap); break;
	}

	return RpcValue::fromType(RpcValue::Type::Null);
}

int ShvTypeDescr::decimalPlaces() const
{
	return dataValue(KEY_DEC_PLACES).toInt();
}

ShvTypeDescr &ShvTypeDescr::setDecimalPlaces(int n)
{
	setDataValue(KEY_DEC_PLACES, n);
	return *this;
}

RpcValue ShvTypeDescr::toRpcValue() const
{
	RpcValue::Map map = m_data.asMap();

	map[KEY_TYPE_NAME] = map.value(KEY_NAME);
	map.setValue(KEY_NAME, {}); // erare obsolete
	map.setValue(KEY_TYPE, {}); // erase obsolete

	if(sampleType() == ShvTypeDescr::SampleType::Invalid || sampleType() == ShvTypeDescr::SampleType::Continuous)
		map.setValue(KEY_SAMPLE_TYPE, {});
	else
		map.setValue(KEY_SAMPLE_TYPE, sampleTypeToString(sampleType()));
	auto remove_empty_list = [&map](const std::string &name) {
		if(map.hasKey(name) && map.value(name).asList().empty())
			map.erase(name);
	};
	remove_empty_list(KEY_FIELDS);
	return map;
}

ShvTypeDescr ShvTypeDescr::fromRpcValue(const RpcValue &v)
{
	ShvTypeDescr ret;
	{
		RpcValue::Map map = v.asMap();
		RpcValue src_fields = map.take("fields");
		RpcList fields;
		for(const auto &rv : src_fields.asList()) {
			RpcValue::Map field = rv.asMap();
			mergeTags(field);
			fields.push_back(std::move(field));
		}
		mergeTags(map);
		map[KEY_FIELDS] = RpcValue(std::move(fields));
		ret.m_data = RpcValue(std::move(map));
	}
	{
		auto rv = ret.dataValue(KEY_TYPE_NAME);
		if(rv.isString())
			ret.setTypeName(rv.asString());
	}
	if(ret.type() == Type::Invalid) {
		// obsolete
		auto rv = ret.dataValue(KEY_NAME);
		if(rv.isString())
			ret.setTypeName(rv.asString());
	}
	if(ret.type() == Type::Invalid) {
		// obsolete
		auto rv = ret.dataValue(KEY_TYPE);
		if(rv.isString())
			ret.setTypeName(rv.asString());
	}
	{
		auto rv = ret.dataValue(KEY_SAMPLE_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_SAMPLE_TYPE, static_cast<int>(sampleTypeFromString(rv.asString())));
	}

	if (ret.isValid() && (ret.sampleType() == SampleType::Invalid))
		ret.setSampleType(SampleType::Continuous);

	return ret;
}

//=====================================================================
// ShvPropertyDescr
//=====================================================================
ShvPropertyDescr::ShvPropertyDescr() = default;

ShvPropertyDescr::ShvPropertyDescr(const std::string &name, const std::string &type_name) : Super(name, type_name)
{
}

std::vector<ShvMethodDescr> ShvPropertyDescr::methods() const
{
	std::vector<ShvMethodDescr> ret;
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		ret.push_back(ShvMethodDescr::fromRpcValue(m));
	}
	return ret;
}

ShvMethodDescr ShvPropertyDescr::method(const std::string &name) const
{
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		auto mm = ShvMethodDescr::fromRpcValue(m);
		if(mm.name() == name)
			return mm;
	}
	return {};
}

ShvPropertyDescr &ShvPropertyDescr::addMethod(const ShvMethodDescr &method_descr)
{
	RpcList new_methods = dataValue(KEY_METHODS).asList();
	new_methods.push_back(method_descr.toRpcValue());
	setDataValue(KEY_METHODS, new_methods);
	return *this;
}

ShvPropertyDescr &ShvPropertyDescr::setMethod(const ShvMethodDescr &method_descr)
{
	auto method_list = methods();
	bool method_found = false;
	for(auto &mm : method_list) {
		if(mm.name() == method_descr.name()) {
			mm = method_descr;
			method_found = true;
			break;
		}
	}
	if(!method_found) {
		method_list.push_back(method_descr);
	}
	RpcList new_methods;
	for(const auto &mm : method_list) {
		new_methods.push_back(mm.toRpcValue());
	}
	setDataValue(KEY_METHODS, new_methods);
	return *this;
}

RpcValue ShvPropertyDescr::toRpcValue() const
{
	auto rv = Super::toRpcValue();
	RpcList mml;
	for (const auto &m : methods()) {
		mml.push_back(m.toMap());
	}
	rv.set(KEY_METHODS, mml);
	return rv;
}

ShvPropertyDescr ShvPropertyDescr::fromRpcValue(const RpcValue &v, RpcValue::Map *extra_tags)
{
	static const std::vector<std::string> known_tags{
		KEY_DEVICE_TYPE,
		//KEY_SUPER_DEVICE_TYPE,
		KEY_NAME,
		KEY_TYPE_NAME,
		KEY_LABEL,
		KEY_DESCRIPTION,
		KEY_UNIT,
		KEY_METHODS,
		"autoload",
		"autorefresh",
		"monitored",
		"monitorOptions",
		KEY_SAMPLE_TYPE,
		KEY_ALARM,
		KEY_ALARM_LEVEL,
	};
	RpcValue::Map m = v.asMap();
	RpcValue::Map node_map;
	for(const auto &key : known_tags)
		if(auto it = m.find(key); it != m.cend()) {
			auto nh = m.extract(key);
			if(key != KEY_DEVICE_TYPE) {
				// deviceType might be imported from old typeinfo formats
				// but should not be part of property descr
				node_map.insert(std::move(nh));
			}
		}
	if(extra_tags)
		*extra_tags = std::move(m);
	ShvPropertyDescr ret;
	ret.setData(node_map);
	return ret;
}

//=====================================================================
// ShvDeviceDescription
//=====================================================================

ShvDeviceDescription::Properties::iterator ShvDeviceDescription::findProperty(const std::string &name)
{
	return std::find_if(properties.begin(), properties.end(), [&name](const auto &p) {
		return p.name() == name;
	});
}

ShvDeviceDescription::Properties::const_iterator ShvDeviceDescription::findProperty(const std::string &name) const
{
	return std::find_if(properties.begin(), properties.end(), [&name](const auto &p) {
		return p.name() == name;
	});
}

ShvDeviceDescription::Properties::const_iterator ShvDeviceDescription::findLongestPropertyPrefix(const std::string &name) const
{
	auto ret = properties.end();
	size_t max_len = 0;
	for(auto it = properties.begin(); it != properties.end(); ++it) {
		const auto property_name = it->name();
		if(shvpath::startsWithPath(name, property_name)) {
			// we need >= operator to support empty property name
			if(property_name.size() >= max_len) {
				max_len = property_name.size();
				ret = it;
			}
		}
	}
	return ret;
}

ShvDeviceDescription ShvDeviceDescription::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvDeviceDescription ret;
	const auto &map = v.asMap();
	//ret.deviceType = map.value(KEY_DEVICE_TYPE).asString();
	ret.restrictionOfDevice = map.value(KEY_RESTRICTION_OF_DEVICE).asString();
	ret.siteSpecificLocalization = map.value(KEY_SITE_SPECIFIC_LOCALIZATION).toBool();
	for(const auto &rv : map.valref("properties").asList()) {
		ret.properties.push_back(ShvPropertyDescr::fromRpcValue(rv));
	}
	return ret;
}

RpcValue ShvDeviceDescription::toRpcValue() const
{
	RpcValue::Map ret;
	//ret[KEY_DEVICE_TYPE] = deviceType;
	if(!restrictionOfDevice.empty())
		ret[KEY_RESTRICTION_OF_DEVICE] = restrictionOfDevice;
	if(siteSpecificLocalization)
		ret[KEY_SITE_SPECIFIC_LOCALIZATION] = siteSpecificLocalization;
	RpcList props;
	for(const auto &pd : properties) {
		props.push_back(pd.toRpcValue());
	}
	ret["properties"] = props;
	return ret;
}

ShvDeviceDescription &ShvDeviceDescription::setPropertyDescription(const ShvPropertyDescr &property_descr)
{
	auto property_name = property_descr.name();
	if(property_descr.isValid()) {
		if(auto it = findProperty(property_name); it != properties.end()) {
			*it = property_descr;
		}
		else {
			properties.push_back(property_descr);
		}
	}
	else {
		if(auto it = findProperty(property_name); it != properties.end()) {
			properties.erase(it);
		}
	}
	return *this;
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
template<typename T>
typename std::map<std::string, T>::const_iterator find_longest_prefix(const std::map<std::string, T> &map, const std::string &path)
{
	std::string prefix = path;
	while(true) {
		auto it = map.find(prefix);
		if(it == map.end()) {
			// if not found, cut last path part
			size_t ix = prefix.find_last_of('/');
			if(ix == std::string::npos) {
				if(prefix.empty()) {
					return map.cend();
				}
				prefix = "";
			}
			else {
				prefix = shv::core::utils::mid(prefix, 0, ix);
			}
		}
		else {
			return it;
		}
	}
	return map.cend();
}

bool ShvTypeInfo::isPathBlacklisted(const std::string &shv_path) const
{
	return std::any_of(m_blacklistedPaths.begin(), m_blacklistedPaths.end(), [&shv_path] (const auto& path) {
		return shv::core::utils::ShvPath::startsWithPath(shv_path, path.first);
	});
}

void ShvTypeInfo::setBlacklist(const std::string &shv_path, const chainpack::RpcValue &blacklist)
{
	if(blacklist.isList()) {
		for(const auto &path : blacklist.asList()) {
			m_blacklistedPaths[shv::core::utils::joinPath(shv_path, path.asString())];
		}
	}
	else if(blacklist.isMap()) {
		for(const auto &[path, val] : blacklist.asMap()) {
			m_blacklistedPaths[shv::core::utils::joinPath(shv_path, path)] = val;
		}
	}
}

void ShvTypeInfo::setPropertyDeviation(const std::string &shv_path, const ShvPropertyDescr &property_descr)
{
	m_propertyDeviations[shv_path] = property_descr;
}

ShvTypeInfo ShvTypeInfo::fromVersion2(std::map<std::string, ShvTypeDescr> &&types, const std::map<std::string, ShvPropertyDescr> &node_descriptions)
{
	ShvTypeInfo ret;
	ret.m_types = std::move(types);
	ret.m_devicePaths[""] = "";
	auto &device_descr = ret.m_deviceDescriptions[""];
	for(const auto &[k, v] : node_descriptions) {
		ShvPropertyDescr pd = v;
		pd.setName(k);
		device_descr.properties.push_back(std::move(pd));
	}
	return ret;
}

ShvTypeInfo ShvTypeInfo::fromVersion2(std::map<std::string, ShvTypeDescr> &&types, const std::vector<ShvPropertyDescr> &property_descriptions)
{
	ShvTypeInfo ret;
	ret.m_types = std::move(types);
	ret.m_devicePaths[""] = "";
	auto &device_descr = ret.m_deviceDescriptions[""];
	for(const auto &pd : property_descriptions) {
		device_descr.properties.push_back(pd);
	}
	return ret;
}

bool ShvTypeInfo::isValid() const
{
	return !(m_types.empty() && m_devicePaths.empty() && m_deviceDescriptions.empty());
}

const std::map<std::string, std::string>& ShvTypeInfo::devicePaths() const
{
	return m_devicePaths;
}

const std::map<std::string, ShvTypeDescr>& ShvTypeInfo::types() const
{
	return m_types;
}

const std::map<std::string, ShvDeviceDescription>& ShvTypeInfo::deviceDescriptions() const
{
	return m_deviceDescriptions;
}

const ShvDeviceDescription &ShvTypeInfo::deviceDescription(const std::string &device_type) const
{
	auto it = m_deviceDescriptions.find(device_type);
	if( it == m_deviceDescriptions.end()) {
		static ShvDeviceDescription empty;
		return empty;
	}

	return it->second;
}

const std::map<std::string, std::string>& ShvTypeInfo::systemPathsRoots() const
{
	return m_systemPathsRoots;
}

const std::map<std::string, shv::chainpack::RpcValue>& ShvTypeInfo::extraTags() const
{
	return m_extraTags;
}

ShvTypeInfo &ShvTypeInfo::setDevicePath(const std::string &device_path, const std::string &device_type)
{
	m_devicePaths[device_path] = device_type;
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setDeviceDescription(const std::string &device_type, const ShvDeviceDescription &device_descr)
{
	m_deviceDescriptions[device_type] = device_descr;
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setPropertyDescription(const std::string &device_type, const ShvPropertyDescr &property_descr)
{
	auto &dev_descr = m_deviceDescriptions[device_type];
	dev_descr.setPropertyDescription(property_descr);
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setExtraTags(const std::string &node_path, const chainpack::RpcValue &tags)
{
	if(tags.isValid())
		m_extraTags[node_path] = tags;
	else
		m_extraTags.erase(node_path);
	return *this;
}

RpcValue ShvTypeInfo::extraTags(const std::string &node_path) const
{
	auto it = m_extraTags.find(node_path);
	if( it == m_extraTags.cend()) {
		return {};
	}

	return it->second;
}

ShvTypeInfo &ShvTypeInfo::setTypeDescription(const std::string &type_name, const ShvTypeDescr &type_descr)
{
	if(type_descr.isValid())
		m_types[type_name] = type_descr;
	else
		m_types.erase(type_name);
	return *this;
}

namespace {
std::string cut_prefix(const std::string &path, const std::string &prefix) {
	if(prefix.empty()) {
		return path;
	}
	if(prefix.size() < path.size()) {
		return path.substr(prefix.size() + 1);
	}
	return std::string();
}
std::string cut_sufix(const std::string &path, const std::string &postfix) {
	if(postfix.empty()) {
		return path;
	}
	if(!postfix.empty() && postfix.size() < path.size()) {
		return path.substr(0, path.size() - postfix.size() - 1);
	}
	return std::string();
}
}

ShvTypeInfo::PathInfo ShvTypeInfo::pathInfo(const std::string &shv_path) const
{
	PathInfo ret;
	bool deviation_found = false;
	if(auto it3 = find_longest_prefix(m_propertyDeviations, shv_path); it3 != m_propertyDeviations.cend()) {
		deviation_found = true;
		const std::string own_property_path = it3->first;
		ret.fieldPath = cut_prefix(shv_path, own_property_path);
		ret.propertyDescription = it3->second;
	}
	const auto &[device_path, device_type, property_path] = findDeviceType(shv_path);
	ret.devicePath = device_path;
	ret.deviceType = device_type;
	if(deviation_found) {
		ret.propertyDescription.setName(cut_sufix(cut_prefix(shv_path, ret.devicePath), ret.fieldPath));
	}
	else {
		const auto &[property_descr, field_path] = findPropertyDescription(device_type, property_path);
		if(property_descr.isValid()) {
			ret.fieldPath = field_path;
			ret.propertyDescription = property_descr;
		}
	}
	return ret;
}

ShvPropertyDescr ShvTypeInfo::propertyDescriptionForPath(const std::string &shv_path, std::string *p_field_name) const
{
	auto info = pathInfo(shv_path);
	if(p_field_name)
		*p_field_name = info.fieldPath;
	return info.propertyDescription;
}

std::tuple<std::string, std::string, std::string> ShvTypeInfo::findDeviceType(const std::string &shv_path) const
{
	auto it = find_longest_prefix(m_devicePaths, shv_path);
	if(it == m_devicePaths.cend()) {
		return {};
	}

	const std::string prefix = it->first;
	const std::string device_type = it->second;
	const std::string property_path = prefix.empty()? shv_path: shv::core::utils::mid(shv_path, prefix.size() + 1);
	return make_tuple(prefix, device_type, property_path);
}

std::tuple<ShvPropertyDescr, std::string> ShvTypeInfo::findPropertyDescription(const std::string &device_type, const std::string &property_path) const
{
	if(auto it2 = m_deviceDescriptions.find(device_type); it2 != m_deviceDescriptions.end()) {
		const auto &dev_descr = it2->second;
		if(auto it3 = dev_descr.findLongestPropertyPrefix(property_path); it3 != dev_descr.properties.cend()) {
			const std::string own_property_path = it3->name();
			const std::string field_path = cut_prefix(property_path, own_property_path);
			return make_tuple(*it3, field_path);
		}
	}
	return {};
}

ShvTypeDescr ShvTypeInfo::findTypeDescription(const std::string &type_name) const
{
	auto it = m_types.find(type_name);
	if( it == m_types.end())
		return ShvTypeDescr(type_name);

	return it->second;
}

ShvTypeDescr ShvTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	return findTypeDescription(propertyDescriptionForPath(shv_path).typeName());
}

RpcValue ShvTypeInfo::extraTagsForPath(const std::string &shv_path) const
{
	auto it = m_extraTags.find(shv_path);
	if( it == m_extraTags.end())
		return {};

	return it->second;
}

std::string ShvTypeInfo::findSystemPath(const std::string &shv_path) const
{
	std::string current_root;
	std::string ret;
	for(const auto& [shv_root_path, system_path] : m_systemPathsRoots) {
		if(shv_path.starts_with(shv_root_path)) {
			if(shv_root_path.empty() || shv_path.size() == shv_root_path.size() || shv_path[shv_root_path.size()] == '/') {
				if(current_root.empty() || shv_root_path.size() > current_root.size()) {
					// fing longest match
					current_root = shv_root_path;
					ret = system_path;
				}
			}
		}
	}
	return ret;
}

RpcValue ShvTypeInfo::typesAsRpcValue() const
{
	RpcValue::Map m;
	for(const auto &kv : m_types) {
		m[kv.first] = kv.second.toRpcValue();
	}
	return m;
}

namespace {
constexpr auto VERSION = "version";
constexpr auto PATHS = "paths"; // SHV2
constexpr auto TYPES = "types"; // SHV2 + SHV3
constexpr auto DEVICE_PATHS = "devicePaths";
constexpr auto DEVICE_PROPERTIES = "deviceProperties";
constexpr auto DEVICE_DESCRIPTIONS = "deviceDescriptions";
constexpr auto PROPERTY_DEVIATIONS = "propertyDeviations";
constexpr auto EXTRA_TAGS = "extraTags";
constexpr auto SYSTEM_PATHS_ROOTS = "systemPathsRoots";
constexpr auto BLACKLISTED_PATHS = "blacklistedPaths";
}

RpcValue ShvTypeInfo::toRpcValue() const
{
	RpcValue::Map map;
	map[TYPES] = typesAsRpcValue();
	{
		RpcValue::Map m;
		for(const auto &kv : m_devicePaths) {
			m[kv.first] = kv.second;
		}
		map[DEVICE_PATHS] = std::move(m);
	}
	{
		RpcValue::Map m;
		for(const auto &[device_type, dev_descr] : m_deviceDescriptions) {
			m[device_type] = dev_descr.toRpcValue();
		}
		map[DEVICE_DESCRIPTIONS] = std::move(m);
	}
	map[EXTRA_TAGS] = m_extraTags;
	{
		RpcValue::Map m;
		for(const auto &[key, val] : m_systemPathsRoots) {
			m[key] = val;
		}
		map[SYSTEM_PATHS_ROOTS] = std::move(m);
	}
	map[BLACKLISTED_PATHS] = m_blacklistedPaths;
	if(!m_propertyDeviations.empty()) {
		RpcValue::Map m;
		for(const auto &[key, val] : m_propertyDeviations) {
			m[key] = val.toRpcValue();
		}
		map[PROPERTY_DEVIATIONS] = std::move(m);
	}
	RpcValue ret = map;
	ret.setMetaValue(VERSION, 4);
	return ret;
}

ShvTypeInfo ShvTypeInfo::fromRpcValue(const RpcValue &v)
{
	int version = v.metaValue(VERSION).toInt();
	const RpcValue::Map &map = v.asMap();
	ShvTypeInfo ret;
	if(version == 3 || version == 4) {
		{
			const RpcValue::Map &m = map.valref(TYPES).asMap();
			for(const auto &kv : m) {
				ret.m_types[kv.first] = ShvTypeDescr::fromRpcValue(kv.second);
			}
		}
		{
			const RpcValue::Map &m = map.valref(DEVICE_PATHS).asMap();
			for(const auto &kv : m) {
				ret.m_devicePaths[kv.first] = kv.second.asString();
			}
		}
		if(version == 3) {
			const RpcValue::Map &m = map.valref(DEVICE_PROPERTIES).asMap();
			for(const auto &[device_type, prop_map] : m) {
				const RpcValue::Map &pm = prop_map.asMap();
				auto &dev_descr = ret.m_deviceDescriptions[device_type];
				for(const auto &[property_path, property_descr_rv] : pm) {
					auto property_descr = ShvPropertyDescr::fromRpcValue(property_descr_rv);
					property_descr.setName(property_path);
					dev_descr.properties.push_back(std::move(property_descr));
				}
			}
		}
		else {
			const RpcValue::Map &m = map.valref(DEVICE_DESCRIPTIONS).asMap();
			for(const auto &[device_type, rv] : m) {
				auto &dev_descr = ret.m_deviceDescriptions[device_type];
				dev_descr = ShvDeviceDescription::fromRpcValue(rv);
			}
		}
		ret.m_extraTags = map.value(EXTRA_TAGS).asMap();
		{
			const RpcValue::Map &m = map.valref(SYSTEM_PATHS_ROOTS).asMap();
			for(const auto &[key, val] : m) {
				ret.m_systemPathsRoots[key] = val.asString();
			}
		}
		ret.m_blacklistedPaths = map.value(BLACKLISTED_PATHS).asMap();
		{
			const RpcValue::Map &m = map.valref(PROPERTY_DEVIATIONS).asMap();
			for(const auto &[shv_path, node_descr] : m) {
				ret.m_propertyDeviations[shv_path] = ShvPropertyDescr::fromRpcValue(node_descr);
			}
		}
	}
	else if(map.hasKey(PATHS) && map.hasKey(TYPES)) {
		// version 2
		{
			const RpcValue::Map &m = map.valref(TYPES).asMap();
			for(const auto &kv : m) {
				ret.m_types[kv.first] = ShvTypeDescr::fromRpcValue(kv.second);
			}
		}
		{
			auto &dev_descr = ret.m_deviceDescriptions[""];
			const RpcValue::Map &m = map.valref(PATHS).asMap();
			for(const auto &[path, val] : m) {
				auto nd = ShvPropertyDescr::fromRpcValue(val);
				nd.setTypeName(val.asMap().value("type").asString());
				nd.setName(path);
				dev_descr.properties.push_back(std::move(nd));
			}
		}
	}
	else {
		ret = fromNodesTree(v);
	}
	if(ret.m_systemPathsRoots.empty()) {
		ret.m_systemPathsRoots[""] = "system";
	}
	return ret;
}

RpcValue ShvTypeInfo::applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums, bool recursive_bitfields) const
{
	ShvTypeDescr td = findTypeDescription(type_name);
	return applyTypeDescription(val, td, translate_enums, recursive_bitfields);
}

RpcValue ShvTypeInfo::applyTypeDescription(const chainpack::RpcValue &val, const ShvTypeDescr &type_descr, bool translate_enums, bool recursive_bitfields) const
{
	switch(type_descr.type()) {
	case ShvTypeDescr::Type::Invalid:
		return val;
	case ShvTypeDescr::Type::BitField: {
		if (recursive_bitfields) {
		RpcValue::Map map;
		for(const ShvFieldDescr &fld : type_descr.fields()) {
			RpcValue result = fld.bitfieldValue(val.toUInt64());
			result = applyTypeDescription(result, fld.typeName(), translate_enums);
			map[fld.name()] = result;
		}
		return map;
	}
		return val;
	}
	case ShvTypeDescr::Type::Enum: {
		int ival = val.toInt();
		if(translate_enums) {
			for(const ShvFieldDescr &fld : type_descr.fields()) {
				if(fld.value().toInt() == ival)
					return fld.name();
			}
			return "UNKNOWN_" + val.toCpon();
		}

		return ival;
	}
	case ShvTypeDescr::Type::Bool:
		return val.toBool();
	case ShvTypeDescr::Type::UInt:
		return val.toUInt();
	case ShvTypeDescr::Type::Int:
		return val.toInt();
	case ShvTypeDescr::Type::Decimal:
		if(val.isDecimal())
			return val;
		return RpcValue::Decimal::fromDouble(val.toDouble(), type_descr.decimalPlaces());
	case ShvTypeDescr::Type::Double:
		return val.toDouble();
	case ShvTypeDescr::Type::String:
		if(val.isString())
			return val;
		return val.asString();
	case ShvTypeDescr::Type::DateTime:
		return val.toDateTime();
	case ShvTypeDescr::Type::List:
		if(val.isList())
			return val;
		return val.asList();
	case ShvTypeDescr::Type::Map:
		if(val.isMap()) {
			RpcValue::Map map;
			for (const auto &v: val.asMap()) {
				map[v.first] = applyTypeDescription(v.second, type_descr.field(v.first).typeName(), translate_enums, recursive_bitfields);
			}
			return map;
		}
		return val.asMap();
	case ShvTypeDescr::Type::IMap:
		if(val.isIMap())
			return val;
		return val.asIMap();
	}
	shvError() << "Invalid type:" << static_cast<int>(type_descr.type());
	return val;
}

void ShvTypeInfo::forEachDeviceProperty(const std::string &device_type, const std::function<void (const ShvPropertyDescr &)>& fn) const
{
	auto it = m_deviceDescriptions.find(device_type);
	if( it == m_deviceDescriptions.end()) {
		// device path invalid
		return;
	}

	for(const auto &property_descr : it->second.properties) {
		fn(property_descr);
	};
}

void ShvTypeInfo::forEachProperty(const std::function<void (const std::string &shv_path, const ShvPropertyDescr &)>& fn) const
{
	for(const auto& [device_path, device_type] : m_devicePaths) {
		if(auto it = m_deviceDescriptions.find(device_type); it != m_deviceDescriptions.end()) {
			for(const auto &property_descr : it->second.properties) {
				if (!property_descr.name().empty()) {
					const auto shv_path = shv::core::utils::joinPath(device_path, property_descr.name());
					fn(shv_path, property_descr);
				}
			}
		}
	}
}

void ShvTypeInfo::fromNodesTree_helper(const RpcValue::Map &node_types,
										const shv::chainpack::RpcValue &node,
										const std::string &device_type,
										const std::string &device_path,
										const std::string &property_path,
										ShvDeviceDescription *device_description)
{
	using namespace shv::core::utils;
	using namespace shv::core;
	if(!node.isValid())
		return;

	if(auto ref = node.metaValue("nodeTypeRef").asString(); !ref.empty()) {
		fromNodesTree_helper(node_types, node_types.value(ref), device_type, device_path, property_path, device_description);
		return;
	}

	std::string current_device_type = device_type;
	std::string current_device_path = device_path;
	std::string current_property_path = property_path;
	ShvDeviceDescription new_device_description;
	ShvDeviceDescription *current_device_description = device_description? device_description: &new_device_description;
	RpcValue::Map property_descr_map;
	RpcList property_methods;
	bool new_device_type_entered = device_description == nullptr;
	static const std::string CREATE_FROM_TYPE_NAME = "createFromTypeName";
	static const std::string SYSTEM_PATH = "systemPath";
	for(const RpcValue &rv : node.metaData().valref("methods").asList()) {
		const auto mm = MetaMethod::fromRpcValue(rv);
		const std::string &method_name = mm.name();
		if(method_name == Rpc::METH_LS || method_name == Rpc::METH_DIR)
			continue;
		property_methods.push_back(mm.toRpcValue());
		if(!mm.extra().empty()) {
			std::string key = shv::core::utils::joinPath(property_path, std::string("method"));
			key = shv::core::utils::joinPath(key, mm.name());
			setExtraTags(key, mm.extra());
		}
	}
	const RpcValue::Map &node_tags = node.metaData().valref(KEY_TAGS).asMap();
	if(!node_tags.empty()) {
		RpcValue::Map tags_map = node_tags;
		const std::string &dtype = tags_map.valref(KEY_DEVICE_TYPE).asString();
		if(!dtype.empty()) {
			current_device_type = dtype;
			current_device_path = utils::joinPath(device_path, property_path);
			current_property_path = std::string();
			current_device_description = &new_device_description;
			new_device_type_entered = true;
		}
		property_descr_map.merge(tags_map);
	}
	property_descr_map.erase(CREATE_FROM_TYPE_NAME); // erase shvgate obsolete tag
	if(!property_methods.empty())
		property_descr_map[KEY_METHODS] = property_methods;
	shv::core::utils::ShvPropertyDescr property_descr;
	if(!property_descr_map.empty()) {
		RpcValue::Map extra_tags;
		property_descr = shv::core::utils::ShvPropertyDescr::fromRpcValue(property_descr_map, &extra_tags);
		if(!property_descr.isEmpty()) {
			property_descr.setName(current_property_path);
			current_device_description->properties.push_back(property_descr);
		}
		auto system_path = extra_tags.take(SYSTEM_PATH).asString();
		if(!system_path.empty()) {
			auto shv_path = utils::joinPath(current_device_path, current_property_path);
			m_systemPathsRoots[shv_path] = system_path;
		}
		const auto blacklist = extra_tags.take(KEY_BLACKLIST);
		if(blacklist.isValid()) {
			auto shv_path = utils::joinPath(current_device_path, current_property_path);
			setBlacklist(shv_path, blacklist);
		}
		if(!extra_tags.empty()) {
			auto shv_path = utils::joinPath(current_device_path, current_property_path);
			setExtraTags(shv_path, extra_tags);
		}
	}
	if(property_descr.typeName().empty() && !node_tags.hasKey(CREATE_FROM_TYPE_NAME)) {
		// property with type-name defined cannot have child properties
		for (const auto& [child_name, child_node] : node.asMap()) {
			if(child_name.empty())
				continue;
			ShvPath child_property_path = shv::core::utils::joinPath(current_property_path, child_name);
			fromNodesTree_helper(node_types, child_node, current_device_type, current_device_path, child_property_path.asString(), current_device_description);
		}
	}
	if(new_device_type_entered) {
		setDevicePath(current_device_path, current_device_type);
		if(auto it = m_deviceDescriptions.find(current_device_type); it == m_deviceDescriptions.end()) {
			// device type defined first time
			m_deviceDescriptions[current_device_type] = std::move(new_device_description);
		}
		else {
			// check deviations
			const auto &existing_device_descr = it->second;
			for(const auto &existing_property_descr : existing_device_descr.properties) {
				if(auto it2 = new_device_description.findProperty(existing_property_descr.name()); it2 == new_device_description.properties.end()) {
					// property defined only in previous definition, add it as empty to deviations
					auto shv_path = utils::joinPath(current_device_path, existing_property_descr.name());
					//shvInfo() << "set property deviation to {}, path:" << shv_path;
					setPropertyDeviation(shv_path, {});
				}
				else {
					// defined in both definitions, compare them
					const auto &new_property_descr = *it2;
					if(!(new_property_descr == existing_property_descr)) {
						// they are not the same, create deviation
						auto shv_path = utils::joinPath(current_device_path, existing_property_descr.name());
						setPropertyDeviation(shv_path, new_property_descr);
					}
					new_device_description.properties.erase(it2);
				}
			}
			// check remaining property paths in new definition
			for(const auto &new_property_descr : new_device_description.properties) {
				auto shv_path = utils::joinPath(current_device_path, new_property_descr.name());
				setPropertyDeviation(shv_path, new_property_descr);
			}
		}
	}
}

ShvTypeInfo ShvTypeInfo::fromNodesTree(const chainpack::RpcValue &v)
{
	ShvTypeInfo ret;
	{
		const RpcValue types = v.metaValue("typeInfo").asMap().value("types");
		const RpcValue::Map &m = types.asMap();
		for(const auto &kv : m) {
			auto td = ShvTypeDescr::fromRpcValue(kv.second);
			if(td.typeName().empty())
				td.setTypeName(kv.first);
			ret.m_types[kv.first] = td;
		}
	}
	const RpcValue node_types = v.metaValue("nodeTypes");
	ret.fromNodesTree_helper(node_types.asMap(), v, {}, {}, {}, nullptr);
	return ret;
}

} // namespace shv
