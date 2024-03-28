#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>

#include <optional>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT MetaMethod
{
public:
	enum class Signature {VoidVoid = 0, VoidParam, RetVoid, RetParam};
	struct Flag {
		enum {
			None = 0,
			IsSignal = 1 << 0,
			IsGetter = 1 << 1,
			IsSetter = 1 << 2,
			LargeResultHint = 1 << 3,
		};
	};
	enum class AccessLevel {
		None = 0,
		Browse = 1,
		Read = 8,
		Write = 16,
		Command = 24,
		Config = 32,
		Service = 40,
		SuperService = 48,
		Devel = 56,
		Admin = 63,
	};

	static constexpr auto KEY_NAME = "name";
	static constexpr auto KEY_SIGNATURE = "signature";
	static constexpr auto KEY_FLAGS = "flags";
	static constexpr auto KEY_ACCESS = "access";
	static constexpr auto KEY_ACCESS_LEVEL = "accessLevel";
	static constexpr auto KEY_DESCRIPTION = "description";
	static constexpr auto KEY_LABEL = "label";
	static constexpr auto KEY_TAGS = "tags";
public:
	struct SHVCHAINPACK_DECL_EXPORT Signal {
		Signal(std::string name, std::optional<std::string> param_type = {});
		std::string name;
		std::optional<std::string> param_type;
	};

	MetaMethod();
	MetaMethod(
		std::string name,
		unsigned flags = 0,
		std::optional<std::string> param = {},
		std::optional<std::string> result = {},
		AccessLevel access_level = AccessLevel::Browse,
		const std::vector<Signal>& signal_definitions = {},
		const std::string& description = {},
		const std::string& label = {},
		const RpcValue::Map& extra = {});

	// SHV 2 compatibility constructor
	MetaMethod(std::string name,
		Signature signature,
		unsigned flags,
		const std::string &access_grant,
		const std::string& description = {},
		const RpcValue::Map& extra = {});


	bool isValid() const;
	const std::string& name() const;
	const std::string& resultType() const;
	bool hasResultType() const;
	const std::string& paramType() const;
	bool hasParamType() const;
	const std::string& label() const;
	MetaMethod& setLabel(const std::string &label);
	const std::string& description() const;
	unsigned flags() const;
	int accessLevelAsInt() const;
	std::optional<MetaMethod::AccessLevel> accessLevel() const;
	void setAccessLevel(std::optional<AccessLevel> level);
	const RpcValue::Map& extra() const;
	RpcValue tag(const std::string &key, const RpcValue& default_value = {}) const;
	MetaMethod& setTag(const std::string &key, const RpcValue& value);

	RpcValue toRpcValue() const;
	static MetaMethod fromRpcValue(const RpcValue &rv);
	void applyAttributesMap(const RpcValue::Map &attr_map);
	void applyAttributesIMap(const RpcValue::IMap &attr_map);

	static Signature signatureFromString(const std::string &sigstr);
	static const char* signatureToString(Signature sig);
	static std::string flagsToString(unsigned flags);
	static const char* accessLevelToAccessString(AccessLevel access_level);
	static std::optional<AccessLevel> accessLevelFromAccessString(std::string_view s);
	static std::optional<AccessLevel> accessLevelFromInt(int i);
	//static AccessLevel findAccessLevel(const std::string &access);
private:
	std::string m_name;
	unsigned m_flags = 0;
	std::string m_param;
	std::string m_result;
	int m_accessLevelInt;
	RpcValue::Map m_signals;
	std::string m_label;
	std::string m_description;
	RpcValue::Map m_extra;
};

namespace methods {
const auto LS = MetaMethod{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Flag::None, "LsParam", "LsResult", shv::chainpack::MetaMethod::AccessLevel::Browse};
const auto DIR = MetaMethod{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Flag::None, "DirParam", "DirResult", shv::chainpack::MetaMethod::AccessLevel::Browse};
}

constexpr bool operator<(int a, const MetaMethod::AccessLevel b)
{
	return a < static_cast<int>(b);
}

constexpr bool operator==(int a, const MetaMethod::AccessLevel b)
{
	return a == static_cast<int>(b);
}

constexpr bool operator>(int a, const MetaMethod::AccessLevel b)
{
	return a > static_cast<int>(b);
}

constexpr bool operator<(const MetaMethod::AccessLevel a, const MetaMethod::AccessLevel b)
{
	return static_cast<int>(a) < static_cast<int>(b);
}

constexpr bool operator>(const MetaMethod::AccessLevel a, const MetaMethod::AccessLevel b)
{
	return static_cast<int>(a) > static_cast<int>(b);
}

} // namespace chainpack
} // namespace shv
