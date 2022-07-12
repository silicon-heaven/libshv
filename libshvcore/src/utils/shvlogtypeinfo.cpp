#include "shvlogtypeinfo.h"
#include "shvpath.h"
#include "../string.h"
#include "../utils.h"
#include "../log.h"

#include<shv/chainpack/metamethod.h>

#include <cassert>

using namespace std;
using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

static const char* KEY_LABEL = "label";
static const char* KEY_DESCRIPTION = "description";
static const char* KEY_UNIT = "unit";
static const char* KEY_NAME = "name";
static const char* KEY_TYPE_NAME = "typeName";
static const char* KEY_TYPE = "type";
static const char* KEY_VALUE = "value";
static const char* KEY_FIELDS = "fields";
static const char* KEY_SAMPLE_TYPE = "sampleType";
static const char* KEY_TAGS = "tags";
static const char* KEY_METHODS = "methods";
//static const char* KEY_MIN_VAL = "minVal";
//static const char* KEY_MAX_VAL = "maxVal";
static const char* KEY_DEC_PLACES = "decPlaces";
static const char* KEY_VISUAL_STYLE = "visualStyle";
static const char* KEY_ALARM = "alarm";
static const char* KEY_ALARM_LEVEL = "alarmLevel";

//=====================================================================
// ShvLogDescrBase
//=====================================================================
RpcValue ShvLogDescrBase::dataValue(const std::string &key, const chainpack::RpcValue &default_val) const
{
	return m_data.asMap().value(key, default_val);
}

void ShvLogDescrBase::setDataValue(const std::string &key, const chainpack::RpcValue &val)
{
	if(!m_data.isMap()) {
		m_data = RpcValue::Map();
	}
	m_data.set(key, val);
}

void ShvLogDescrBase::setData(const chainpack::RpcValue &data)
{
	if(data.isMap()) {
		RpcValue::Map map = data.asMap();
		mergeTags(map);
		m_data = map;
	}
}

void ShvLogDescrBase::mergeTags(chainpack::RpcValue::Map &map)
{
	auto nh = map.extract(KEY_TAGS);
	if(!nh.empty()) {
		RpcValue::Map tags = nh.mapped().asMap();
		map.merge(tags);
	}
}
//=====================================================================
// ShvLogTypeDescrField
//=====================================================================
ShvLogFieldDescr::ShvLogFieldDescr(const std::string &name, const std::string &type_name, const chainpack::RpcValue &value)
{
	setDataValue(KEY_NAME, name);
	setDataValue(KEY_TYPE_NAME, type_name);
	setDataValue(KEY_VALUE, value);
}

string ShvLogFieldDescr::name() const
{
	return dataValue(KEY_NAME).asString();
}

string ShvLogFieldDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

string ShvLogFieldDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

string ShvLogFieldDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

RpcValue ShvLogFieldDescr::value() const
{
	return dataValue(KEY_VALUE);
}

string ShvLogFieldDescr::alarm() const
{
	return dataValue(KEY_ALARM).asString();
}

int ShvLogFieldDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

RpcValue ShvLogFieldDescr::toRpcValue() const
{
	RpcValue ret = m_data;
	if(description().empty())
		ret.set(KEY_DESCRIPTION, {});
	return ret;
}

ShvLogFieldDescr ShvLogFieldDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogFieldDescr ret;
	ret.setData(v);
	return ret;
}

std::pair<unsigned, unsigned> ShvLogFieldDescr::bitRange() const
{
	unsigned bit_no1 = 0;
	unsigned bit_no2 = 0;
	if(value().isList()) {
		const auto &lst = value().asList();
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

RpcValue ShvLogFieldDescr::bitfieldValue(uint64_t uval) const
{
	auto [bit_no1, bit_no2] = bitRange();
	uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 + 1));
	shvDebug() << "bits:" << bit_no1 << bit_no2 << (bit_no1 == bit_no2) << "uval:" << uval << "mask:" << mask;
	uval = (uval & mask) >> bit_no1;
	shvDebug() << "uval masked and rotated right by:" << bit_no1 << "bits, uval:" << uval;
	RpcValue result;
	if(bit_no1 == bit_no2)
		result = static_cast<bool>(uval != 0);
	else
		result = uval;
	return result;
}

