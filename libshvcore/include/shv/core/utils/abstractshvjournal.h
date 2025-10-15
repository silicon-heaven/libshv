#pragma once

#include <shv/core/shvcoreglobal.h>

#include <string>
#include <map>

namespace shv {
namespace chainpack { class RpcValue; }
namespace core::utils {

class ShvJournalEntry;
struct ShvGetLogParams;

struct SHVCORE_DECL_EXPORT ShvSnapshot
{
	std::map<std::string, ShvJournalEntry> keyvals;
};

enum class IgnoreRecordCountLimit{
	Yes,
	No
};

class SHVCORE_DECL_EXPORT AbstractShvJournal
{
public:
	virtual ~AbstractShvJournal();

	virtual void append(const ShvJournalEntry &entry) = 0;
	virtual shv::chainpack::RpcValue getLog(const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No) = 0;
	virtual shv::chainpack::RpcValue getSnapShotMap();
	void clearSnapshot();
	static void addToSnapshot(ShvSnapshot &snapshot, const ShvJournalEntry &entry);
protected:
	ShvSnapshot m_snapshot;
};

} // namespace core::utils
} // namespace shv
