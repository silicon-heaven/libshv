#include <shv/core/utils/getlog.h>
#include <shv/core/utils/shvjournalcommon.h>
#include <shv/core/utils/shvfilejournal.h>

#include <shv/core/utils/patternmatcher.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogheader.h>

#include <shv/core/log.h>
#include <shv/core/exception.h>
#include <shv/core/string.h>

#include <shv/chainpack/rpc.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logMShvJournal() shvCMessage("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

using namespace shv::chainpack;

namespace shv::core::utils {
namespace {
void handle_error_code(const std::string_view& func_name, const std::error_code& code)
{
	if (code) {
		logWShvJournal().nospace() << func_name << ": " << code.message() << " (" << std::to_string(code.value()) << ")";
	}
}
}

namespace {
bool is_dir(const std::string &dir_name)
{
	std::error_code code;
	auto ret = std::filesystem::is_directory(dir_name, code);
	handle_error_code(__FUNCTION__, code);
	return ret;
}

bool mkpath(const std::string &dir_name)
{
	std::error_code code;
	auto ret = std::filesystem::create_directories(dir_name, code);
	handle_error_code(__FUNCTION__, code);
	return ret;
}

int64_t file_size(const std::string &file_name)
{
	std::error_code code;
	auto ret =  std::filesystem::file_size(file_name, code);
	handle_error_code(__FUNCTION__, code);
	return ret;
}

int64_t rm_file(const std::string &file_name)
{
	int64_t sz = file_size(file_name);
	std::error_code code;
	auto ret = std::filesystem::remove(file_name, code);
	handle_error_code(__FUNCTION__, code);
	if (ret) {
		return sz;
	}
	return 0;
}

bool path_exists(const std::string &path)
{
	std::error_code code;
	auto ret =  std::filesystem::exists(path, code);
	handle_error_code(__FUNCTION__, code);
	return ret;
}

int64_t str_to_size(const std::string &str)
{
	std::istringstream is(str);
	int64_t n;
	char c;
	is >> n >> c;
	switch(std::toupper(c)) {
	case 'K': n *= 1024; break;
	case 'M': n *= 1024 * 1024; break;
	case 'G': n *= 1024 * 1024 * 1024; break;
	default: break;
	}
	if(n < 1024)
		n = 1024;
	return n;
}
}

const std::string ShvFileJournal::FILE_EXT = ".log2";

ShvFileJournal::ShvFileJournal() = default;

ShvFileJournal::ShvFileJournal(std::string device_id)
{
	setDeviceId(device_id);
}

void ShvFileJournal::setJournalDir(std::string s)
{
	if(s == m_journalContext.journalDir)
		return;
	shvInfo() << "Journal dir set to:" << s;
	m_journalContext.journalDir = std::move(s);
}

const std::string &ShvFileJournal::journalDir()
{
	if(m_journalContext.journalDir.empty()) {
		std::string d = "/tmp/shvjournal/";
		if(m_journalContext.deviceId.empty()) {
			d += "default";
		}
		else {
			std::string id = m_journalContext.deviceId;
			String::replace(id, '/', '-');
			String::replace(id, ':', '-');
			String::replace(id, '.', '-');
			d += id;
		}
		m_journalContext.journalDir = d;
		shvInfo() << "Journal dir not set, falling back to default value:" << journalDir();
	}
	return m_journalContext.journalDir;
}

void ShvFileJournal::setFileSizeLimit(const std::string &n)
{
	setFileSizeLimit(str_to_size(n));
}

void ShvFileJournal::setFileSizeLimit(int64_t n)
{
	m_fileSizeLimit = n;
}

int64_t ShvFileJournal::fileSizeLimit() const
{
	return m_fileSizeLimit;
}

void ShvFileJournal::setJournalSizeLimit(int64_t n)
{
	m_journalSizeLimit = n;
}

int64_t ShvFileJournal::journalSizeLimit() const
{
	return m_journalSizeLimit;
}

void ShvFileJournal::setTypeInfo(const ShvTypeInfo &i)
{
	m_journalContext.typeInfo = i;
}

const ShvTypeInfo& ShvFileJournal::typeInfo() const
{
	return m_journalContext.typeInfo;
}

std::string ShvFileJournal::deviceId() const
{
	return m_journalContext.deviceId;
}

void ShvFileJournal::setDeviceId(std::string id)
{
	m_journalContext.deviceId = std::move(id);
}

std::string ShvFileJournal::deviceType() const
{
	return m_journalContext.deviceType;
}

void ShvFileJournal::setDeviceType(std::string type)
{
	m_journalContext.deviceType = std::move(type);
}

int64_t ShvFileJournal::recentlyWrittenEntryDateTime() const
{
	return m_journalContext.recentTimeStamp;
}


void ShvFileJournal::setJournalSizeLimit(const std::string &n)
{
	setJournalSizeLimit(str_to_size(n));
}

void ShvFileJournal::append(const ShvJournalEntry &entry)
{
	try {
		appendThrow(entry);
		return;
	}
	catch (std::exception &e) {
		logIShvJournal() << "Append to log failed, journal dir will be read again, SD card might be replaced:" << e.what();
	}
	try {
		createJournalDirIfNotExist();
		checkJournalContext_helper(true);
		appendThrow(entry);
	}
	catch (std::exception &e) {
		logWShvJournal() << "Append to log failed after journal dir check:" << e.what();
	}
}

void ShvFileJournal::appendThrow(const ShvJournalEntry &entry)
{
	shvLogFuncFrame();// << "last file no:" << lastFileNo();

	checkJournalContext_helper();
	checkRecentTimeStamp();

	ShvJournalEntry e = entry;
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = RpcValue::DateTime::now().msecsSinceEpoch(); //m_appendLogTSNowFn();
	if(msec < m_journalContext.recentTimeStamp)
		msec = m_journalContext.recentTimeStamp;
	e.epochMsec = msec;
	int64_t journal_file_start_msec = 0;
	if(m_journalContext.files.empty() || m_journalContext.lastFileSize > m_fileSizeLimit) {
		/// create new file
		journal_file_start_msec = msec;
		createNewLogFile(journal_file_start_msec);
	}
	else {
		journal_file_start_msec = m_journalContext.files[m_journalContext.files.size() - 1];
	}
	if(!m_journalContext.files.empty() && journal_file_start_msec < m_journalContext.files[m_journalContext.files.size() - 1])
		SHV_EXCEPTION("Journal context corrupted!");

	addToSnapshot(m_snapshot, e);
	ShvJournalFileWriter wr(journalDir(), journal_file_start_msec, m_journalContext.recentTimeStamp);
	auto orig_fsz = wr.fileSize();
	wr.appendMonotonic(e);
	m_journalContext.recentTimeStamp = wr.recentTimeStamp();
	auto new_fsz = wr.fileSize();
	m_journalContext.lastFileSize = new_fsz;
	m_journalContext.journalSize += new_fsz - orig_fsz;
	if(m_journalContext.journalSize > m_journalSizeLimit) {
		rotateJournal();
	}
}

void ShvFileJournal::createNewLogFile(int64_t journal_file_start_msec)
{
	checkJournalContext();
	if(journal_file_start_msec == 0) {
		journal_file_start_msec = RpcValue::DateTime::now().msecsSinceEpoch();
		if(!m_journalContext.files.empty() && m_journalContext.files[m_journalContext.files.size() - 1] >= journal_file_start_msec)
			SHV_EXCEPTION("Journal context corrupted, new log file is older than last existing one.");
	}
	ShvJournalFileWriter wr(journalDir(), journal_file_start_msec, journal_file_start_msec);
	logMShvJournal() << "New log file:" << wr.fileName() << "created.";
	// new file should start with snapshot
	logDShvJournal() << "Writing snapshot, entries count:" << m_snapshot.keyvals.size();
	wr.appendSnapshot(journal_file_start_msec, m_snapshot.keyvals);
	m_journalContext.files.push_back(journal_file_start_msec);
	m_journalContext.recentTimeStamp = journal_file_start_msec;
}

bool ShvFileJournal::JournalContext::isConsistent() const
{
	return journalDirExists && journalSize >= 0;
}

int64_t ShvFileJournal::JournalContext::fileNameToFileMsec(const std::string &fn)
{
	return ShvJournalFileReader::fileNameToFileMsec(fn);
}

std::string ShvFileJournal::JournalContext::msecToBaseFileName(int64_t msec)
{
	return ShvJournalFileReader::msecToBaseFileName(msec);
}

std::string ShvFileJournal::JournalContext::fileMsecToFileName(int64_t msec)
{
	return msecToBaseFileName(msec) + FILE_EXT;
}

std::string ShvFileJournal::JournalContext::fileMsecToFilePath(int64_t file_msec) const
{
	std::string fn = fileMsecToFileName(file_msec);
	return journalDir + '/' + fn;
}

void ShvFileJournal::checkJournalContext_helper(bool force)
{
	if(!m_journalContext.isConsistent() || force) {
		logMShvJournal() << "journal context not consistent or check forced, check forced:" << force;
		m_journalContext.recentTimeStamp = 0;
		m_journalContext.journalDirExists = journalDirExists();
		if(!m_journalContext.journalDirExists)
			createJournalDirIfNotExist();
		if(m_journalContext.journalDirExists)
			updateJournalStatus();
		else
			shvWarning() << "Journal dir:" << journalDir() << "does not exist!";
		if(m_journalContext.isConsistent())
			logMShvJournal() << "journal context is consistent now";
	}
	if(!m_journalContext.isConsistent()) {
		SHV_EXCEPTION("Journal cannot be brought to consistent state.");
	}
}

void ShvFileJournal::createJournalDirIfNotExist()
{
	if(!mkpath(journalDir())) {
		m_journalContext.journalDirExists = false;
		SHV_EXCEPTION("Journal dir: " + m_journalContext.journalDir + " do not exists and cannot be created");
	}
	m_journalContext.journalDirExists = true;
}

bool ShvFileJournal::journalDirExists()
{
	auto journal_dir = journalDir();
	return path_exists(journal_dir) && is_dir(journal_dir);
}

void ShvFileJournal::rotateJournal()
{
	logMShvJournal() << "Rotating journal of size:" << m_journalContext.journalSize;
	updateJournalStatus();
	size_t file_cnt = m_journalContext.files.size();
	for(int64_t file_msec : m_journalContext.files) {
		if(file_cnt == 1) {
			/// keep at least one file in case of bad limits configuration
			break;
		}
		if(m_journalContext.journalSize < m_journalSizeLimit)
			break;
		std::string fn = m_journalContext.fileMsecToFilePath(file_msec);
		logMShvJournal() << "\t deleting file:" << fn;
		m_journalContext.journalSize -= rm_file(fn);
		file_cnt--;
	}
	updateJournalStatus();
	logMShvJournal() << "New journal of size:" << m_journalContext.journalSize;
}

void ShvFileJournal::convertLog1JournalDir()
{
	const std::string &journal_dir = journalDir();
	std::error_code code;
	auto dir_iter = std::filesystem::directory_iterator(journal_dir, code);
	if (code) {
		shvError() << "Cannot read content of dir:" << journal_dir << " (" << code.value() << ")";
		return;
	}
	const std::string ext = ".log";
	for (const auto& entry : dir_iter) {
		if (!entry.is_regular_file()) {
			continue;
		}
		int n_files = 0;
		std::string fn = entry.path().filename().string();
		if(!fn.ends_with(ext))
			continue;
		if(n_files++ == 0)
			shvInfo() << "======= Journal1 format file(s) found, converting to format 2";
		int n = 0;
		try {
			n = std::stoi(fn.substr(0, fn.size() - ext.size()));
		} catch (std::logic_error &e) {
			shvWarning() << "Malformed shv journal file name" << fn << e.what();
		}
		if(n > 0) {
			fn = journal_dir + '/' + entry.path().filename().string();
			std::ifstream in(fn, std::ios::in | std::ios::binary);
			if (!in) {
				shvWarning() << "Cannot open file:" << fn << "for reading.";
			}
			else {
				static constexpr size_t DT_LEN = 30;
				std::array<char, DT_LEN> buff;
				in.read(buff.data(), buff.size());
				auto char_count = in.gcount();
				if(char_count > 0) {
					std::string s(buff.data(), static_cast<unsigned>(char_count));
					int64_t file_msec = chainpack::RpcValue::DateTime::fromUtcString(s).msecsSinceEpoch();
					if(file_msec == 0) {
						shvWarning() << "cannot read date time from first line of file:" << fn << "line:" << s;
					}
					else {
						std::string new_fn = journal_dir + '/' + JournalContext::fileMsecToFileName(file_msec);
						shvInfo() << "renaming" << fn << "->" << new_fn;
						if (std::rename(fn.c_str(), new_fn.c_str())) {
							shvError() << "cannot rename:" << fn << "to:" << new_fn;
						}
					}
				}
			}
		}
	}
}

#ifdef __unix
#define DIRENT_HAS_TYPE_FIELD
#endif
void ShvFileJournal::updateJournalStatus()
{
	logMShvJournal() << "ShvFileJournal::updateJournalStatus()";
	m_journalContext.journalSize = 0;
	m_journalContext.lastFileSize = 0;
	m_journalContext.files.clear();
	int64_t max_file_msec = -1;
	std::error_code code;
	auto dir_iter = std::filesystem::directory_iterator(m_journalContext.journalDir.c_str(), code);
	if (code) {
		SHV_EXCEPTION("Cannot read content of dir: " + m_journalContext.journalDir + " (" + std::to_string(code.value()) + ")");
		return;
	}
	m_journalContext.journalSize = 0;
	const std::string &ext = FILE_EXT;
	for (const auto& entry : dir_iter) {
		if(!entry.is_regular_file()) {
			continue;
		}
		std::string fn = entry.path().filename().string();
		if(!fn.ends_with(ext))
			continue;
		try {
			int64_t msec = m_journalContext.fileNameToFileMsec(fn);
			m_journalContext.files.push_back(msec);
			fn = m_journalContext.journalDir + '/' + fn;
			int64_t sz = file_size(fn);
			if(msec > max_file_msec) {
				max_file_msec = msec;
				m_journalContext.lastFileSize = sz;
			}
			m_journalContext.journalSize += sz;
		} catch (std::logic_error &e) {
			shvWarning() << "Mallformated shv journal file name" << fn << e.what();
		}
	}
	std::sort(m_journalContext.files.begin(), m_journalContext.files.end());
	logMShvJournal() << "journal dir contains:" << m_journalContext.files.size() << "files";
	if(!m_journalContext.files.empty()) {
		logMShvJournal() << "first file:"
						 << m_journalContext.files[0]
						 << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[0]).toIsoString();
		logMShvJournal() << "last file:"
						 << m_journalContext.files[m_journalContext.files.size()-1]
						 << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[m_journalContext.files.size()-1]).toIsoString();
	}
}