uint64_t ShvLogFieldDescr::setBitfieldValue(uint64_t bitfield, uint64_t uval) const
{
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
// ShvLogTypeDescr
//=====================================================================
ShvLogTypeDescr::Type ShvLogTypeDescr::type() const
{
	return static_cast<Type>(dataValue(KEY_TYPE).toInt());
}

ShvLogTypeDescr &ShvLogTypeDescr::setType(Type t)
{
	setDataValue(KEY_TYPE, (int)t);
	return *this;
}

std::vector<ShvLogFieldDescr> ShvLogTypeDescr::fields() const
{
	std::vector<ShvLogFieldDescr> ret;
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList())
		ret.push_back(ShvLogFieldDescr::fromRpcValue(rv));
	return ret;
}

ShvLogTypeDescr &ShvLogTypeDescr::setFields(const std::vector<ShvLogFieldDescr> &fields)
{
	RpcValue::List cp_fields;
	for (const ShvLogFieldDescr &field : fields) {
		cp_fields.push_back(field.toRpcValue());
	}
	setDataValue(KEY_FIELDS, cp_fields);
	return *this;
}

ShvLogTypeDescr::SampleType ShvLogTypeDescr::sampleType() const
{
	return static_cast<SampleType>(dataValue(KEY_SAMPLE_TYPE).toInt());
}

ShvLogTypeDescr &ShvLogTypeDescr::setSampleType(ShvLogTypeDescr::SampleType st)
{
	setDataValue(KEY_SAMPLE_TYPE, (int)st);
	return *this;
}

ShvLogFieldDescr ShvLogTypeDescr::field(const std::string &field_name) const
{
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList()) {
		auto fld_descr = ShvLogFieldDescr::fromRpcValue(rv);
		if(fld_descr.name() == field_name) {
			return fld_descr;
		}
	}
	return {};
}

RpcValue ShvLogTypeDescr::fieldValue(const chainpack::RpcValue &val, const std::string &field_name) const
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

const std::string ShvLogTypeDescr::typeToString(ShvLogTypeDescr::Type t)
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
	return "Invalid";
}

ShvLogTypeDescr::Type ShvLogTypeDescr::typeFromString(const std::string &s)
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

const std::string ShvLogTypeDescr::sampleTypeToString(ShvLogTypeDescr::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: return "Discrete";
	case SampleType::Continuous: return "Continuous";
	case SampleType::Invalid: return "Invalid";
	}
	return std::string();
}

ShvLogTypeDescr::SampleType ShvLogTypeDescr::sampleTypeFromString(const std::string &s)
{
	if(s == "2" || s == "D" || s == "Discrete" || s == "discrete")
		return SampleType::Discrete;
	if(s.empty() || s == "C" || s == "Continuous" || s == "continuous")
		return SampleType::Continuous;
	return SampleType::Invalid;
}

string ShvLogTypeDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

ShvLogTypeDescr &ShvLogTypeDescr::setUnit(const std::string &unit)
{
	setDataValue(KEY_UNIT, unit);
	return *this;
}
/*
void ShvLogTypeDescr::applyTags(const RpcValue::Map &t)
{
	if (type == Type::Invalid)
		type = typeFromString(t.value("typeName").toStdString());

	if (t.hasKey("sampleType"))
		sampleType = sampleTypeFromString(t.value("sampleType").asString());

	//if (t.hasKey(description))
	//	description = t.value(KEY_DESCRIPTION).toStdString();

	tags.insert(t.begin(), t.end());
}
*/

RpcValue ShvLogTypeDescr::defaultRpcValue() const
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

std::string ShvLogTypeDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE).asString();
}

std::string ShvLogTypeDescr::alarm() const
{
	return dataValue(KEY_ALARM).asString();
}

