#pragma once

#include "../shvcoreglobal.h"

#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"
#include "shvlogheader.h"

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvMemoryJournal : public AbstractShvJournal
{
public:
	ShvMemoryJournal();

	void setSince(const shv::chainpack::RpcValue &since);
	void setUntil(const shv::chainpack::RpcValue &until);
	void setTypeInfo(const ShvTypeInfo &ti, const std::string &path_prefix = ShvLogHeader::EMPTY_PREFIX_KEY);
	void setTypeInfo(ShvTypeInfo &&ti, const std::string &path_prefix = ShvLogHeader::EMPTY_PREFIX_KEY);
	void setDeviceId(std::string id);
	void setDeviceType(std::string type);

	const ShvTypeInfo &typeInfo(const std::string &path_prefix = ShvLogHeader::EMPTY_PREFIX_KEY) const;

	bool isShortTimeCorrection() const;
	void setShortTimeCorrection(bool b);

	void append(const ShvJournalEntry &entry) override;

	void loadLog(const shv::chainpack::RpcValue &log, bool append_records = false);
	void loadLog(const shv::chainpack::RpcValue &log, const shv::chainpack::RpcValue::Map &default_snapshot_values);
	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;

	bool hasSnapshot() const;

	const std::vector<ShvJournalEntry>& entries() const;
	bool isEmpty() const;
	size_t size() const;
	const ShvJournalEntry& at(size_t ix) const;
	void clear();
	void removeLastEntry();
private:
	using Entry = ShvJournalEntry;

	std::map<std::string, int> m_pathDictionary;

	ShvLogHeader m_logHeader;
	std::vector<Entry> m_entries;

	struct ShortTime {
		int64_t epochTime = 0;
		uint16_t recentShortTime = 0;

		uint16_t shortTimeDiff(uint16_t msec) const;
		int64_t toEpochTime(uint16_t msec) const;
		int64_t addShortTime(uint16_t msec);
	};

	bool m_isShortTimeCorrection = false;
	std::map<std::string, ShortTime> m_recentShortTimes;
};

} // namespace utils
} // namespace core
} // namespace shv

