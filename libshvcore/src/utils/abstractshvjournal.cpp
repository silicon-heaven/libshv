#include <shv/core/utils/abstractshvjournal.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvpath.h>

#include <shv/core/log.h>
#include <shv/core/exception.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

using namespace shv::chainpack;

namespace shv::core::utils {
AbstractShvJournal::~AbstractShvJournal() = default;

chainpack::RpcValue AbstractShvJournal::getSnapShotMap()
{
	SHV_EXCEPTION("getSnapShot() not implemented");
}

void AbstractShvJournal::clearSnapshot()
{
	m_snapshot = {};
}

void AbstractShvJournal::addToSnapshot(ShvSnapshot &snapshot, const ShvJournalEntry &entry)
{
	if(entry.domain != ShvJournalEntry::DOMAIN_VAL_CHANGE) {
		return;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodeDrop) {
		// NODE_DROP
		shvDebug() << "dropping path:" << entry.path;
		ShvPath::forEachDirAndSubdirsDeleteIf(snapshot.keyvals, entry.path, [](decltype(snapshot.keyvals)::const_iterator){return true;});
	}
	else {
		snapshot.keyvals[entry.path] = entry;
	}
}

} // namespace shv