ShvLogTypeDescr &ShvLogTypeDescr::setAlarm(const std::string &alarm)
{
	setDataValue(KEY_ALARM, alarm);
	return *this;
}

int ShvLogTypeDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

string ShvLogTypeDescr::alarmDescription() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

int ShvLogTypeDescr::decimalPlaces() const
{
	return dataValue(KEY_DEC_PLACES).toInt();
}

ShvLogTypeDescr &ShvLogTypeDescr::setDecimalPlaces(int n)
{
	setDataValue(KEY_DEC_PLACES, n);
	return *this;
}

RpcValue ShvLogTypeDescr::toRpcValue() const
{
	RpcValue::Map map = m_data.asMap();
	map.setValue(KEY_TYPE, typeToString(type()));
	if(sampleType() != ShvLogTypeDescr::SampleType::Invalid)
		map.setValue(KEY_SAMPLE_TYPE, sampleTypeToString(sampleType()));
	else
		map.setValue(KEY_SAMPLE_TYPE, {});
	return map;
}

ShvLogTypeDescr ShvLogTypeDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeDescr ret;
	{
		RpcValue::Map map = v.asMap();
		RpcValue src_fields = map.take("fields");
		RpcValue::List fields;
		for(const auto &rv : src_fields.asList()) {
			RpcValue::Map field = rv.asMap();
			mergeTags(field);
			fields.push_back(move(field));
		}
		mergeTags(map);
		map[KEY_FIELDS] = RpcValue(move(fields));
		ret.m_data = RpcValue(move(map));
	}
	{
		auto rv = ret.dataValue(KEY_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_TYPE, (int)typeFromString(rv.asString()));
	}
	{
		auto rv = ret.dataValue(KEY_SAMPLE_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_SAMPLE_TYPE, (int)sampleTypeFromString(rv.asString()));
	}
	return ret;
}

//=====================================================================
// ShvLogNodeDescr
//=====================================================================
string ShvLogNodeDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

string ShvLogNodeDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

string ShvLogNodeDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

string ShvLogNodeDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

std::vector<ShvLogMethodDescr> ShvLogNodeDescr::methods() const
{
	std::vector<ShvLogMethodDescr> ret;
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		ret.push_back(ShvLogMethodDescr::fromRpcValue(m));
	}
	return ret;
}

ShvLogMethodDescr ShvLogNodeDescr::method(const std::string &name) const
{
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		auto mm = ShvLogMethodDescr::fromRpcValue(m);
		if(mm.name() == name)
			return mm;
	}
	return {};
}

/*
string ShvLogNodeDescr::alarm() const
{
	return dataValue(KEY_ALARM, std::string()).asString();
}

int ShvLogNodeDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

RpcValue ShvLogNodeDescr::tags() const
{
	return dataValue(KEY_TAGS);
}
ShvLogNodeDescr ShvLogNodeDescr::fromTags(const RpcValue::Map &tags)
{
	ShvLogNodeDescr ret;
	ret.m_data = tags;
	//ret.typeName = ret.tags.take(KEY_TYPE_NAME).asString();
	//ret.description = ret.tags.take(KEY_DESCRIPTION).asString();
	return ret;
}
*/
RpcValue ShvLogNodeDescr::toRpcValue() const
{
	//RpcValue::Map m;
	//m[KEY_TYPE_NAME] = typeName;
	//if(!description.empty())
	//	m[KEY_DESCRIPTION] = description;
	//if(!tags.empty())
	//	m[KEY_TAGS] = tags;
	return m_data;
}

ShvLogNodeDescr ShvLogNodeDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogNodeDescr ret;
	ret.setData(v);
	return ret;
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
map<string, string>::const_iterator find_longest_prefix(const map<string, string> &map, const string &path)
{
	shv::core::String prefix = path;
	while(true) {
		auto it = map.find(prefix);
		if(it == map.end()) {
			// if not found, cut last path part
			size_t ix = prefix.lastIndexOf('/');
			if(ix == string::npos) {
				return map.cend();
			}
			else {
				prefix = prefix.mid(0, ix);
			}
		}
		else {
			return it;
		}
	}
	return map.cend();
}

