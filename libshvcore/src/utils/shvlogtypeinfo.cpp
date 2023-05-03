#include "shvlogtypeinfo.h"

using namespace std;
using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

//=====================================================================
// ShvLogTypeDescrField
//=====================================================================
RpcValue ShvLogTypeDescrField::toRpcValue() const
{
	RpcValue::Map m;
	m["name"] = name;
	if(!typeName.empty())
		m["typeName"] = typeName;
	if(!label.empty())
		m["label"] = label;
	if(!description.empty())
		m["description"] = description;
	if(!label.empty() && label != name)
		m["label"] = label;
	if(value.isValid())
		m["value"] = value;
	if(!tags.empty())
		m["tags"] = tags;
	return m;
}

ShvLogTypeDescrField ShvLogTypeDescrField::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeDescrField ret;
	if(v.isMap()) {
		const RpcValue::Map &m = v.asMap();
		ret.name = m.value("name").asString();
		ret.label = m.value("label").asString();
		ret.description = m.value("description").asString();
		ret.typeName = m.value("typeName").asString();
		ret.value = m.value("value");
		ret.tags = m.value("tags").asMap();
		if(ret.tags.empty())
			ret.tags = m.value("options").asMap();
	}
	return ret;
}

//=====================================================================
// ShvLogTypeDescr
//=====================================================================
const char* ShvLogTypeDescr::OPT_MIN_VAL = "minVal";
const char* ShvLogTypeDescr::OPT_MAX_VAL = "maxVal";
const char* ShvLogTypeDescr::OPT_DEC_PLACES = "decPlaces";

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

void ShvLogTypeDescr::applyTags(const RpcValue::Map &t)
{
	if (type == Type::Invalid)
		type = typeFromString(t.value("typeName").toStdString());

	if (t.hasKey("sampleType"))
		sampleType = sampleTypeFromString(t.value("sampleType").asString());

	if (t.hasKey(description))
		description = t.value("description").toStdString();

	tags.insert(t.begin(), t.end());
}

RpcValue ShvLogTypeDescr::defaultRpcValue() const
{
	using namespace chainpack;

	switch (type) {
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

std::string ShvLogTypeDescr::unit() const
{
	return tags.value("unit", std::string()).toStdString();
}

std::string ShvLogTypeDescr::visualStyleName() const
{
	return tags.value("visualStyle", std::string()).toStdString();
}

std::string ShvLogTypeDescr::alarm() const
{
	return tags.value("alarm", std::string()).toStdString();
}

int ShvLogTypeDescr::decimalPlaces() const
{
	return tags.value("decPlaces", 0).toInt();
}

RpcValue ShvLogTypeDescr::toRpcValue() const
{
	RpcValue::Map m;
	if(type != Type::Invalid)
		m["type"] = typeToString(type);
	if(sampleType != SampleType::Continuous)
		m["sampleType"] = sampleTypeToString(sampleType);
	if(!description.empty())
		m["description"] = description;
	if(!fields.empty()) {
		RpcValue::List lst;
		for(const ShvLogTypeDescrField &fld : fields)
			lst.push_back(fld.toRpcValue());
		m["fields"] = std::move(lst);
	}
	if(!tags.empty()) {
		m["tags"] = tags;
	}
	return m;
}

ShvLogTypeDescr ShvLogTypeDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeDescr ret;
	if(v.isMap()) {
		const RpcValue::Map &m = v.toMap();
		ret.type = typeFromString(m.value("type").toString());
		ret.sampleType = sampleTypeFromString(m.value("sampleType").asString());
		ret.description = m.value("description").toString();
		int current_value = 0;
		for(const auto &rv : m.valref("fields").toList()) {
			auto fld = ShvLogTypeDescrField::fromRpcValue(rv);
			if(ret.type == Type::Enum) {
				if(fld.value.isInt())
					current_value = fld.value.toInt() + 1;
				else
					fld.value = current_value++;
			}
			ret.fields.push_back(std::move(fld));
		}

		ret.tags = m.value("tags").toMap();
		if(ret.tags.empty())
			ret.tags = m.value("options").toMap();
	}
	return ret;
}

//=====================================================================
// ShvLogPathDescr
//=====================================================================
ShvLogPathDescr ShvLogPathDescr::fromTags(const RpcValue::Map &tags)
{
	ShvLogPathDescr ret;
	ret.typeName = tags.value("typeName").asString();
	ret.description = tags.value("description").asString();
	ret.tags = tags;
	return ret;
}

RpcValue ShvLogPathDescr::toRpcValue() const
{
	RpcValue::Map m;
	m["type"] = typeName;
	if(!description.empty())
		m["description"] = description;
	if(!tags.empty())
		m["tags"] = tags;
	return m;
}

ShvLogPathDescr ShvLogPathDescr::fromRpcValue(const RpcValue &v)
{
	if(v.isMap()) {
		const RpcValue::Map &m = v.toMap();
		ShvLogPathDescr ret = ShvLogPathDescr::fromTags(m.value("tags").asMap());
		const string &type_name = m.valref("type").asString();
		if(!type_name.empty())
			ret.typeName = type_name;
		const string &description = m.valref("description").asString();
		if(!description.empty())
			ret.description = description;
		return ret;
	}
	return {};
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
ShvLogTypeDescr ShvLogTypeInfo::typeDescription(const std::string &shv_path) const
{
	ShvLogTypeDescr ret;
	auto it_path_descr = paths.find(shv_path);
	if (it_path_descr == paths.end())
		return ret;

	auto it_type_descr = types.find(it_path_descr->second.typeName);
	if (it_type_descr != types.end())
		ret = it_type_descr->second;

	ret.applyTags(it_path_descr->second.tags);
	return ret;
}

std::string ShvLogTypeInfo::findSystemPath(const std::string &shv_path) const
{
	auto path_descr = paths.find(shv_path);
	return (path_descr == paths.end()) ? std::string() : path_descr->second.systemPath;
}

RpcValue ShvLogTypeInfo::toRpcValue() const
{
	RpcValue::Map m;
	RpcValue::Map mt;
	for(const auto &kv : types) {
		mt[kv.first] = kv.second.toRpcValue();
	}
	m["types"] = std::move(mt);
	RpcValue::Map mp;
	for(const auto &kv : paths) {
		mp[kv.first] = kv.second.toRpcValue();
	}
	m["paths"] = std::move(mp);
	return m;
}

ShvLogTypeInfo ShvLogTypeInfo::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeInfo ret;
	const RpcValue::Map &m = v.toMap();
	const RpcValue::Map &mt = m.valref("types").toMap();
	for(const auto &kv : mt) {
		ret.types[kv.first] = ShvLogTypeDescr::fromRpcValue(kv.second);
	}
	const RpcValue::Map &mp = m.valref("paths").toMap();
	for(const auto &kv : mp) {
		ret.paths[kv.first] = ShvLogPathDescr::fromRpcValue(kv.second);
	}
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
