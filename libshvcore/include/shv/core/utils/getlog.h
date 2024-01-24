#pragma once
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/abstractshvjournal.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>


namespace shv::core::utils {
[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<std::function<ShvJournalFileReader()>>& readers, const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<std::function<ShvLogRpcValueReader()>>& readers, const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
[[nodiscard]] chainpack::RpcValue SHVCORE_DECL_EXPORT getLog(const std::vector<ShvJournalEntry>& entries, const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit = IgnoreRecordCountLimit::No);
}


