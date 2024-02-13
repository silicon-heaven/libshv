#pragma once

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <variant>
#include <functional>

namespace shv::chainpack { class MetaMethod; }

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvDescriptionBase
{
public:
	ShvDescriptionBase();
	ShvDescriptionBase(const chainpack::RpcValue &v);

	std::string name() const;
	void setName(const std::string &n);

	bool isEmpty() const;
	bool isValid() const;
	chainpack::RpcValue dataValue(const std::string &key, const chainpack::RpcValue &default_val = {}) const;

	bool operator==(const ShvDescriptionBase &o) const;
protected:
	bool hasName() const;
	void setDataValue(const std::string &key, const chainpack::RpcValue &val);

	void setData(const chainpack::RpcValue &data);
	static void mergeTags(shv::chainpack::RpcValue::Map &map);
protected:
	chainpack::RpcValue m_data;
};

class SHVCORE_DECL_EXPORT ShvFieldDescr : public ShvDescriptionBase
{
	using Super = ShvDescriptionBase;
public:
	ShvFieldDescr();
	ShvFieldDescr(const std::string &name,
				  const std::string &type_name = {},
				  const chainpack::RpcValue &value = {},
				  chainpack::RpcValue::Map &&tags = {});

	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	chainpack::RpcValue value() const;
	std::string visualStyleName() const;
	std::string alarm() const;
	int alarmLevel() const;

	std::string unit() const;

	ShvFieldDescr &setTypeName(const std::string &type_name);
	ShvFieldDescr &setLabel(const std::string &label);
	ShvFieldDescr &setDescription(const std::string &description);
	ShvFieldDescr &setUnit(const std::string &unit);
	ShvFieldDescr &setVisualStyleName(const std::string &visual_style_name);
	ShvFieldDescr &setAlarm(const std::string &alarm);

	chainpack::RpcValue toRpcValue() const;
	static ShvFieldDescr fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue bitfieldValue(uint64_t val) const;
	uint64_t setBitfieldValue(uint64_t bitfield, uint64_t uval) const;
private:
	std::pair<unsigned, unsigned> bitRange() const;
};

class SHVCORE_DECL_EXPORT ShvTypeDescr : public ShvDescriptionBase
{
	using Super = ShvDescriptionBase;
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

	ShvTypeDescr();
	ShvTypeDescr(const std::string &type_name);
	ShvTypeDescr(Type t, SampleType st = SampleType::Continuous, chainpack::RpcValue::Map &&tags = {});
	ShvTypeDescr(Type t, std::vector<ShvFieldDescr> &&flds, SampleType st = SampleType::Continuous, chainpack::RpcValue::Map &&tags = {});

	Type type() const;
	ShvTypeDescr& setType(Type t);
	std::string typeName() const;
	ShvTypeDescr& setTypeName(const std::string &type_name);
	std::vector<ShvFieldDescr> fields() const;
	ShvTypeDescr &setFields(const std::vector<ShvFieldDescr> &fields);
	SampleType sampleType() const; // = SampleType::Continuous;
	ShvTypeDescr &setSampleType(SampleType st);

	// name of TypeDescr which is super-set of this one
	// setting this property effectivelly excudes this type description
	// from localization tools search path, because all the names were
	// localized in super-type already
	std::string restrictionOfType() const;
	bool isSiteSpecificLocalization() const;

	bool isValid() const;

	ShvFieldDescr field(const std::string &field_name) const;
	shv::chainpack::RpcValue fieldValue(const shv::chainpack::RpcValue &val, const std::string &field_name) const;

	static std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	int decimalPlaces() const;
	ShvTypeDescr& setDecimalPlaces(int n);

	chainpack::RpcValue toRpcValue() const;
	static ShvTypeDescr fromRpcValue(const chainpack::RpcValue &v);

	chainpack::RpcValue defaultRpcValue() const;
};

using ShvMethodDescr = shv::chainpack::MetaMethod;

class SHVCORE_DECL_EXPORT ShvPropertyDescr : public ShvFieldDescr
{
	using Super = ShvFieldDescr;
public:
	ShvPropertyDescr();
	ShvPropertyDescr(const std::string &name, const std::string &type_name);
	[[deprecated]] ShvPropertyDescr(const std::string &type_name);

