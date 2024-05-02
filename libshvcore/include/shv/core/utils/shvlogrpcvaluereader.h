#pragma once

#include "shvlogheader.h"
#include "shvjournalentry.h"

namespace shv::core::utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvLogRpcValueReader
{
public:
	ShvLogRpcValueReader(const shv::chainpack::RpcValue &log, bool throw_exceptions = false);

	bool next();
	bool isInSnapshot() const;
	const ShvJournalEntry& entry();

	const ShvLogHeader &logHeader() const;
private:
	ShvLogHeader m_logHeader;
	ShvJournalEntry m_currentEntry;

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
	const shv::chainpack::RpcValue m_log;
	bool m_isThrowExceptions;
	size_t m_currentIndex = 0;
	int64_t m_snapshotMsec = -1;
};

} // namespace shv::core::utils