ShvLogTypeInfo &ShvLogTypeInfo::setDevicePath(const std::string &device_path, const std::string &device_type)
{
	m_devicePaths[device_path] = device_type;
	return *this;
}

ShvLogTypeInfo &ShvLogTypeInfo::setNodeDescription(const ShvLogNodeDescr &node_descr, const std::string &node_path, const std::string &device_type)
{
	string path = node_path;
	if(!device_type.empty()) {
		path = path.empty()? device_type: device_type + '/' + path;
	}
	m_nodeDescriptions[path] = node_descr;
	return *this;
}

ShvLogTypeInfo &ShvLogTypeInfo::setTypeDescription(const ShvLogTypeDescr &type_descr, const std::string &type_name)
{
	if(type_descr.isValid())
		m_types[type_name] = type_descr;
	else
		m_types.erase(type_name);
	return *this;
}

ShvLogNodeDescr ShvLogTypeInfo::nodeDescriptionForDevice(const std::string &device_type, const string &property_path) const
{
	string property = property_path.empty()? device_type: device_type + '/' + property_path;
	if(auto it_node_descr = m_nodeDescriptions.find(property); it_node_descr == m_nodeDescriptions.cend()) {
		return {};
	}
	else {
		return it_node_descr->second;
	}
}

ShvLogNodeDescr ShvLogTypeInfo::nodeDescriptionForPath(const std::string &shv_path) const
{
	if (auto it_node_descr = m_nodeDescriptions.find(shv_path); it_node_descr == m_nodeDescriptions.cend()) {
		if(auto it = find_longest_prefix(m_devicePaths, shv_path); it == m_devicePaths.cend()) {
			return {};
		}
		else {
			string prefix = it->first;
			string device_type = it->second;
			string property_path = String(shv_path).mid(prefix.size() + 1);
			return nodeDescriptionForDevice(device_type, property_path);
		}
	}
	else {
		return it_node_descr->second;
	}
}
/*
ShvLogTypeDescr ShvLogTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	ShvLogNodeDescr node_descr = nodeDescriptionForPath(shv_path);
	return typeDescriptionForName(node_descr.typeName());
}
*/
ShvLogTypeDescr ShvLogTypeInfo::typeDescriptionForName(const std::string &type_name) const
{
	if(auto it = m_types.find(type_name); it == m_types.end())
		return {};
	else
		return it->second;
}

ShvLogTypeDescr ShvLogTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	return typeDescriptionForName(nodeDescriptionForPath(shv_path).typeName());
}

std::string ShvLogTypeInfo::findSystemPath(const std::string &shv_path) const
{
	string current_root;
	string ret;
	for(const auto& [shv_root_path, system_path] : m_systemPathsRoots) {
		if(String::startsWith(shv_path, shv_root_path)) {
			if(shv_path.size() == shv_root_path.size() || shv_path[shv_root_path.size()] == '/') {
				if(shv_root_path.size() > current_root.size()) {
					// fing longest match
					current_root = shv_root_path;
					ret = system_path;
				}
			}
		}
	}
	return ret;
}

RpcValue ShvLogTypeInfo::typesAsRpcValue() const
{
	RpcValue::Map m;
	for(const auto &kv : m_types) {
		m[kv.first] = kv.second.toRpcValue();
	}
	return m;
}

RpcValue ShvLogTypeInfo::toRpcValue() const
{
	RpcValue::Map map;
	map["types"] = typesAsRpcValue();
	{
		RpcValue::Map m;
		for(const auto &kv : m_devicePaths) {
			m[kv.first] = kv.second;
		}
		map["devices"] = std::move(m);
	}
	{
		RpcValue::Map m;
		for(const auto &kv : m_nodeDescriptions) {
			m[kv.first] = kv.second.toRpcValue();
		}
		map["nodes"] = std::move(m);
	}
	RpcValue ret = map;
	ret.setMetaValue("version", 3);
	return ret;
}

