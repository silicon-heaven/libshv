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
	struct DirAttribute {
		enum {
			Signature = 1 << 0,
			Flags = 1 << 1,
			AccessGrant = 1 << 2,
			Description = 1 << 3,
			Tags = 1 << 4,
		};
	};
	//struct LsAttribute {
	//	enum {
	//		HasChildren = 1 << 0,
	//	};
	//};
public:
	MetaMethod() = default;
	MetaMethod(std::string name, Signature ms, unsigned flags = 0
				, const RpcValue &access_grant = Rpc::ROLE_BROWSE
				, const std::string &description = {}
				, const RpcValue::Map &tags = {});


	bool isValid() const { return !name().empty(); }
	const std::string& name() const {return m_name;}
	const std::string& label() const {return m_label;}
	MetaMethod& setLabel(const std::string &label) { m_label = label; return *this; }
	const std::string& description() const {return m_description;}
	Signature signature() const {return m_signature;}
	unsigned flags() const {return m_flags;}
	const RpcValue& accessGrant() const {return m_accessGrant;}
	RpcValue attributes(unsigned mask) const
	{
		RpcValue::List lst;
		if(mask & static_cast<unsigned>(DirAttribute::Signature))
			lst.push_back(static_cast<unsigned>(m_signature));
		if(mask & DirAttribute::Flags)
			lst.push_back(m_flags);
		if(mask & DirAttribute::AccessGrant)
			lst.push_back(m_accessGrant);
		if(mask & DirAttribute::Description)
			lst.push_back(m_description);
		if(mask & DirAttribute::Tags)
			lst.push_back(m_tags);
		if(lst.empty())
			return name();
		lst.insert(lst.begin(), name());
		return RpcValue{lst};
	}
	const RpcValue::Map& tags() const { return m_tags; }
	RpcValue tag(const std::string &key, const RpcValue& default_value = {}) const { return m_tags.value(key, default_value); }
	MetaMethod& setTag(const std::string &key, const RpcValue& value) { m_tags.setValue(key, value); return *this; }

	RpcValue toRpcValue() const;
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
