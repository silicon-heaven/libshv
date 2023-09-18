#pragma once

#include "rpcvalue.h"
#include "rpc.h"

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
	struct AccessLevel {
		enum {
			None = 0,
			Browse = 1,
			Read = 10,
			Write = 20,
			Command = 30,
			Config = 40,
			Service = 50,
			SuperService = 55,
			Devel = 60,
			Admin = 70,
		};
	};
	struct DirKey {
		enum {
			Name = 1,
			Signature,
			Flags,
			Access,
			Description,
			Label,
			Tags,
		};
	};

	static constexpr auto KEY_NAME = "name";
	static constexpr auto KEY_SIGNATURE = "signature";
	static constexpr auto KEY_FLAGS = "flags";
	static constexpr auto KEY_ACCESS = "access";
	static constexpr auto KEY_DESCRIPTION = "description";
	static constexpr auto KEY_LABEL = "label";
	static constexpr auto KEY_TAGS = "tags";

public:
	MetaMethod();
	MetaMethod(std::string name, Signature ms, unsigned flags = 0
				, const RpcValue &access_grant = Rpc::ROLE_BROWSE
				, const std::string &description = {}
				, const RpcValue::Map &tags = {});


	bool isValid() const;
	const std::string& name() const;
	const std::string& label() const;
	MetaMethod& setLabel(const std::string &label);
	const std::string& description() const;
	Signature signature() const;
	unsigned flags() const;
	const RpcValue& accessGrant() const;
	//RpcValue attributes(unsigned mask) const;
	const RpcValue::Map& tags() const;
	RpcValue tag(const std::string &key, const RpcValue& default_value = {}) const;
	MetaMethod& setTag(const std::string &key, const RpcValue& value);

	RpcValue toRpcValue() const;
	RpcValue toIMap() const;
	static MetaMethod fromRpcValue(const RpcValue &rv);
	void applyAttributesMap(const RpcValue::Map &attr_map);

	static Signature signatureFromString(const std::string &sigstr);
	static const char* signatureToString(Signature sig);
	static std::string flagsToString(unsigned flags);
	static const char* accessLevelToString(int access_level);
private:
	std::string m_name;
	Signature m_signature = Signature::VoidVoid;
	unsigned m_flags = 0;
	RpcValue m_accessGrant;
	std::string m_label;
	std::string m_description;
	RpcValue::Map m_tags;
};

} // namespace chainpack
} // namespace shv