ShvLogTypeInfo ShvLogTypeInfo::fromRpcValue(const RpcValue &v)
{
	int version = v.metaValue("version").toInt();
	if(version != 3) {
		//shvError() << "Type info version:" << version << "is not supported.";
		return fromNodesTree(v);
	}
	ShvLogTypeInfo ret;
	const RpcValue::Map &map = v.asMap();
	{
		const RpcValue::Map &m = map.value("types").asMap();
		for(const auto &kv : m) {
			ret.m_types[kv.first] = ShvLogTypeDescr::fromRpcValue(kv.second);
		}
	}
	{
		const RpcValue::Map &m = map.value("devices").asMap();
		for(const auto &kv : m) {
			ret.m_devicePaths[kv.first] = kv.second.asString();
		}
	}
	{
		const RpcValue::Map &m = map.value("nodes").asMap();
		for(const auto &kv : m) {
			ret.m_nodeDescriptions[kv.first] = ShvLogNodeDescr::fromRpcValue(kv.second);
		}
	}
	return ret;
}

RpcValue ShvLogTypeInfo::applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums) const
{
	ShvLogTypeDescr td = typeDescriptionForName(type_name);
	//shvWarning() << type_name << "--->" << td.toRpcValue().toCpon();
	switch(td.type()) {
	case ShvLogTypeDescr::Type::Invalid:
		return val;
	case ShvLogTypeDescr::Type::BitField: {
		RpcValue::Map map;
		for(const ShvLogFieldDescr &fld : td.fields()) {
			RpcValue result = fld.bitfieldValue(val.toUInt64());
			result = applyTypeDescription(result, fld.typeName());
			map[fld.name()] = result;
		}
		return map;
	}
	case ShvLogTypeDescr::Type::Enum: {
		int ival = val.toInt();
		if(translate_enums) {
			for(const ShvLogFieldDescr &fld : td.fields()) {
				if(fld.value().toInt() == ival)
					return fld.name();
			}
			return "UNKNOWN_" + val.toCpon();
		}
		else {
			return ival;
		}
	}
	case ShvLogTypeDescr::Type::Bool:
		return val.toBool();
	case ShvLogTypeDescr::Type::UInt:
		return val.toUInt();
	case ShvLogTypeDescr::Type::Int:
		return val.toInt();
	case ShvLogTypeDescr::Type::Decimal:
		if(val.isDecimal())
			return val;
		return RpcValue::Decimal::fromDouble(val.toDouble(), td.decimalPlaces());
	case ShvLogTypeDescr::Type::Double:
		return val.toDouble();
	case ShvLogTypeDescr::Type::String:
		if(val.isString())
			return val;
		return val.asString();
	case ShvLogTypeDescr::Type::DateTime:
		return val.toDateTime();
	case ShvLogTypeDescr::Type::List:
		if(val.isList())
			return val;
		return val.asList();
	case ShvLogTypeDescr::Type::Map:
		if(val.isMap())
			return val;
		return val.asMap();
	case ShvLogTypeDescr::Type::IMap:
		if(val.isIMap())
			return val;
		return val.asIMap();
	}
	shvError() << "Invalid type:" << (int)td.type();
	return val;
}

void ShvLogTypeInfo::forEachNodeDescription(const std::string &node_descr_root, std::function<void (const std::string &, const ShvLogNodeDescr &)> fn) const
{
	ShvPath::forEachDirAndSubdirs(nodeDescriptions(), node_descr_root, [=](auto it) {
		string property_path = it->first;
		assert(property_path.size() >= node_descr_root.size());
		property_path = property_path.substr(node_descr_root.size());
		if(property_path.size() > 0 && property_path[0] == '/')
			property_path = property_path.substr(1);
		const ShvLogNodeDescr &node_descr = it->second;
		fn(property_path, node_descr);
	});
}

