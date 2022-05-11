#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"
#include "shvpath.h"
#include "../stringview.h"

#include "../log.h"
#include "../exception.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

const int AbstractShvJournal::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;
const char *AbstractShvJournal::KEY_NAME = "name";
const char *AbstractShvJournal::KEY_RECORD_COUNT = "recordCount";
const char *AbstractShvJournal::KEY_PATHS_DICT = "pathsDict";

AbstractShvJournal::~AbstractShvJournal()
{
}

chainpack::RpcValue AbstractShvJournal::getSnapShotMap()
{
	SHV_EXCEPTION("getSnapShot() not implemented");
}

void AbstractShvJournal::clearSnapshot()
{
	m_snapshot = {};
}

static bool is_property_of_live_object(const std::string &property_path, const std::set<std::string> &object_paths)
{
	// not optimal, but we do not expect to have thousands live objects in the snapshot
	for(const std::string &object_path : object_paths) {
		if(String::startsWith(property_path, object_path))
			return true;
	}
	return false;
}

/**
Log only ANY->NOT_DEFAULT changes or changes from NOT_DEFAULT->DEFAULT
Adding DEFAULT value to snapshot will remove this path, so it will be MISSING next time

We can save many log lines thanks to this simple rule
1. short snapshot does not contain paths with default values (can have thousands of lines otherwise)
2. when OPC-UA is disconnected and connected again, then short snapshot is created

Short snapshot is snapshot which does not contain paths with DEFAULT values
*/
bool AbstractShvJournal::addToSnapshot(ShvSnapshot &snapshot, const ShvJournalEntry &entry)
{
	if(entry.domain != ShvJournalEntry::DOMAIN_VAL_CHANGE) {
		//shvDebug() << "remove not CHNG from snapshot:" << RpcValue(entry.toRpcValueMap()).toCpon();
		// always add not-chng to log
		return true;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodeDrop) {
		// NODE_DROP
		/*
		be aware of exact match that might be out of property paths range

		a/b
		a/b-c
		a/b/c
		a/b/c/d
		*/
		shvDebug() << "dropping path:" << entry.path;
		snapshot.keyvals.erase(entry.path);
		std::string property_prefix = entry.path + '/';
		for (auto it = snapshot.keyvals.lower_bound(property_prefix); it != snapshot.keyvals.end(); ) {
			shvDebug() << "  checking if path:" << it->first << "starts with:" << property_prefix;
			if(String::startsWith(it->first, property_prefix)) {
				// it.key starts with key, then delete it from snapshot
				shvDebug() << "  DROP:" << it->first;
				it = snapshot.keyvals.erase(it);
			}
			else {
				shvDebug() << "  BREAK";
				break;
			}
		}
		// always add node-drop to log
		snapshot.liveNodePropertyMaps.erase(entry.path + '/');
		return true;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodePropertyMap) {
		// bulk node update, possibly NODE_NEW
		snapshot.keyvals[entry.path] = entry;
		snapshot.liveNodePropertyMaps.insert(entry.path + '/');
		/**
		We have to keep track of live node objects created by NodePropertyMap

		Imagine this log example
		-> 2022-04-16T18:59:47Z tram/1234 <PropertyMap>{"coupled": true}
		-> 2022-04-16T19:59:47Z tram/1234/coupled false
		Making snapshot from this log will lead to
		-> 2022-04-16T18:59:47Z tram/1234 <PropertyMap>{"coupled": true}
		because
		-> 2022-04-16T19:59:47Z tram/1234/coupled false
		has default value, so it is not saved in the snapshot.

		To mitigate this problem, we are registering live NodePropertyMaps (objects)
		and changes of their properties are saved to the snapshot even if they have default value.
		This is not optimal solution, but it is also not bad solution.
		In the worst case, the snapshot will behave like if 'not-default-values' optimization will be OFF
		*/
		return true;
	}
	if(is_property_of_live_object(entry.path, snapshot.liveNodePropertyMaps)) {
		// property of live object must be stored in the snaphot always
		// doesn't matter if it is default or not-default value
		snapshot.keyvals[entry.path] = entry;
		return true;
	}
	if(entry.value.isDefaultValue()) {
		// writing default value to the snapshot must erase previous value if any
		auto it = snapshot.keyvals.find(entry.path);
		if(it == snapshot.keyvals.end()) {
			/**
			DEFAULT->DEFAULT
			not-spontaneous values are sent to log for all the nodes after app restart
			this can create snapshot like list of log entries when device is restarted

			change to default value is not present in snapshot
			that means that it is firs time after restart or two default-values in the row
			exclude it from logging if the change is not spontaneous

			this can create snapshot from not-default values only when device is restarted
			this optimization can make snapshot on start of log file 10x smaller

			We still have to log SPONTANEOUS values, even with default value, because this is change which
			has happen in reality, thus it is not a subject of the snapshot size optimization
			*/
			return entry.isSpontaneous();
		}
		else {
			/**
			NOT_DEFAULT->DEFAULT
			change from not-default to default must be logged
			we know, that value is not-default, because default values are not stored in snapshot
			*/
			snapshot.keyvals.erase(it);
			return true;
		}
	}
	snapshot.keyvals[entry.path] = entry;
	return true;
};

} // namespace utils
} // namespace core
} // namespace shv
