#include <shv/core/utils/shvlogrpcvaluereader.h>

#include <shv/core/exception.h>
#include <shv/core/log.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv::core::utils {

ShvLogRpcValueReader::ShvLogRpcValueReader(const shv::chainpack::RpcValue &log, bool throw_exceptions)
	: m_log(log)
	, m_isThrowExceptions(throw_exceptions)
{
	m_logHeader = ShvLogHeader::fromMetaData(m_log.metaData());
	if(m_logHeader.withSnapShot())
		m_snapshotMsec = m_logHeader.sinceMsec();

	if(!m_log.isList() && m_isThrowExceptions)
		SHV_EXCEPTION("Log is corrupted!");
}

bool ShvLogRpcValueReader::next()
{
	auto unmap_path = [this](const cp::RpcValue &p) {
		if(p.isInt())
			return m_logHeader.pathDictCRef().value(p.toInt()).asString();
		return p.asString();
	};
	while(true) {
		m_currentEntry = {};
		const chainpack::RpcValue::List &list = m_log.asList();
		if(m_currentIndex >= list.size())
			return false;

		const cp::RpcValue &val = list[m_currentIndex++];
		const chainpack::RpcValue::List &row = val.asList();
		std::string err;
		m_currentEntry = ShvJournalEntry::fromRpcValueList(row, unmap_path, &err);
		if(!err.empty()) {
			if(m_isThrowExceptions)
				throw shv::core::Exception(err);

			logWShvJournal() << err;
			continue;
		}
		return true;
	}
}

bool ShvLogRpcValueReader::isInSnapshot() const
{
	return m_snapshotMsec == m_currentEntry.epochMsec || m_currentEntry.isSnapshotValue();
}

const ShvJournalEntry& ShvLogRpcValueReader::entry()
{
	return m_currentEntry;
}

const ShvLogHeader &ShvLogRpcValueReader::logHeader() const
{
	return m_logHeader;
}

} // namespace shv
