#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvfilejournal.h>

#include <shv/core/exception.h>
#include <shv/core/log.h>

#include <shv/chainpack/rpc.h>

#include <algorithm>

#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv::core::utils {
ShvJournalFileWriter::ShvJournalFileWriter(const std::string &file_name)
	: m_fileName(file_name)
{
	open();
}

ShvJournalFileWriter::ShvJournalFileWriter(const std::string &journal_dir, int64_t journal_start_time, int64_t last_entry_ts)
	: m_fileName(journal_dir + '/' + ShvFileJournal::JournalContext::fileMsecToFileName(journal_start_time))
	, m_recentTimeStamp(last_entry_ts)
{
	open();
}

ShvJournalFileWriter::ShvJournalFileWriter(std::ostream &out)
{
	m_out = &out;
}

void ShvJournalFileWriter::open()
{
	m_fileOut.open(m_fileName, std::ios::binary | std::ios::out | std::ios::app);
	if(!m_fileOut)
		SHV_EXCEPTION("Cannot open file " + m_fileName + " for writing");
	m_out = &m_fileOut;
}

std::ofstream::pos_type ShvJournalFileWriter::fileSize()
{
	return m_out->tellp();
}

const std::string& ShvJournalFileWriter::fileName() const
{
	return m_fileName;
}

int64_t ShvJournalFileWriter::recentTimeStamp() const
{
	return m_recentTimeStamp;
}

void ShvJournalFileWriter::append(const ShvJournalEntry &entry)
{
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	m_recentTimeStamp = msec;
	append(msec, msec, entry);
}

void ShvJournalFileWriter::appendMonotonic(const ShvJournalEntry &entry)
{

	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	int64_t msec2 = msec;
	if(m_recentTimeStamp > 0) {
		msec = std::max(msec, m_recentTimeStamp);
	}
	else {
		m_recentTimeStamp = msec;
	}
	append(msec, msec2, entry);
}

void ShvJournalFileWriter::appendSnapshot(int64_t msec, const std::vector<ShvJournalEntry> &snapshot)
{
	for(ShvJournalEntry e : snapshot) {
		e.setSnapshotValue(true);
		// erase EVENT flag in the snapshot values,
		// they can trigger events during reply otherwise
		e.setSpontaneous(false);
		append(msec, msec, e);
	}
	m_recentTimeStamp = msec;
}

void ShvJournalFileWriter::appendSnapshot(int64_t msec, const std::map<std::string, ShvJournalEntry> &snapshot)
{
	for(const auto &kv : snapshot) {
		ShvJournalEntry e = kv.second;
		e.setSnapshotValue(true);
		// erase EVENT flag in the snapshot values,
		// they can trigger events during reply otherwise
		e.setSpontaneous(false);
		append(msec, msec, e);
	}
	m_recentTimeStamp = msec;
}

void ShvJournalFileWriter::append(int64_t msec, int64_t orig_time, const ShvJournalEntry &entry)
{
	logDShvJournal() << "ShvJournalFileWriter::append:" << entry.toRpcValue().toCpon();
	*m_out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	if(orig_time != msec)
		*m_out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(orig_time).toIsoString();
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	*m_out << entry.path;
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	*m_out << entry.value.toCpon();
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	if(entry.shortTime >= 0)
		*m_out << entry.shortTime;
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	*m_out << entry.domain;
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	*m_out << static_cast<int>(entry.valueFlags);
	*m_out << ShvFileJournal::FIELD_SEPARATOR;
	*m_out << entry.userId;
	*m_out << ShvFileJournal::RECORD_SEPARATOR;
	m_out->flush();
	m_recentTimeStamp = msec;
}

} // namespace shv
