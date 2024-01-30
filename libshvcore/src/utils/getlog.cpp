#include <shv/core/log.h>
#include <shv/core/utils/abstractshvjournal.h>
#include <shv/core/utils/getlog.h>
#include <shv/core/utils/patternmatcher.h>
#include <shv/core/utils/shvlogheader.h>

#include <concepts>
#include <functional>
#include <optional>

#define logWGetLog() shvCWarning("GetLog")
#define logIGetLog() shvCInfo("GetLog")
#define logMGetLog() shvCMessage("GetLog")
#define logDGetLog() shvCDebug("GetLog")

namespace shv::core::utils {
using chainpack::RpcValue;

namespace {
[[nodiscard]] RpcValue as_shared_path(chainpack::RpcValue::Map& path_map, const std::string &path)
{
	RpcValue ret;
	if (auto it = path_map.find(path); it == path_map.end()) {
		ret = static_cast<int>(path_map.size());
		logDGetLog() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
		path_map[path] = ret;
	} else {
		ret = it->second;
	}
	return ret;
}

struct GetLogContext {
	RpcValue::Map pathCache;
	ShvGetLogParams params;
};

enum class Status {
	Ok,
	RecordCountLimitHit
};

void append_log_entry(RpcValue::List& log, const ShvJournalEntry &e, GetLogContext& ctx)
{
	logDGetLog() << "\t append_log_entry:" << e.toRpcValueMap().toCpon();
	std::function<shv::chainpack::RpcValue(const std::string&)> map_path;
	if (ctx.params.withPathsDict) {
		map_path = [&ctx] (const auto& path) {
			return as_shared_path(ctx.pathCache, path);
		};
	}
	log.push_back(e.toRpcValueList(map_path));
}

auto snapshot_to_entries(const ShvSnapshot& snapshot, const bool since_last, const int64_t params_since_msec, GetLogContext& ctx)
{
	RpcValue::List res;
	logMGetLog() << "\t writing snapshot, record count:" << snapshot.keyvals.size();
	if (!snapshot.keyvals.empty()) {
		auto since_res = params_since_msec;

		if (since_last) {
			for (const auto& kv : snapshot.keyvals) {
				since_res = std::max(kv.second.epochMsec, since_res);
			}
		}
		for (const auto& kv : snapshot.keyvals) {
			ShvJournalEntry e = kv.second;
			e.epochMsec = since_res;
			e.setSnapshotValue(true);
			// erase EVENT flag in the snapshot values,
			// they can trigger events during reply otherwise
			e.setSpontaneous(false);
			logDGetLog() << "\t writing SNAPSHOT entry:" << e.toRpcValueMap().toCpon();
			append_log_entry(res, e, ctx);
		}
	}

	return res;
}
}

namespace {
class ShvLogVectorReader {
public:
	ShvLogVectorReader(const std::vector<ShvJournalEntry>& entries)
		: m_entries(entries)
	{
	}
	bool next()
	{
		if (!m_entriesIter.has_value()) {
			m_entriesIter = m_entries.begin();
		} else {
			m_entriesIter.value()++;
		}

		if (m_entriesIter.value() == m_entries.end()) {
			return false;
		}

		return true;
	}
	const ShvJournalEntry& entry() const
	{
		if (!m_entriesIter.has_value()) {
			throw std::logic_error{"ShvLogVectorReader: entry() called before next()"};
		}

		return *(m_entriesIter.value());
	}

private:
	const std::vector<ShvJournalEntry>& m_entries;
	std::optional<std::vector<ShvJournalEntry>::const_iterator> m_entriesIter;
};

template <typename Type>
concept LogReader = requires(Type x)
{
	{ x.next() } -> std::same_as<bool>;
	{ x.entry() } -> std::same_as<const ShvJournalEntry&>;
};

template <LogReader Type>
[[nodiscard]] chainpack::RpcValue impl_get_log(const std::vector<std::function<Type()>>& readers, const ShvGetLogParams& orig_params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	logIGetLog() << "========================= getLog ==================";
	logIGetLog() << "params:" << orig_params.toRpcValue().toCpon();
	GetLogContext ctx;
	ctx.params = orig_params;

	// Asking for a snapshot without supplying `since` is invalid. We'll continue as if the user didn't want a snapshot.
	if (ctx.params.withSnapshot && !ctx.params.since.isValid()) {
		logWGetLog() << "Asking for a snapshot without `since` is invalid. No snapshot will be created.";
		ctx.params.withSnapshot = false;
	}

	ShvSnapshot snapshot;
	RpcValue::List result_log;
	PatternMatcher pattern_matcher(ctx.params);

	ShvLogHeader log_header;
	auto record_count_limit =
		ignore_record_count_limit == IgnoreRecordCountLimit::Yes ? std::numeric_limits<decltype(ctx.params.recordCountLimit)>::max()
		: std::min(ctx.params.recordCountLimit, ShvJournalCommon::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);

	log_header.setRecordCountLimit(record_count_limit);
	log_header.setWithSnapShot(ctx.params.withSnapshot);
	log_header.setWithPathsDict(ctx.params.withPathsDict);

	const auto params_since_msec =
		ctx.params.since.isDateTime() ? ctx.params.since.toDateTime().msecsSinceEpoch() : 0;

	const auto params_until_msec =
		ctx.params.until.isDateTime() ? ctx.params.until.toDateTime().msecsSinceEpoch() : std::numeric_limits<int64_t>::max();

	std::optional<ShvJournalEntry> last_entry;
	std::optional<ShvJournalEntry> first_unmatching_entry;

	int record_count = 0;
	bool have_data_before_since_param = false;

	for (const auto& readerFn : readers) {
		auto reader = readerFn();
		while(reader.next()) {
			const auto& entry = reader.entry();

			if (!pattern_matcher.match(entry)) {
				logDGetLog() << "\t SKIPPING:" << entry.path << "because it doesn't match" << ctx.params.pathPattern;
				continue;
			}

			if (ctx.params.isSinceLast() || entry.epochMsec < params_since_msec) {
				logDGetLog() << "\t saving SNAPSHOT entry:" << entry.toRpcValueMap().toCpon();
				AbstractShvJournal::addToSnapshot(snapshot, entry);
			}

			if (entry.epochMsec >= params_until_msec) {
				first_unmatching_entry = entry;
				goto exit_nested_loop;
			}

			if (entry.epochMsec >= params_since_msec && !ctx.params.isSinceLast()) {
				if ((static_cast<int>(snapshot.keyvals.size()) + record_count + 1) > ctx.params.recordCountLimit) {
					log_header.setRecordCountLimitHit(true);
					if (last_entry.has_value() && entry.dateTime() != last_entry->dateTime()) {
						first_unmatching_entry = entry;
						goto exit_nested_loop;
					}
				}

				append_log_entry(result_log, entry, ctx);
				record_count++;
			} else {
				have_data_before_since_param = true;
			}
			last_entry = entry;
		}
	}
exit_nested_loop:

	auto snapshot_entries = [&] {
		if (ctx.params.withSnapshot) {
			return snapshot_to_entries(snapshot, ctx.params.isSinceLast(), params_since_msec, ctx);
		}

		if (last_entry && ctx.params.isSinceLast()) {
			RpcValue::List res;
			append_log_entry(res, *last_entry, ctx);
			return res;
		}
		return RpcValue::List{};
	}();

	RpcValue::List result_entries = snapshot_entries;
	std::copy(result_log.begin(), result_log.end(), std::back_inserter(result_entries));

	if (ctx.params.withPathsDict) {
		logMGetLog() << "Generating paths dict size:" << ctx.pathCache.size();
		log_header.setPathDict([&] {
			RpcValue::IMap path_dict;
			for(const auto &kv : ctx.pathCache) {
				path_dict[kv.second.toInt()] = kv.first;
			}
			return path_dict;
		}());
	}

	auto unmap_path = [&log_header, &ctx] (const RpcValue& path) {
		if (ctx.params.withPathsDict) {
			return log_header.pathDictCRef().at(path.toInt()).asString();
		}

		return path.asString();
	};

	// If there isn't an unmatched entry, and we don't have any result log entries, we'll use a snapshot entry as the
	// last unmatched entry. This means that result until will be the same as result since, but that's fine - it's ok if
	// the caller calls getLog again with the same since, because there were no data to give him anyway.
	if (!first_unmatching_entry.has_value() && result_log.empty() && !snapshot_entries.empty()) {
		first_unmatching_entry = ShvJournalEntry::fromRpcValueList(result_entries.back().asList(), unmap_path);
	}

	// If there isn't an unmatched entry, we can't be sure we have all the entries from the last ms in result_entries.
	// We must remove all of them and they'll become the first_unmatching_entry.
	if (!ctx.params.isSinceLast() && !result_entries.empty() && !first_unmatching_entry.has_value()) {
		first_unmatching_entry = ShvJournalEntry::fromRpcValueList(result_entries.back().asList(), unmap_path);
		std::erase_if(result_entries, [compare_with = first_unmatching_entry.value().dateTime(), &unmap_path] (const RpcValue& entry) {
			return ShvJournalEntry::fromRpcValueList(entry.asList(), unmap_path).dateTime() == compare_with;
		});
	}

	if (result_entries.empty()) {
		log_header.setSince(ctx.params.since.isValid() ? ctx.params.since : ctx.params.until);
		log_header.setUntil(ctx.params.until.isValid() ? ctx.params.until : ctx.params.since);
	} else if (ctx.params.isSinceLast()) {
		auto first_entry = result_entries.front().asList().value(ShvLogHeader::Column::Timestamp);
		log_header.setSince(first_entry);
		log_header.setUntil(first_entry);
	} else {
		log_header.setSince(have_data_before_since_param ? ctx.params.since : result_entries.front().asList().value(ShvLogHeader::Column::Timestamp));
		log_header.setUntil([&first_unmatching_entry] {
			return first_unmatching_entry.value().dateTime();
		}());
	}

	logDGetLog() << "result since:" << log_header.sinceCRef().toCpon() << "result until:" << log_header.untilCRef().toCpon();

	log_header.setDateTime(RpcValue::DateTime::now());
	log_header.setLogParams(orig_params);
	log_header.setRecordCount(static_cast<int>(result_entries.size()));

	auto rpc_value_result = RpcValue{result_entries};
	rpc_value_result.setMetaData(log_header.toMetaData());

	return rpc_value_result;
}
}

[[nodiscard]] chainpack::RpcValue getLog(const std::vector<std::function<ShvJournalFileReader()>>& readers, const ShvGetLogParams& params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	return impl_get_log(readers, params, ignore_record_count_limit);
}

[[nodiscard]] chainpack::RpcValue getLog(const std::vector<std::function<ShvLogRpcValueReader()>>& readers, const ShvGetLogParams& params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	return impl_get_log(readers, params, ignore_record_count_limit);
}

[[nodiscard]] chainpack::RpcValue getLog(const std::vector<ShvJournalEntry>& entries, const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	std::vector<std::function<ShvLogVectorReader()>> readers;
	readers.emplace_back([&entries] { return ShvLogVectorReader(entries); });
	return impl_get_log(readers, params, ignore_record_count_limit);
}
}