void ShvFileJournal::checkRecentTimeStamp()
{
	if(m_journalContext.recentTimeStamp > 0)
		return;
	logDShvJournal() << "FileShvJournal2::updateRecentTimeStamp()";
	if(m_journalContext.files.empty()) {
		// recentTimeStamp will be set on first appendLog() call
		logMShvJournal() << "journal dir is empty, setting recent timestamp to 0";
		m_journalContext.recentTimeStamp = 0; //RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else {
		auto last_file_start_msec = m_journalContext.files[m_journalContext.files.size() - 1];
		std::string fn = m_journalContext.fileMsecToFilePath(last_file_start_msec);
		m_journalContext.recentTimeStamp = findLastEntryDateTime(fn, last_file_start_msec);
		logMShvJournal() << "setting recent timestamp to last entry in:" << fn
						 << "to:" << m_journalContext.recentTimeStamp << "epoch msec"
						 << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.recentTimeStamp).toIsoString();
		if(m_journalContext.recentTimeStamp < 0) {
			logWShvJournal() << "corrupted log file:" << fn;
			m_journalContext.recentTimeStamp = 0;
		}
	}
}

int64_t ShvFileJournal::findLastEntryDateTime(const std::string &fn, int64_t journal_start_msec, std::ifstream::pos_type *p_date_time_fpos)
{
	shvLogFuncFrame() << "'" + fn + "'";
	std::ifstream::pos_type date_time_fpos = -1;
	if(p_date_time_fpos)
		*p_date_time_fpos = date_time_fpos;
	std::ifstream in(fn, std::ios::in | std::ios::binary);
	if (!in)
		SHV_EXCEPTION("Cannot open file: " + fn + " for reading.");
	int64_t dt_msec = -1;
	in.seekg(0, std::ios::end);
	auto fpos = in.tellg();
	if(fpos == 0) {
		// empty file
		return journal_start_msec;
	}
	static constexpr int TS_LEN = 30;
	static constexpr int CHUNK_LEN = 512;
	std::array<char, CHUNK_LEN + TS_LEN> buff;
	logDShvJournal() << "------------------findLastEntryDateTime-----------------------------" << fn;
	while(fpos > 0) {
		in.seekg(-CHUNK_LEN, std::ios::cur);
		fpos = in.tellg();
		if(fpos < 0) {
			in.clear();
			in.seekg(0);
			fpos = 0;
		}
		// date time string can be partialy on end of this chunk and at beggining of next,
		// read little bit more data to cover this
		// serialized date-time should never exceed 28 bytes see: 2018-01-10T12:03:56.123+0130
		in.read(buff.data(), buff.size());
		auto n = in.gcount();
		if(in.eof()) {
			in.clear();
			in.seekg(-n, std::ios::end);
		}
		else {
			in.seekg(-n, std::ios::cur);
		}
		if(n == 0)
			break;

		std::string chunk(buff.data(), static_cast<size_t>(n));
		logDShvJournal() << "fpos:" << fpos << "chunk:" << chunk;
		size_t line_start_pos = 0;

		auto pos = chunk.length() - 1;
		// remove trailing blanks, like trailing '\n' in log file
		for(; pos > 0; --pos)
			if(!(chunk[pos] == '\n' || chunk[pos] == '\t' || chunk[pos] == ' '))
				break;
		line_start_pos = chunk.rfind(RECORD_SEPARATOR, pos);
		if(line_start_pos != std::string::npos)
			line_start_pos++;

		if(line_start_pos != std::string::npos) {
			auto tab_pos = chunk.find(FIELD_SEPARATOR, line_start_pos);
			if(tab_pos != std::string::npos) {
				std::string s = chunk.substr(line_start_pos, tab_pos - line_start_pos);
				logDShvJournal() << "\t checking:" << s;
				size_t len;
				chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s, &len);
				if(len > 0) {
					date_time_fpos = fpos + static_cast<decltype(fpos)>(line_start_pos);
					dt_msec = dt.msecsSinceEpoch();
				}
				else {
					logWShvJournal() << fn << "Malformed shv journal date time:" << s << "will be ignored.";
				}
			}
			else if(chunk.size() > line_start_pos) {
				logWShvJournal() << fn << "Truncated shv journal date time:" << chunk.substr(line_start_pos) << "will be ignored.";
			}
		}

		if(dt_msec > 0) {
			logDShvJournal() << "\t return:" << dt_msec << chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(dt_msec).toIsoString();
			if(p_date_time_fpos)
				*p_date_time_fpos = date_time_fpos;
			return dt_msec;
		}
	}
	logWShvJournal() << fn << "File does not contain record with valid date time";
	return -1;
}

