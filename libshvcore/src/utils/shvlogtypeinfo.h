#ifndef SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
#define SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H

#include "../shvcoreglobal.h"

//#include <shv/chainpack/datachange.h>
#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <variant>

namespace shv::chainpack { class MetaMethod; }

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvLogDescrBase
{
public:
	ShvLogDescrBase() {}
	ShvLogDescrBase(const chainpack::RpcValue &v) : m_data(v) {}

	bool isValid() const { return m_data.isMap(); }
protected:
	chainpack::RpcValue dataValue(const std::string &key, const chainpack::RpcValue &default_val = {}) const;
	void setDataValue(const std::string &key, const chainpack::RpcValue &val);

	void setData(const chainpack::RpcValue &data);
	static void mergeTags(shv::chainpack::RpcValue::Map &map);
protected:
	chainpack::RpcValue m_data;
};

class SHVCORE_DECL_EXPORT ShvLogTypeDescrField : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	ShvLogTypeDescrField() {}
	//ShvLogTypeDescrField(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	//ShvLogTypeDescrField(const std::string &n)
	//	: ShvLogTypeDescrField(n, std::string(), chainpack::RpcValue(), std::string()) {}
	//ShvLogTypeDescrField(const std::string &name, int bit_pos)
	//	: ShvLogTypeDescrField(n, std::string(), bit_pos, d) {}
	//ShvLogTypeDescrField(const std::string &n, const std::string &type_name)
	//	: ShvLogTypeDescrField(n, type_name, chainpack::RpcValue(), std::string()) {}
	ShvLogTypeDescrField(const std::string &n, const std::string &type_name = std::string(), const chainpack::RpcValue &v = chainpack::RpcValue());

	std::string name() const;
	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	chainpack::RpcValue value() const;
	std::string alarm() const;
	int alarmLevel() const;
	std::string alarmDescription() const { return description(); }

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescrField fromRpcValue(const chainpack::RpcValue &v);
};

class SHVCORE_DECL_EXPORT ShvLogTypeDescr : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	enum class Type {
		Invalid,
		BitField,
		Enum,
		Bool,
		UInt,
		Int,
		Decimal,
		Double,
		String,
		DateTime,
		List,
		Map,
		IMap,
	};
	enum class SampleType {Invalid = 0, Continuous , Discrete};

	ShvLogTypeDescr() {}
	//ShvLogTypeDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	//ShvLogTypeDescr(const std::string &type_name) : type(typeFromString(type_name)) { }
	ShvLogTypeDescr(Type t, SampleType st = SampleType::Continuous)
		: ShvLogTypeDescr(t, std::vector<ShvLogTypeDescrField>(), st) {}
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, SampleType st = SampleType::Continuous);

	Type type() const;
	ShvLogTypeDescr& setType(Type t);
	std::vector<ShvLogTypeDescrField> fields() const;
	ShvLogTypeDescr &setFields(const std::vector<ShvLogTypeDescrField> &fields);
	SampleType sampleType() const; // = SampleType::Continuous;
	ShvLogTypeDescr &setSampleType(SampleType st);

	//ShvLogTypeDescr& withTags(const chainpack::RpcValue::Map &tags) { this->tags = tags; return *this; }
	ShvLogTypeDescr& withType(Type t);

	bool isValid() const { return type() != Type::Invalid; }

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	//void applyTags(const chainpack::RpcValue::Map &t);
	//chainpack::RpcValue defaultRpcValue() const;

	//std::string unit() const;
	std::string visualStyleName() const;
	std::string alarm() const;
	int alarmLevel() const;
	std::string alarmDescription() const;
	int decimalPlaces() const;
	ShvLogTypeDescr& setDecimalPlaces(int n);

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescr fromRpcValue(const chainpack::RpcValue &v);
};

class SHVCORE_DECL_EXPORT ShvLogNodeDescr : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	ShvLogNodeDescr() {}
	//ShvLogNodeDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS

	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	std::string unit() const;
	//std::string alarm() const;
	//int alarmLevel() const;
	//chainpack::RpcValue tags() const;
	std::vector<shv::chainpack::MetaMethod> methods() const;

	chainpack::RpcValue toRpcValue() const;
	static ShvLogNodeDescr fromRpcValue(const chainpack::RpcValue &v);
	//static ShvLogNodeDescr fromTags(const chainpack::RpcValue::Map &tags);
};

class SHVCORE_DECL_EXPORT ShvLogTypeInfo
{
public:
	ShvLogTypeInfo() {}
	ShvLogTypeInfo(std::map<std::string, ShvLogTypeDescr> &&types, std::map<std::string, ShvLogNodeDescr> &&paths)
		: m_types(std::move(types))
		, m_nodeDescriptions(std::move(paths))
	{}

	bool isEmpty() const { return m_types.size() == 0 && m_nodeDescriptions.size() == 0; }
	//const std::map<std::string, ShvLogTypeDescr>& types() const { return m_types; }
	const std::map<std::string, std::string>& devicePaths() const { return m_devicePaths; }
	const std::map<std::string, ShvLogTypeDescr>& types() const { return m_types; }
	const std::map<std::string, ShvLogNodeDescr>& nodeDescriptions() const { return m_nodeDescriptions; }

	ShvLogTypeInfo& setDevicePath(const std::string &device_path, const std::string &device_type);
	ShvLogTypeInfo& setNodeDescription(const ShvLogNodeDescr &node_descr, const std::string &node_path, const std::string &device_type = {} );
	ShvLogTypeInfo& setTypeDescription(const ShvLogTypeDescr &type_descr, const std::string &type_name);
	ShvLogNodeDescr nodeDescriptionForPath(const std::string &shv_path) const;
	ShvLogNodeDescr nodeDescriptionForDevice(const std::string &device_type, const std::string &property_path) const;
	//ShvLogTypeDescr typeDescriptionForPath(const std::string &shv_path) const;
	ShvLogTypeDescr typeDescriptionForName(const std::string &type_name) const;
	std::string findSystemPath(const std::string &shv_path) const;

	chainpack::RpcValue typesAsRpcValue() const;
	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeInfo fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums = true) const;

	static shv::chainpack::RpcValue fieldValue(const shv::chainpack::RpcValue &val, const ShvLogTypeDescrField &field_descr);
private:
	std::map<std::string, ShvLogTypeDescr> m_types; // type_name -> type_description
	std::map<std::string, std::string> m_devicePaths; // path -> device
	std::map<std::string, ShvLogNodeDescr> m_nodeDescriptions; // node/device_property path -> node_descr
	std::map<std::string, std::string> m_systemPathsRoots;
};

// backward compatibility
using ShvLogPathDescr = ShvLogNodeDescr;

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