	std::vector<ShvMethodDescr> methods() const;
	ShvMethodDescr method(const std::string &name) const;
	ShvPropertyDescr& addMethod(const ShvMethodDescr &method_descr);
	ShvPropertyDescr& setMethod(const ShvMethodDescr &method_descr);

	chainpack::RpcValue toRpcValue() const;
	static ShvPropertyDescr fromRpcValue(const chainpack::RpcValue &v, chainpack::RpcValue::Map *extra_tags = nullptr);
};

class SHVCORE_DECL_EXPORT ShvDeviceDescription
{
public:
	using Properties = std::vector<ShvPropertyDescr>;

	Properties properties;
	/*
	 * Restriction of device means, that current device has only subset o original device properties.
	 * FL will create property sheet of original device and hide properties not existing in current device.
	 * Also all localization phrases can be found in original device description
	 *
	 * FL uses restrictionOfDevice as device-type to create proper Document, Controller and PropertySheet.
	 * restrictionOfDevice cannot be used recursivelly
	 *
	 * Redefinition of properties in restricted device is not allowed.
	 *
	 * This property is not well designed and it will be deprecated.
	 */
	std::string restrictionOfDevice;
	/*
	 * Generate site specific localization file for this site & device combination
	 */
	bool siteSpecificLocalization = false;

	static ShvDeviceDescription fromRpcValue(const chainpack::RpcValue &v);
	chainpack::RpcValue toRpcValue() const;

	ShvDeviceDescription& setPropertyDescription(const ShvPropertyDescr &property_descr);

	Properties::iterator findProperty(const std::string &name);
	Properties::const_iterator findProperty(const std::string &name) const;
	Properties::const_iterator findLongestPropertyPrefix(const std::string &name) const;
};

class SHVCORE_DECL_EXPORT ShvTypeInfo
{
public:
	ShvTypeInfo() = default;

	static ShvTypeInfo fromVersion2(std::map<std::string, ShvTypeDescr> &&types, const std::map<std::string, ShvPropertyDescr> &node_descriptions);
	static ShvTypeInfo fromVersion2(std::map<std::string, ShvTypeDescr> &&types, const std::vector<ShvPropertyDescr> &property_descriptions);

	bool isValid() const;

	using DeviceProperties = ShvDeviceDescription::Properties;
	using DevicePropertiesMap = std::map<std::string, DeviceProperties>;

	const std::map<std::string, std::string>& devicePaths() const;
	const std::map<std::string, ShvTypeDescr>& types() const;
	const std::map<std::string, ShvDeviceDescription>& deviceDescriptions() const;
	const ShvDeviceDescription& deviceDescription(const std::string &device_type) const;
	const std::map<std::string, std::string>& systemPathsRoots() const;
	const std::map<std::string, shv::chainpack::RpcValue>& extraTags() const;

	ShvTypeInfo& setDevicePath(const std::string &device_path, const std::string &device_type);
	ShvTypeInfo& setDeviceDescription(const std::string &device_type, const ShvDeviceDescription &device_descr);
	ShvTypeInfo& setPropertyDescription(const std::string &device_type, const ShvPropertyDescr &property_descr);
	ShvTypeInfo& setExtraTags(const std::string &node_path, const shv::chainpack::RpcValue &tags);
	shv::chainpack::RpcValue extraTags(const std::string &node_path) const;

	[[deprecated("setTypeDescription(const ShvTypeDescr &type_descr, const std::string &type_name) is deprecated, use setTypeDescription(const std::string &type_name, const ShvTypeDescr &type_descr)")]] ShvTypeInfo& setTypeDescription(const ShvTypeDescr &type_descr, const std::string &type_name)
	{
		setTypeDescription(type_name, type_descr);
		return *this;
	}
	ShvTypeInfo& setTypeDescription(const std::string &type_name, const ShvTypeDescr &type_descr);

	ShvPropertyDescr propertyDescriptionForPath(const std::string &shv_path, std::string *p_field_name = nullptr) const;

