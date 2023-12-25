#include <shv/core/utils/getlog.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/patternmatcher.h>

#include <shv/core/exception.h>
#include <shv/core/log.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv::core::utils {

ShvMemoryJournal::ShvMemoryJournal() = default;
void ShvMemoryJournal::setSince(const shv::chainpack::RpcValue &since)
{
	m_logHeader.setSince(since);
}

void ShvMemoryJournal::setUntil(const shv::chainpack::RpcValue &until)
{
	m_logHeader.setUntil(until);
}

void ShvMemoryJournal::setTypeInfo(const ShvTypeInfo &ti, const std::string &path_prefix)
{
	setTypeInfo(ShvTypeInfo(ti), path_prefix);
}

void ShvMemoryJournal::setTypeInfo(ShvTypeInfo &&ti, const std::string &path_prefix)
{
	m_logHeader.setTypeInfo(std::move(ti), path_prefix);
}

void ShvMemoryJournal::setDeviceId(std::string id)
{
	m_logHeader.setDeviceId(std::move(id));
}

void ShvMemoryJournal::setDeviceType(std::string type)
{
	m_logHeader.setDeviceType(std::move(type));
}

const ShvTypeInfo &ShvMemoryJournal::typeInfo(const std::string &path_prefix) const
{
	return m_logHeader.typeInfo(path_prefix);
}

bool ShvMemoryJournal::isShortTimeCorrection() const
{
	return m_isShortTimeCorrection;
}

void ShvMemoryJournal::setShortTimeCorrection(bool b)
{
	m_isShortTimeCorrection = b;
}

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log, bool append_records)
{
	shvLogFuncFrame();
	shv::core::utils::ShvLogRpcValueReader rd(log, !shv::core::Exception::Throw);
	if(!append_records) {
		m_entries.clear();
		m_logHeader = rd.logHeader();
	}
	while(rd.next()) {
		const core::utils::ShvJournalEntry &entry = rd.entry();
		append(entry);
	}
}

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log, const chainpack::RpcValue::Map &default_snapshot_values)
{
	shvLogFuncFrame();
	shv::core::utils::ShvLogRpcValueReader rd(log, !shv::core::Exception::Throw);
	m_entries.clear();
	m_logHeader = rd.logHeader();
	chainpack::RpcValue::Map missing_snapshot_values = default_snapshot_values;
	auto append_default_snapshot_values = [&missing_snapshot_values, this]() {
		for(const auto &kv : missing_snapshot_values) {
			core::utils::ShvJournalEntry e;
			e.epochMsec = m_logHeader.sinceMsec();
			e.path = kv.first;
			e.value = kv.second;
			shvDebug() << "generating snapshot entry with default value, path:" << e.path;
			append(e);
		}
		missing_snapshot_values.clear();
	};
	while(rd.next()) {
		const core::utils::ShvJournalEntry &entry = rd.entry();
		if(rd.isInSnapshot()) {
			shvDebug() << "--:" << entry.path;
			missing_snapshot_values.erase(entry.path);
		}
		else if(!missing_snapshot_values.empty()) {
			append_default_snapshot_values();
		}
		append(entry);
	}
	// call append_default_snapshot_values() for case, that log contains snapshot only
	append_default_snapshot_values();
}

void ShvMemoryJournal::append(const ShvJournalEntry &entry)
{
	int64_t epoch_msec = entry.epochMsec;
	if(epoch_msec == 0) {
		epoch_msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else if(isShortTimeCorrection()) {
		if(!entry.isSpontaneous() && entry.shortTime != shv::core::utils::ShvJournalEntry::NO_SHORT_TIME) {
			auto short_msec = static_cast<uint16_t>(entry.shortTime);
			ShortTime &st = m_recentShortTimes[entry.path];
			if(entry.shortTime == st.recentShortTime) {
				// the same short times in the row, this can happen only when
				// data is badly generated, we will ignore such values
				shvWarning() << "two same short-times in the row:" << entry.shortTime << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
				st.epochTime = epoch_msec;
			}
			else {
				if(st.epochTime == 0) {
					st.epochTime = epoch_msec;
					st.recentShortTime = static_cast<uint16_t>(entry.shortTime);
				}
				else {
					int64_t new_epoch_msec = st.toEpochTime(short_msec);
					int64_t dt_diff = new_epoch_msec - epoch_msec;
					constexpr int64_t MAX_SKEW = 500;
					if(dt_diff < -MAX_SKEW) {
						shvWarning() << "short time too late:" << -dt_diff << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
						// short time is too late
						// pin it to date time from log entry
						st.epochTime = epoch_msec - MAX_SKEW;
						st.recentShortTime = static_cast<uint16_t>(entry.shortTime);
						epoch_msec = st.epochTime;
					}
					else if(dt_diff > MAX_SKEW) {
						shvWarning() << "short time too ahead:" << dt_diff << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
						// short time is too ahead
						// pin it to greater of recent short-time and epoch-time
						st.epochTime = epoch_msec + MAX_SKEW;
						st.recentShortTime = static_cast<uint16_t>(entry.shortTime);
						epoch_msec = st.epochTime;
					}
					else {
						epoch_msec = st.addShortTime(short_msec);
					}
				}
			}
		}
	}

	if (m_pathDictionary.find(entry.path) == m_pathDictionary.end()) {
		m_pathDictionary[entry.path] = static_cast<int>(m_pathDictionary.size() + 1);
	}

	Entry e(entry);
	e.epochMsec = epoch_msec;
	int64_t last_time = m_entries.empty()? 0: m_entries[m_entries.size()-1].epochMsec;
	if(epoch_msec < last_time) {
		auto it = std::upper_bound(m_entries.begin(), m_entries.end(), entry, [](const Entry &e1, const Entry &e2) {
			return e1.epochMsec < e2.epochMsec;
		});
		m_entries.insert(it, std::move(e));
	}
	else {
		m_entries.push_back(std::move(e));
	}
}

chainpack::RpcValue ShvMemoryJournal::getLog(const ShvGetLogParams &params, bool ignore_record_count_limit)
{
	return shv::core::utils::getLog(m_entries, params);
}

bool ShvMemoryJournal::hasSnapshot() const
{
	return m_logHeader.withSnapShot();
}

const std::vector<ShvJournalEntry>& ShvMemoryJournal::entries() const
{
	return m_entries;
}

bool ShvMemoryJournal::isEmpty() const
{
	return  m_entries.empty();
}

size_t ShvMemoryJournal::size() const
{
	return  m_entries.size();
}

const ShvJournalEntry& ShvMemoryJournal::at(size_t ix) const
{
	return  m_entries.at(ix);
}

void ShvMemoryJournal::clear()
{
	m_entries.clear();
}

void ShvMemoryJournal::removeLastEntry()
{
	if (!m_entries.empty()) m_entries.pop_back();
}

uint16_t ShvMemoryJournal::ShortTime::shortTimeDiff(uint16_t msec) const
{
	return static_cast<uint16_t>(msec - recentShortTime);
}

int64_t ShvMemoryJournal::ShortTime::toEpochTime(uint16_t msec) const
{
	return epochTime + shortTimeDiff(msec);
}

int64_t ShvMemoryJournal::ShortTime::addShortTime(uint16_t msec)
{
	epochTime = toEpochTime(msec);
	recentShortTime = msec;
	return epochTime;
}

} // namespace shv
