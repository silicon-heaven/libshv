#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/utils.h>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT DataChange
{
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::DataChange};
		struct Tag { enum Enum {DateTime = chainpack::meta::Tag::USER,
								ShortTime,
								ValueFlags,
								SpecialListValue,
								MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	enum GetValueAgeOption {DONT_CARE_TS = -2, USE_CACHE = -1, RELOAD_FORCE, RELOAD_OLDER};
	static constexpr int NO_SHORT_TIME = -1;
	enum ValueFlag {Snapshot = 0, Spontaneous, Provisional, ValueFlagCount};
	using ValueFlags = unsigned;
	static constexpr ValueFlags NO_VALUE_FLAGS = 0;
	static const char* valueFlagToString(ValueFlag flag);
	static std::string valueFlagsToString(ValueFlags st);

	DataChange() = default;
	DataChange(const RpcValue &val, const RpcValue::DateTime &date_time, int short_time = NO_SHORT_TIME);
	DataChange(const RpcValue &val, unsigned short_time);

	bool isValid() const;
	static bool isDataChange(const RpcValue &rv);

	RpcValue value() const;
	void setValue(const RpcValue &val);

	bool hasDateTime() const;
	RpcValue dateTime() const;
	void setDateTime(const RpcValue &dt);

	int64_t epochMSec() const;

	bool hasShortTime() const;
	RpcValue shortTime() const;
	void setShortTime(const RpcValue &st);

	bool hasValueflags() const;
	void setValueFlags(ValueFlags st);
	ValueFlags valueFlags() const;

	bool isProvisional() const;
	void setProvisional(bool b);

	bool isSpontaneous() const;
	void setSpontaneous(bool b);

	bool isSnapshotValue() const;
	void setIsSnaptshotValue(bool b);

	static bool testBit(const unsigned &n, int pos);
	static void setBit(unsigned &n, int pos, bool b);

	static DataChange fromRpcValue(const RpcValue &val);
	RpcValue toRpcValue() const;
private:
	RpcValue m_value;
	RpcValue::DateTime m_dateTime;
	int m_shortTime = NO_SHORT_TIME;
	ValueFlags m_valueFlags = NO_VALUE_FLAGS;
};
} // namespace shv::chainpack
