#ifndef SHV_CORE_UTILS_ABSTRACTSHVJOURNAL_H
#define SHV_CORE_UTILS_ABSTRACTSHVJOURNAL_H

#include "../shvcoreglobal.h"

#include <string>
#include <map>
#include <set>
#include <regex>

namespace shv {
namespace chainpack { class RpcValue; }
namespace core {
namespace utils {

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

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_ABSTRACTSHVJOURNAL_H