	/// return auto [device_path, device_type, property_path]
	std::tuple<std::string, std::string, std::string> findDeviceType(const std::string &shv_path) const;
	/// return auto [own_property_path, field_path, property_descr]
	std::tuple<ShvPropertyDescr, std::string> findPropertyDescription(const std::string &device_type, const std::string &property_path) const;
	ShvTypeDescr findTypeDescription(const std::string &type_name) const;

	ShvTypeDescr typeDescriptionForPath(const std::string &shv_path) const;
	shv::chainpack::RpcValue extraTagsForPath(const std::string &shv_path) const;
	std::string findSystemPath(const std::string &shv_path) const;
	bool isPathBlacklisted(const std::string &shv_path) const;
	void setBlacklist(const std::string &shv_path, const chainpack::RpcValue &blacklist);
	void setPropertyDeviation(const std::string &shv_path, const ShvPropertyDescr &property_descr);

	struct SHVCORE_DECL_EXPORT PathInfo
	{
		std::string devicePath;
		std::string deviceType;
		ShvPropertyDescr propertyDescription;
		std::string fieldPath;
	};
	PathInfo pathInfo(const std::string &shv_path) const;

	chainpack::RpcValue typesAsRpcValue() const;
	chainpack::RpcValue toRpcValue() const;
	static ShvTypeInfo fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums = true) const;
	shv::chainpack::RpcValue applyTypeDescription(const shv::chainpack::RpcValue &val, const ShvTypeDescr &type_descr, bool translate_enums = true) const;

	void forEachDeviceProperty(const std::string &device_type, std::function<void (const ShvPropertyDescr &)> fn) const;
	void forEachProperty(std::function<void (const std::string &shv_path, const ShvPropertyDescr &)> fn) const;
private:
	static ShvTypeInfo fromNodesTree(const chainpack::RpcValue &v);
	void fromNodesTree_helper(const shv::chainpack::RpcValue::Map &node_types,
							  const shv::chainpack::RpcValue &node,
							  const std::string &device_type,
							  const std::string &device_path,
							  const std::string &property_path,
							  ShvDeviceDescription *device_description);
private:
	std::map<std::string, ShvTypeDescr> m_types; // type-name -> type-description
	std::map<std::string, std::string> m_devicePaths; // path -> device-type-name
	std::map<std::string, ShvDeviceDescription> m_deviceDescriptions; // device-type-name -> device-descr
	std::map<std::string, shv::chainpack::RpcValue> m_extraTags; // shv-path -> tags
	std::map<std::string, std::string> m_systemPathsRoots; // shv-path-root -> system-path
	std::map<std::string, chainpack::RpcValue> m_blacklistedPaths; // shv-path -> blacklist
	std::map<std::string, ShvPropertyDescr> m_propertyDeviations; // should be empty, devices should not have different property descriptions for same device-type
};

// backward compatibility
using ShvLogMethodDescr [[deprecated("ShvLogMethodDescr is deprecated, use ShvMethodDescr instead")]] = ShvMethodDescr;
using ShvLogFieldDescr [[deprecated("ShvLogFieldDescr is deprecated, use ShvFieldDescr instead")]] = ShvFieldDescr;
using ShvLogTypeDescr [[deprecated("ShvLogTypeDescr is deprecated, use ShvTypeDescr instead")]] = ShvTypeDescr;
using ShvLogNodeDescr [[deprecated("ShvLogNodeDescr is deprecated, use ShvPropertyDescr instead")]] = ShvPropertyDescr;
using ShvNodeDescr [[deprecated("ShvNodeDescr is deprecated, use ShvPropertyDescr instead")]] = ShvPropertyDescr;
using ShvLogTypeInfo [[deprecated("ShvLogTypeInfo is deprecated, use ShvTypeInfo instead")]] = ShvTypeInfo;
using ShvLogPathDescr [[deprecated("ShvLogPathDescr is deprecated, use ShvPropertyDescr instead")]] = ShvPropertyDescr;
using ShvLogTypeDescrField [[deprecated("ShvLogTypeDescrField is deprecated, use ShvFieldDescr instead")]] = ShvFieldDescr;


} // namespace utils
} // namespace core
} // namespace shv