const ShvFileJournal::JournalContext &ShvFileJournal::checkJournalContext(bool force)
{
	if(!force) try {
		checkJournalContext_helper(false);
		return m_journalContext;
	}
	catch (std::exception &e) {
		logIShvJournal() << "Check journal consistecy failed, journal dir will be read again, SD card might be replaced, error:" << e.what();
	}
	checkJournalContext_helper(true);
	return m_journalContext;
}

chainpack::RpcValue ShvFileJournal::getLog(const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	return ShvFileJournal::getLog(checkJournalContext(), params, ignore_record_count_limit);
}

chainpack::RpcValue ShvFileJournal::getLog(const JournalContext &journal_context, const ShvGetLogParams &params, IgnoreRecordCountLimit ignore_record_count_limit)
{
	std::vector<std::function<ShvJournalFileReader()>> readers;
	{
		std::vector<int64_t> non_empty_files;
		std::copy_if(journal_context.files.begin(), journal_context.files.end(), std::back_inserter(non_empty_files), [&journal_context] (const auto& ms)  {return file_size(journal_context.fileMsecToFilePath(ms)) > 0;});
		for (auto it = shv::core::utils::newestMatchingFileIt(non_empty_files, params); it != non_empty_files.cend(); ++it) {
			readers.emplace_back([full_file_name = journal_context.fileMsecToFilePath(*it)] {
				return shv::core::utils::ShvJournalFileReader(full_file_name);
			});
		}
	}
	return shv::core::utils::getLog(readers, params, shv::chainpack::RpcValue::DateTime::now(), ignore_record_count_limit);
}

