#ifndef SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
#define SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H

#include "../shvcoreglobal.h"

//#include <shv/chainpack/datachange.h>
#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <variant>
#include <functional>

namespace shv::chainpack { class MetaMethod; }

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvTypeDescrBase
{
public:
	ShvTypeDescrBase() {}
	ShvTypeDescrBase(const chainpack::RpcValue &v) : m_data(v) {}

	bool isValid() const { return m_data.isMap(); }
protected:
	chainpack::RpcValue dataValue(const std::string &key, const chainpack::RpcValue &default_val = {}) const;
	void setDataValue(const std::string &key, const chainpack::RpcValue &val);

	void setData(const chainpack::RpcValue &data);
	static void mergeTags(shv::chainpack::RpcValue::Map &map);
protected:
	chainpack::RpcValue m_data;
};

class SHVCORE_DECL_EXPORT ShvFieldDescr : public ShvTypeDescrBase
{
	using Super = ShvTypeDescrBase;
public:
	ShvFieldDescr() {}
	//ShvLogTypeDescrField(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	//ShvLogTypeDescrField(const std::string &n)
	//	: ShvLogTypeDescrField(n, std::string(), chainpack::RpcValue(), std::string()) {}
	//ShvLogTypeDescrField(const std::string &name, int bit_pos)
	//	: ShvLogTypeDescrField(n, std::string(), bit_pos, d) {}
	//ShvLogTypeDescrField(const std::string &n, const std::string &type_name)
	//	: ShvLogTypeDescrField(n, type_name, chainpack::RpcValue(), std::string()) {}
	ShvFieldDescr(const std::string &n, const std::string &type_name = std::string(), const chainpack::RpcValue &v = chainpack::RpcValue());

	std::string name() const;
	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	chainpack::RpcValue value() const;
	std::string visualStyleName() const;
	std::string alarm() const;
	int alarmLevel() const;
	std::string alarmDescription() const { return description(); }

	chainpack::RpcValue toRpcValue() const;
	static ShvFieldDescr fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue bitfieldValue(uint64_t val) const;
	uint64_t setBitfieldValue(uint64_t bitfield, uint64_t uval) const;
private:
	std::pair<unsigned, unsigned> bitRange() const;
};

// backward compatibility
using ShvLogTypeDescrField = ShvFieldDescr;

class SHVCORE_DECL_EXPORT ShvTypeDescr : public ShvTypeDescrBase
{
	using Super = ShvTypeDescrBase;
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

	ShvTypeDescr() {}
	//ShvLogTypeDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	ShvTypeDescr(const std::string &type_name) { setType(typeFromString(type_name)); }
	ShvTypeDescr(Type t, SampleType st = SampleType::Continuous)
		: ShvTypeDescr(t, std::vector<ShvFieldDescr>(), st) {}
	ShvTypeDescr(Type t, std::vector<ShvFieldDescr> &&flds, SampleType st = SampleType::Continuous)
	{
		setType(t);
		setFields(flds);
		setSampleType(st);
	}

	Type type() const;
	ShvTypeDescr& setType(Type t);
	std::vector<ShvFieldDescr> fields() const;
	ShvTypeDescr &setFields(const std::vector<ShvFieldDescr> &fields);
	SampleType sampleType() const; // = SampleType::Continuous;
	ShvTypeDescr &setSampleType(SampleType st);

	bool isValid() const { return type() != Type::Invalid; }

	ShvFieldDescr field(const std::string &field_name) const;
	shv::chainpack::RpcValue fieldValue(const shv::chainpack::RpcValue &val, const std::string &field_name) const;

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	std::string alarm() const;
	ShvTypeDescr& setAlarm(const std::string &alarm);
	int alarmLevel() const;
	std::string alarmDescription() const;
	int decimalPlaces() const;
	ShvTypeDescr& setDecimalPlaces(int n);

	chainpack::RpcValue toRpcValue() const;
	static ShvTypeDescr fromRpcValue(const chainpack::RpcValue &v);

	chainpack::RpcValue defaultRpcValue() const;
};

using ShvMethodDescr = shv::chainpack::MetaMethod;

class SHVCORE_DECL_EXPORT ShvNodeDescr : public ShvTypeDescrBase
{
	using Super = ShvTypeDescrBase;
public:
	ShvNodeDescr() {}
	//ShvLogNodeDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS

