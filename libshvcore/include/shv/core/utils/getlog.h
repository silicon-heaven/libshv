#pragma once
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/abstractshvjournal.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>


namespace shv::core::utils {
std::vector<int64_t>::const_iterator SHVCORE_DECL_EXPORT newestMatchingFileIt(const std::vector<int64_t>& files, const ShvGetLogParams& params);

[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<std::function<ShvJournalFileReader()>>& readers, const ShvGetLogParams &params, const shv::chainpack::RpcValue::DateTime& now, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<std::function<ShvLogRpcValueReader()>>& readers, const ShvGetLogParams &params, const shv::chainpack::RpcValue::DateTime& now, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<ShvJournalEntry>& entries, const ShvGetLogParams &params, const shv::chainpack::RpcValue::DateTime& now, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
}