chainpack::RpcValue ShvFileJournal::getSnapShotMap()
{
	std::vector<ShvJournalEntry> snapshot;
	RpcValue::Map m;
	for(const auto &kv : m_snapshot.keyvals) {
		const ShvJournalEntry &e = kv.second;
		if(e.value.isValid())
			m[e.path] = e.value;
	}
	return m;
}

const char *ShvFileJournal::TxtColumn::name(ShvFileJournal::TxtColumn::Enum e)
{
	switch (e) {
	case TxtColumn::Enum::Timestamp: return ShvLogHeader::Column::name(ShvLogHeader::Column::Timestamp);
	case TxtColumn::Enum::UpTime: return "upTime";
	case TxtColumn::Enum::Path: return ShvLogHeader::Column::name(ShvLogHeader::Column::Path);
	case TxtColumn::Enum::Value: return ShvLogHeader::Column::name(ShvLogHeader::Column::Value);
	case TxtColumn::Enum::ShortTime: return ShvLogHeader::Column::name(ShvLogHeader::Column::ShortTime);
	case TxtColumn::Enum::Domain: return ShvLogHeader::Column::name(ShvLogHeader::Column::Domain);
	case TxtColumn::Enum::ValueFlags: return ShvLogHeader::Column::name(ShvLogHeader::Column::ValueFlags);
	case TxtColumn::Enum::UserId: return ShvLogHeader::Column::name(ShvLogHeader::Column::UserId);
	}
	return "invalid";
}

} // namespace shv