void ShvLogTypeInfo::forEachNode(std::function<void (const std::string &shv_path, const ShvLogNodeDescr &node_descr)> fn) const
{
	map<string, vector<string>> device_id_to_path;
	for(const auto& [path, device_id] : m_devicePaths)
		device_id_to_path[device_id].push_back(path);
	for(const auto& [path, node_descr] : m_nodeDescriptions) {
		if(auto ix = path.find('/'); ix != string::npos) {
			string device_id = path.substr(0, ix);
			if(auto it = device_id_to_path.find(device_id); it != device_id_to_path.end()) {
				// device paths
				for(const auto &device_path : it->second) {
					string shv_path = shv::core::Utils::joinPath(device_path, path);
					fn(shv_path, node_descr);
				}
				continue;
			}
		}
		// not device paths
		fn(path, node_descr);
	}
}

static void fromNodesTree_helper(shv::core::utils::ShvLogTypeInfo &type_info,
								const RpcValue &node,
								const std::string &_shv_path,
								const std::string &recent_device_type,
								const std::string &recent_device_path,
								const RpcValue::Map &node_types)
{
	using namespace shv::core::utils;
	using namespace shv::core;
	if(!node.isValid())
		return;

	string device_type = recent_device_type;
	string device_path = recent_device_path;
	string type_name;
	RpcValue::Map property_descr;
	RpcValue::List property_methods;
	String shv_path = _shv_path;
	if(shv_path.indexOf("/status/") != string::npos) {
		// TODO: find better way how to exclude subtrees created from type description
		return;
	}
	if(auto ref = node.metaValue("nodeTypeRef").asString(); !ref.empty()) {
		fromNodesTree_helper(type_info, node_types.value(ref), shv_path, recent_device_type, recent_device_path, node_types);
		return;
	}
	//shvInfo() << "id:" << node->nodeId() << "node path:" << node_shv_path << "shv path:" << shv_path;
	//StringViewList node_shv_dir_list = ShvPath::splitPath(node_shv_path);
	for(const RpcValue &rv : node.metaValue("methods").asList()) {
		const auto mm = MetaMethod::fromRpcValue(rv);
		const string &method_name = mm.name();
		if(method_name == Rpc::METH_LS || method_name == Rpc::METH_DIR)
			continue;
		property_methods.push_back(mm.toRpcValue());
	}
	if(auto tags = node.metaValue("tags"); tags.isMap()) {
		RpcValue::Map tags_map = tags.asMap();
		const string &dtype = tags_map.value("deviceType").asString();
		if(!dtype.empty()) {
			device_type = dtype;
			device_path = shv_path;
		}
		property_descr.merge(tags_map);
	}
	if(!property_methods.empty())
		property_descr["methods"] = property_methods;
	if(!property_descr.empty()) {
		auto pd = shv::core::utils::ShvLogNodeDescr::fromRpcValue(property_descr);
		string node_path = device_path.empty()? shv_path: shv_path.mid(device_path.size() + 1);
		type_info.setNodeDescription(pd, node_path, device_type);
	}
	if(device_type != recent_device_type)
		type_info.setDevicePath(device_path, device_type);

	for (const auto& [child_name, child_node] : node.asMap()) {
		if(child_name.empty())
			continue;
		ShvPath child_shv_path = shv::core::Utils::joinPath(shv_path, child_name);
		fromNodesTree_helper(type_info, child_node, child_shv_path, device_type, device_path, node_types);
	}
}

ShvLogTypeInfo ShvLogTypeInfo::fromNodesTree(const chainpack::RpcValue &v)
{
	ShvLogTypeInfo ret;
	{
		const RpcValue types = v.metaValue("typeInfo").asMap().value("types");
		const RpcValue::Map &m = types.asMap();
		for(const auto &kv : m) {
			ret.m_types[kv.first] = ShvLogTypeDescr::fromRpcValue(kv.second);
		}
	}
	const RpcValue node_types = v.metaValue("nodeTypes");
	fromNodesTree_helper(ret, v, {}, {}, {}, node_types.asMap());
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
