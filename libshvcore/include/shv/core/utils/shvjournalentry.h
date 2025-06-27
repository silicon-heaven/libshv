#pragma once

#include <shv/core/shvcoreglobal.h>

#include <shv/core/utils/shvtypeinfo.h>

#include <shv/chainpack/datachange.h>
#include <shv/chainpack/rpc.h>

namespace shv::core::utils {

class SHVCORE_DECL_EXPORT ShvJournalEntry
{
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::ShvJournalEntry};
		MetaType();
		static void registerMetaType();
	};
public:
	static constexpr auto DOMAIN_VAL_CHANGE = shv::chainpack::Rpc::SIG_VAL_CHANGED;
	static constexpr auto DOMAIN_VAL_FASTCHANGE = shv::chainpack::Rpc::SIG_VAL_FASTCHANGED;
	static constexpr auto DOMAIN_VAL_SERVICECHANGE = shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED;
	static constexpr auto DOMAIN_SHV_SYSTEM = "SHV_SYS";
	static constexpr auto DOMAIN_SHV_COMMAND = shv::chainpack::Rpc::SIG_COMMAND_LOGGED;


	static constexpr auto PATH_APP_START = "APP_START";
	static constexpr auto PATH_DATA_MISSING = "DATA_MISSING";
	static constexpr auto PATH_DATA_DIRTY = "DATA_DIRTY";

	static constexpr auto DATA_MISSING_UNAVAILABLE = "Unavailable";
	static constexpr auto DATA_MISSING_NOT_EXISTS = "NotExists";


	using ValueFlag = shv::chainpack::DataChange::ValueFlag;
	using ValueFlags = shv::chainpack::DataChange::ValueFlags;

	static constexpr int NO_SHORT_TIME = shv::chainpack::DataChange::NO_SHORT_TIME;
	static constexpr ValueFlags NO_VALUE_FLAGS = shv::chainpack::DataChange::NO_VALUE_FLAGS;

	int64_t epochMsec = 0;
	std::string path;
	shv::chainpack::RpcValue value;
	int shortTime = NO_SHORT_TIME;
	std::string domain;
	unsigned valueFlags = 0;
	std::string userId;

	ShvJournalEntry();
	ShvJournalEntry(const std::string& path_, const shv::chainpack::RpcValue& value_, const std::string& domain_, int short_time, ValueFlags flags, int64_t epoch_msec = 0);
	ShvJournalEntry(const std::string& path_, const shv::chainpack::RpcValue& value_);
	ShvJournalEntry(const std::string& path_, const shv::chainpack::RpcValue& value_, int short_time);
	ShvJournalEntry(const std::string& path_, const shv::chainpack::RpcValue& value_, const std::string& domain_);

	bool isValid() const;
	bool isProvisional() const;
	void setProvisional(bool b);
	bool isSpontaneous() const;
	void setSpontaneous(bool b);
	bool isSnapshotValue() const;
	void setSnapshotValue(bool b);
	bool operator==(const ShvJournalEntry &o) const;
	void setShortTime(int short_time);
	shv::chainpack::RpcValue::DateTime dateTime() const;
	shv::chainpack::RpcValue toRpcValueMap() const;
	shv::chainpack::RpcValue toRpcValueList(const std::function< chainpack::RpcValue (const std::string &)>& map_path = nullptr) const;

	static bool isShvJournalEntry(const shv::chainpack::RpcValue &rv);
	shv::chainpack::RpcValue toRpcValue() const;
	static ShvJournalEntry fromRpcValue(const shv::chainpack::RpcValue &rv);
	static ShvJournalEntry fromRpcValueMap(const shv::chainpack::RpcValue::Map &m);
	static ShvJournalEntry fromRpcValueList(const shv::chainpack::RpcList &row, const std::function< std::string (const chainpack::RpcValue &)>& unmap_path = nullptr, std::string *err = nullptr);

	shv::chainpack::DataChange toDataChange() const;
};
} // namespace shv::core::utils
