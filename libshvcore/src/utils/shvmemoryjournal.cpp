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

static int64_t min_valid(int64_t a, int64_t b)
{
	if(a == 0)
		return b;
	if(b == 0)
		return a;
	return std::min(a, b);
}

chainpack::RpcValue ShvMemoryJournal::getLog(const ShvGetLogParams &params, bool is_record_count_limit_limitted)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();
	using Column = ShvLogHeader::Column;
	cp::RpcValue::List log;
	cp::RpcValue::Map path_cache;
	int max_path_index = 0;
	int rec_cnt = 0;

	auto params_since_msec = params.since.toDateTime().msecsSinceEpoch();
	auto params_until_msec = params.until.toDateTime().msecsSinceEpoch();

	int64_t log_since_msec = m_logHeader.sinceMsec();
	int64_t log_until_msec = m_logHeader.untilMsec();

	int64_t since_msec = std::max(log_since_msec, params_since_msec);
	int64_t until_msec = min_valid(log_until_msec, params_until_msec);

	int rec_cnt_limit = (is_record_count_limit_limitted)? std::min(params.recordCountLimit, ShvJournalCommon::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT): params.recordCountLimit;
	bool rec_cnt_limit_hit = false;

	int64_t last_record_msec = 0;

	if(params_since_msec > 0 && log_until_msec > 0 && params_since_msec >= log_until_msec)
		goto log_finish;
	if(params_until_msec > 0 && log_since_msec > 0 && params_until_msec < log_since_msec)
		goto log_finish;

	{

		auto it1 = m_entries.begin();
		if(params_since_msec > 0) {
			Entry e;
			e.epochMsec = params_since_msec;
			it1 = std::lower_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
				return e1.epochMsec < e2.epochMsec;
			});
		}
		auto it2 = m_entries.end();
		if(params_until_msec > 0) {
			Entry e;
			e.epochMsec = params_until_msec;
			it2 = std::upper_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
				return e1.epochMsec < e2.epochMsec;
			});
		}

		/// this ensure that there be only one copy of each path in memory
		auto make_path_shared = [&path_cache, &max_path_index, &params](const std::string &path) -> cp::RpcValue {
			cp::RpcValue ret = path_cache.value(path);
			if(ret.isValid())
				return ret;
			if(params.withPathsDict)
				ret = ++max_path_index;
			else
				ret = path;
			logIShvJournal() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
			path_cache[path] = ret;
			return ret;
		};

		PatternMatcher pm(params);

		ShvSnapshot snapshot;
		if(params.withSnapshot) {
			for(auto it = m_entries.begin(); it != m_entries.end() && it < it1; ++it) {
				const Entry &e = *it;
				if (it < it1 || e.epochMsec == since_msec) {  //it1 (lower_bound) can be == since_msec (we want in snapshot)
					if(!pm.match(e))                          //or > since_msec (we don't want in snapshot)
						continue;
					addToSnapshot(snapshot, e);
				}
			}
			if(!snapshot.keyvals.empty()) {
				logDShvJournal() << "\t -------------- Snapshot";
				for(const auto &kv : snapshot.keyvals) {
					if(rec_cnt >= rec_cnt_limit) {
						rec_cnt_limit_hit = true;
						goto log_finish;
					}
					auto entry = kv.second;
					entry.setSnapshotValue(true);
					entry.setSpontaneous(false);
					if(since_msec == 0)
						since_msec = entry.epochMsec;
					last_record_msec = since_msec;
					entry.epochMsec = since_msec;
					log.push_back(entry.toRpcValueList(make_path_shared));
					rec_cnt++;
				}
			}
		}
		// keep <since, until) interval open to make log merge simpler
		{
			auto it = it1;
			for(; it != it2; ++it) {
				if(pm.match(*it)) {
					if(rec_cnt >= rec_cnt_limit) {
						rec_cnt_limit_hit = true;
						goto log_finish;
					}
					if(since_msec == 0)
						since_msec = it->epochMsec;
					last_record_msec = it->epochMsec;

					log.push_back(it->toRpcValueList(make_path_shared));
					rec_cnt++;
				}
			}
		}
	}
log_finish:
	cp::RpcValue ret = log;
	ShvLogHeader hdr;
	{
		hdr.setDeviceId(m_logHeader.deviceId());
		hdr.setDeviceType(m_logHeader.deviceType());
		hdr.setDateTime(cp::RpcValue::DateTime::now());
		hdr.setLogParams(params);

		hdr.setSince((since_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec)): cp::RpcValue(nullptr));
		// if record count < limit and params until is specified and it is > log end, then set until to log end
		if(until_msec == 0 || rec_cnt_limit_hit) {
			until_msec = last_record_msec;
		}

		hdr.setUntil((until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec)): cp::RpcValue(nullptr));
		hdr.setRecordCount(rec_cnt);
		hdr.setRecordCountLimit(rec_cnt_limit);
		hdr.setRecordCountLimitHit(rec_cnt_limit_hit);
		hdr.setWithSnapShot(params.withSnapshot);
		hdr.setWithPathsDict(params.withPathsDict);

		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::Domain)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::ValueFlags)}});
		fields.push_back(cp::RpcValue::Map{{ShvJournalCommon::KEY_NAME, Column::name(Column::Enum::UserId)}});
		hdr.setFields(std::move(fields));

		if(params.withTypeInfo)
			hdr.copyTypeInfo(m_logHeader);
	}
	if(params.withPathsDict) {
		logIShvJournal() << "Generating paths dict";
		cp::RpcValue::IMap path_dict;
		for(const auto &kv : path_cache) {
			logIShvJournal() << "Adding record to paths dict:" << kv.second.toInt() << "-->" << kv.first;
			path_dict[kv.second.toInt()] = kv.first;
		}
		hdr.setPathDict(std::move(path_dict));
	}
	ret.setMetaData(hdr.toMetaData());
	return ret;
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