	std::string typeName() const;
	ShvNodeDescr &setTypeName(const std::string &type_name);
	std::string label() const;
	ShvNodeDescr &setLabel(const std::string &label);
	std::string description() const;
	ShvNodeDescr &setDescription(const std::string &description);
	std::string unit() const;
	ShvNodeDescr &setUnit(const std::string &unit);
	std::string visualStyleName() const;
	ShvNodeDescr &setVisualStyleName(const std::string &visual_style_name);
	std::string alarm() const;
	ShvNodeDescr &setAlarm(const std::string &alarm);

	std::vector<ShvMethodDescr> methods() const;
	ShvMethodDescr method(const std::string &name) const;

	chainpack::RpcValue toRpcValue() const;
	static ShvNodeDescr fromRpcValue(const chainpack::RpcValue &v, chainpack::RpcValue::Map *extra_tags = nullptr);
};

class SHVCORE_DECL_EXPORT ShvTypeInfo
{
public:
	ShvTypeInfo() {}
	ShvTypeInfo(std::map<std::string, ShvTypeDescr> &&types, std::map<std::string, ShvNodeDescr> &&node_descriptions)
		: m_types(std::move(types))
		, m_nodeDescriptions(std::move(node_descriptions))
	{}

	bool isValid() const { return !(m_types.empty() && m_devicePaths.empty() && m_nodeDescriptions.empty()); }

	//const std::map<std::string, ShvLogTypeDescr>& types() const { return m_types; }
	const std::map<std::string, std::string>& devicePaths() const { return m_devicePaths; }
	const std::map<std::string, ShvTypeDescr>& types() const { return m_types; }
	const std::map<std::string, ShvNodeDescr>& nodeDescriptions() const { return m_nodeDescriptions; }
	const std::map<std::string, std::string>& systemPathsRoots() const { return m_systemPathsRoots; }

	ShvTypeInfo& setDevicePath(const std::string &device_path, const std::string &device_type);
	ShvTypeInfo& setNodeDescription(const ShvNodeDescr &node_descr, const std::string &node_path, const std::string &device_type = {} );
	ShvTypeInfo& setExtraTags(const std::string &node_path, const shv::chainpack::RpcValue &tags);
	shv::chainpack::RpcValue extraTags(const std::string &node_path) const;
	ShvTypeInfo& setTypeDescription(const ShvTypeDescr &type_descr, const std::string &type_name);
	ShvNodeDescr nodeDescriptionForPath(const std::string &shv_path, std::string *p_field_name = nullptr) const;
	ShvNodeDescr nodeDescriptionForDevice(const std::string &device_type, const std::string &property_path, std::string *p_field_name = nullptr) const;
	//ShvLogTypeDescr typeDescriptionForPath(const std::string &shv_path) const;
	ShvTypeDescr typeDescriptionForName(const std::string &type_name) const;
	ShvTypeDescr typeDescriptionForPath(const std::string &shv_path) const;
	std::string findSystemPath(const std::string &shv_path) const;

	chainpack::RpcValue typesAsRpcValue() const;
	chainpack::RpcValue toRpcValue() const;
	static ShvTypeInfo fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums = true) const;

	void forEachNodeDescription(const std::string &node_descr_root, std::function<void (const std::string &property_path, const ShvNodeDescr &node_descr)> fn) const;
	void forEachNode(std::function<void (const std::string &shv_path, const ShvNodeDescr &node_descr)> fn) const;
private:
	static ShvTypeInfo fromNodesTree(const chainpack::RpcValue &v);
private:
	std::map<std::string, ShvTypeDescr> m_types; // type_name -> type_description
	std::map<std::string, std::string> m_devicePaths; // path -> deviceType
	std::map<std::string, ShvNodeDescr> m_nodeDescriptions; // device-type/device-property-path -> node_descr or shv-path -> node-descr
	std::map<std::string, shv::chainpack::RpcValue> m_extraTags; // shv-path -> tags
	std::map<std::string, std::string> m_systemPathsRoots; // shv-path-root -> system-path
};

// backward compatibility
using ShvLogMethodDescr = ShvMethodDescr;
using ShvLogFieldDescr = ShvFieldDescr;
using ShvLogTypeDescr = ShvTypeDescr;
using ShvLogNodeDescr = ShvNodeDescr;
using ShvLogTypeInfo = ShvTypeInfo;
using ShvLogPathDescr = ShvNodeDescr;

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
