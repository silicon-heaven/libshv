#include "shvfilejournal.h"

#include "patternmatcher.h"
#include "shvjournalfilewriter.h"
#include "shvjournalfilereader.h"
#include "shvlogheader.h"
#include "shvpath.h"

#include "../log.h"
#include "../exception.h"
#include "../string.h"
#include "../stringview.h"

#include <shv/chainpack/rpc.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logMShvJournal() shvCMessage("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

/*
static uint16_t shortTime()
{
	std::chrono::time_point<std::chrono::system_clock> p1 = std::chrono::system_clock::now();
	int64_t msecs = std::chrono::duration_cast<std::chrono:: milliseconds>(p1.time_since_epoch()).count();
	return static_cast<uint16_t>(msecs % 65536);
}
*/
#define SHV_STATBUF              struct stat64
#define SHV_STAT                 ::stat64
#if defined(__unix) || defined(__APPLE__)
#define SHV_MKDIR(dir_name)      ::mkdir(dir_name, 0777)
#else
#define SHV_MKDIR(dir_name)      ::mkdir(dir_name)
#endif
#define SHV_REMOVE_FILE          ::remove

static bool is_dir(const std::string &dir_name)
{
	SHV_STATBUF st;
	return SHV_STAT(dir_name.data(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

static bool mkpath(const std::string &dir_name)
{
	// helper function to check if a given path is a directory, since mkdir can
	// fail if the dir already exists (it may have been created by another
	// thread or another process)
	shvDebug() << dir_name << "exists:" << is_dir(dir_name);
	if(is_dir(dir_name))
		return true;
	logIShvJournal() << "Creating journal dir:" << dir_name;
	if (SHV_MKDIR(dir_name.data()) == 0)
		return true;
	if (errno == EEXIST)
		return is_dir(dir_name);
	if (errno != ENOENT)
		return false;
	// mkdir failed because the parent dir doesn't exist, so try to create it
	auto const slash = dir_name.find_last_of('/');

	if (slash == std::string::npos)
		return false;
	std::string parent_dir_name = dir_name.substr(0, slash);
	if (!mkpath(parent_dir_name))
		return false;
	// try again
	if (SHV_MKDIR(dir_name.data()) == 0)
		return true;
	return errno == EEXIST && is_dir(dir_name);
}

static int64_t file_size(const std::string &file_name)
{
	SHV_STATBUF st;
	if(SHV_STAT(file_name.data(), &st) == 0)
		return st.st_size;
	logWShvJournal() << "Cannot stat file:" << file_name;
	return -1;
}
/*
static bool file_exists(const std::string &file_name)
{
	SHV_STATBUF st;
	return (SHV_STAT(file_name.data(), &st) == 0);
}
*/
static int64_t rm_file(const std::string &file_name)
{
	int64_t sz = file_size(file_name);
	if(SHV_REMOVE_FILE(file_name.c_str()) == 0)
		return sz;
	logWShvJournal() << "Cannot delete file:" << file_name;
	return 0;
}

static int64_t str_to_size(const std::string &str)
{
	std::istringstream is(str);
	int64_t n;
	char c;
	is >> n >> c;
	switch(std::toupper(c)) {
	case 'K': n *= 1024; break;
	case 'M': n *= 1024 * 1024; break;
	case 'G': n *= 1024 * 1024 * 1024; break;
	}
	if(n < 1024)
		n = 1024;
	return n;
}

const std::string ShvFileJournal::FILE_EXT = ".log2";

ShvFileJournal::ShvFileJournal(std::string device_id)
	//, m_appendLogTSNowFn([]() {return RpcValue::DateTime::now().msecsSinceEpoch();})
{
	//setDefaultAppendLogTSNowFn();
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

	if(addToSnapshot(m_snapshot, e)) {
		// log only not-default changes or changes from not-default to default
		ShvJournalFileWriter wr(journalDir(), journal_file_start_msec, m_journalContext.recentTimeStamp);
		ssize_t orig_fsz = wr.fileSize();
		wr.appendMonotonic(e);
		m_journalContext.recentTimeStamp = wr.recentTimeStamp();
		ssize_t new_fsz = wr.fileSize();
		m_journalContext.lastFileSize = new_fsz;
		m_journalContext.journalSize += new_fsz - orig_fsz;
		if(m_journalContext.journalSize > m_journalSizeLimit) {
			rotateJournal();
		}
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
	//logDShvJournal() << "journal dir:" << m_journalStatus.journalDir << "exists";
	m_journalContext.journalDirExists = true;
}

bool ShvFileJournal::journalDirExists()
{
	return is_dir(journalDir());
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
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (journal_dir.c_str())) != nullptr) {
		const std::string ext = ".log";
		int n_files = 0;
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
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
					fn = journal_dir + '/' + ent->d_name;
					std::ifstream in(fn, std::ios::in | std::ios::binary);
					if (!in) {
						shvWarning() << "Cannot open file:" << fn << "for reading.";
					}
					else {
						static constexpr size_t DT_LEN = 30;
						char buff[DT_LEN];
						in.read(buff, sizeof (buff));
						auto n = in.gcount();
						if(n > 0) {
							std::string s(buff, (unsigned)n);
							int64_t file_msec = chainpack::RpcValue::DateTime::fromUtcString(s).msecsSinceEpoch();
							if(file_msec == 0) {
								shvWarning() << "cannot read date time from first line of file:" << fn << "line:" << s;
							}
							else {
								std::string new_fn = journal_dir + '/' + m_journalContext.fileMsecToFileName(file_msec);
								shvInfo() << "renaming" << fn << "->" << new_fn;
								if (std::rename(fn.c_str(), new_fn.c_str())) {
									shvError() << "cannot rename:" << fn << "to:" << new_fn;
								}
							}
						}
					}
				}
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
	}
	else {
		shvError() << "Cannot read content of dir:" << journal_dir;
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
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalContext.journalDir.c_str())) != nullptr) {
		m_journalContext.journalSize = 0;
		const std::string &ext = FILE_EXT;
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
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
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
		std::sort(m_journalContext.files.begin(), m_journalContext.files.end());
		logMShvJournal() << "journal dir contains:" << m_journalContext.files.size() << "files";
		if(m_journalContext.files.size()) {
			logMShvJournal() << "first file:"
							 << m_journalContext.files[0]
							 << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[0]).toIsoString();
			logMShvJournal() << "last file:"
							 << m_journalContext.files[m_journalContext.files.size()-1]
							 << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[m_journalContext.files.size()-1]).toIsoString();
		}
	}
	else {
		SHV_EXCEPTION("Cannot read content of dir: " + m_journalContext.journalDir);
	}
}
/*
int64_t ShvFileJournal::lastEntryTimeStamp()
{
	if(m_journalContext.files.empty())
		return 0;
	std::string fn = m_journalContext.fileMsecToFilePath(m_journalContext.files[m_journalContext.files.size() - 1]);
	return findLastEntryDateTime(fn);
}
*/
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
	//logDShvJournal() << "update recent time stamp:" << m_journalContext.recentTimeStamp << RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.recentTimeStamp).toIsoString();
}

int64_t ShvFileJournal::findLastEntryDateTime(const std::string &fn, int64_t journal_start_msec, ssize_t *p_date_time_fpos)
{
	shvLogFuncFrame() << "'" + fn + "'";
	ssize_t date_time_fpos = -1;
	if(p_date_time_fpos)
		*p_date_time_fpos = date_time_fpos;
	std::ifstream in(fn, std::ios::in | std::ios::binary);
	if (!in)
		SHV_EXCEPTION("Cannot open file: " + fn + " for reading.");
	int64_t dt_msec = -1;
	in.seekg(0, std::ios::end);
	long fpos = in.tellg();
	if(fpos == 0) {
		// empty file
		return journal_start_msec;
	}
	static constexpr int TS_LEN = 30;
	static constexpr int CHUNK_LEN = 512;
	char buff[CHUNK_LEN + TS_LEN];
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
		in.read(buff, sizeof (buff));
		auto n = in.gcount();
		if(in.eof()) {
			in.clear();
			in.seekg(-n, std::ios::end);
		}
		else {
			in.seekg(-n, std::ios::cur);
		}
		//shvInfo() << "fpos:" << fpos << "n:" << n << in.tellg();
		if(n == 0)
			break;

		std::string chunk(buff, static_cast<size_t>(n));
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
					date_time_fpos = fpos + (ssize_t)line_start_pos;
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

chainpack::RpcValue ShvFileJournal::getLog(const ShvGetLogParams &params)
{
	JournalContext ctx = checkJournalContext();
	return getLog(ctx, params);
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

chainpack::RpcValue ShvFileJournal::getLog(const ShvFileJournal::JournalContext &journal_context, const ShvGetLogParams &params)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();

	struct {
		ShvSnapshot snapshot;
		bool snapshotWritten;
		int64_t snapshotMsec = 0;
	} snapshot_ctx;
	snapshot_ctx.snapshotWritten = !params.withSnapshot;

	RpcValue::List log;
	bool since_last = params.isSinceLast();

	RpcValue::Map path_cache;
	const auto params_since_msec = params.since.isDateTime()
								   ? params.since.toDateTime().msecsSinceEpoch()
								   : (since_last ? std::numeric_limits<int64_t>::max() : 0);
	const auto params_until_msec = params.until.isDateTime()? params.until.toDateTime().msecsSinceEpoch(): 0;
	int64_t journal_start_msec = 0;
	int64_t first_record_msec = 0;
	int64_t last_record_msec = 0;
	int rec_cnt_limit = std::min(params.recordCountLimit, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
	bool rec_cnt_limit_hit = false;

	/// this ensure that there be only one copy of each path in memory
	int max_path_id = 0;
	auto make_path_shared = [&path_cache, &max_path_id, &params](const std::string &path) -> RpcValue {
		if(!params.withPathsDict)
			return path;
		RpcValue ret;
		if(auto it = path_cache.find(path); it == path_cache.end()) {
			ret = ++max_path_id;
			logDShvJournal() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
			path_cache[path] = ret;
		}
		else {
			ret = it->second;
		}
		return ret;
	};
	auto append_log_entry = [make_path_shared, rec_cnt_limit, &rec_cnt_limit_hit, &first_record_msec, &last_record_msec, &log](const ShvJournalEntry &e) {
		if((int)log.size() >= rec_cnt_limit) {
			rec_cnt_limit_hit = true;
			return false;
		}
		if(first_record_msec == 0)
			first_record_msec = e.epochMsec;
		last_record_msec = e.epochMsec;
		RpcValue::List rec;
		rec.push_back(e.dateTime());
		rec.push_back(make_path_shared(e.path));
		rec.push_back(e.value);
		rec.push_back(e.shortTime == ShvJournalEntry::NO_SHORT_TIME? RpcValue(nullptr): RpcValue(e.shortTime));
		rec.push_back((e.domain.empty() || e.domain == ShvJournalEntry::DOMAIN_VAL_CHANGE)? RpcValue(nullptr): e.domain);
		rec.push_back(e.valueFlags);
		rec.push_back(e.userId.empty()? RpcValue(nullptr): RpcValue(e.userId));
		log.push_back(std::move(rec));
		return true;
	};
	auto write_snapshot = [append_log_entry, since_last, params_since_msec, &snapshot_ctx]() {
		//shvWarning() << "write_snapshot, snapshot size:" << snapshot_ctx.snapshot.size();
		logMShvJournal() << "\t writing snapshot, record count:" << snapshot_ctx.snapshot.keyvals.size();
		snapshot_ctx.snapshotWritten = true;
		if(!snapshot_ctx.snapshot.keyvals.empty()) {
			snapshot_ctx.snapshotMsec = params_since_msec;
			if(since_last) {
				snapshot_ctx.snapshotMsec = 0;
				for(const auto &kv : snapshot_ctx.snapshot.keyvals)
					snapshot_ctx.snapshotMsec = std::max(snapshot_ctx.snapshotMsec, kv.second.epochMsec);
			}
			for(const auto &kv : snapshot_ctx.snapshot.keyvals) {
				ShvJournalEntry e = kv.second;
				e.epochMsec = snapshot_ctx.snapshotMsec;
				e.setSnapshotValue(true);
				// erase EVENT flag in the snapshot values,
				// they can trigger events during reply otherwise
				e.setSpontaneous(false);
				logDShvJournal() << "\t SNAPSHOT entry:" << e.toRpcValueMap().toCpon();
				if(!append_log_entry(e))
					return false;
			}
		}
		return true;
	};
	if(journal_context.files.size()) {
		std::vector<int64_t>::const_iterator first_file_it = journal_context.files.begin();
		journal_start_msec = *first_file_it;
		if(params_since_msec > 0) {
			logMShvJournal() << "since:" << params.since.toCpon() << "msec:" << params_since_msec;
			first_file_it = std::lower_bound(journal_context.files.begin(), journal_context.files.end(), params_since_msec);
			if(first_file_it == journal_context.files.end()) {
				/// take last file
				--first_file_it;
				logMShvJournal() << "\t" << "not found, taking last file:" << *first_file_it << journal_context.fileMsecToFileName(*first_file_it);
			}
			else if(*first_file_it == params_since_msec) {
				/// take exactly this file
				logMShvJournal() << "\t" << "found exactly:" << *first_file_it << journal_context.fileMsecToFileName(*first_file_it);
			}
			else if(first_file_it == journal_context.files.begin()) {
				/// take first file
				logMShvJournal() << "\t" << "begin, taking first file:" << *first_file_it << journal_context.fileMsecToFileName(*first_file_it);
			}
			else {
				/// take previous file
				logMShvJournal() << "\t" << "lower bound found:" << *first_file_it << journal_context.fileMsecToFileName(*first_file_it) << "taking previous file";
				--first_file_it;
			}
		}

		PatternMatcher pattern_matcher(params);
		auto path_match = [&params, &pattern_matcher](const ShvJournalEntry &e) {
			if(!params.pathPattern.empty()) {
				//logDShvJournal() << "\t MATCHING:" << params.pathPattern << "vs:" << e.path;
				if(!pattern_matcher.match(e.path, e.domain))
					return false;
				//logDShvJournal() << "\t\t MATCH";
			}
			return true;
		};
		for(auto file_it = first_file_it; file_it != journal_context.files.end(); file_it++) {
			std::string fn = journal_context.fileMsecToFilePath(*file_it);
			logMShvJournal() << "-------- opening file:" << fn;
			try {
				/**
				 We must check, that any value set to not-default value in previous file,
				 will be set to default value after snapshot of current file is read
				 if it will not be found in current snapshot

				 This solves the case when for example alarm is set felse while shvgate was down
				 */
				std::set<std::string> snapshot_keys;
				std::vector<ShvJournalEntry> generated_entries;
				logMShvJournal() << "Snapshot size:" << snapshot_ctx.snapshot.keyvals.size();
				for(const auto &kv : snapshot_ctx.snapshot.keyvals) {
					// note that snapshot contains ND values only
					if(kv.second.domain == Rpc::SIG_VAL_CHANGED) {
						logMShvJournal() << "\t adding ND path:" << kv.first;
						snapshot_keys.insert(kv.first);
					}
				}
				ShvJournalFileReader rd(fn);
				// read this file snapshot
				while(rd.next()) {
					const ShvJournalEntry &entry = rd.entry();
					//if(entry.path == "someError") logDShvJournal() << "1. snapshot:" << entry.toRpcValue().toCpon();
					if(rd.inSnapshot()) {
						logDShvJournal() << "\t snapshot:" << entry.path;
						if(auto it = snapshot_keys.find(entry.path); it == snapshot_keys.end()) {
							// value in current file snapshot is not present in previous files
							// it might happen for example when some log file is missing
							generated_entries.push_back(entry);
						}
						else {
							// value is in snapshot already
							// it will be ignored
							snapshot_keys.erase(it);
							continue;
						}
					}
					else {
						for(const std::string &key : snapshot_keys) {
							//logDShvJournal() << "\t Setting missing snapshot entry to default value, path:" << key;
							if(auto it = snapshot_ctx.snapshot.keyvals.find(key); it != snapshot_ctx.snapshot.keyvals.end()) {
								ShvJournalEntry e = it->second;
								e.epochMsec = rd.snapshotMsec();
								e.value.setDefaultValue();
								//e.setSnapshotValue(true); not snapshot value, it is result of file merge
								generated_entries.push_back(std::move(e));
							}
							//snapshot_keys.clear();
						}
						generated_entries.push_back(entry);
						break;
					}
				}
				// read rest of file
				logMShvJournal() << "Writing generated values, cnt:" << generated_entries.size() << "note, that last generated value is the first value after snapshot";
				while(true) {
					ShvJournalEntry entry;
					if(generated_entries.empty()) {
						if(!rd.next())
							goto next_file;
						entry = rd.entry();
						//if(entry.path == "someError") logDShvJournal() << "2. regular:" << entry.toRpcValue().toCpon();
					}
					else {
						entry = *generated_entries.begin();
						generated_entries.erase(generated_entries.begin());
						if(generated_entries.empty())
							logMShvJournal() << "Writing regular values";
					}
					if(!path_match(entry))
						continue;
					//logDShvJournal() << "3. filtered:" << entry.toRpcValue().toCpon();
					bool before_since = params_since_msec > 0 && entry.epochMsec < params_since_msec;
					bool after_until = params_until_msec > 0 && entry.epochMsec >= params_until_msec;
					if(before_since) {
						logDShvJournal() << "\t SNAPSHOT entry:" << entry.toRpcValueMap().toCpon();
						addToSnapshot(snapshot_ctx.snapshot, entry);
					}
					else if(after_until) {
						goto log_finish;
					}
					else {
						if(!snapshot_ctx.snapshotWritten) {
							if(!write_snapshot())
								goto log_finish;
						}
#ifdef SKIP_DUP_LOG_ENTRIES
						{
							// skip CHNG duplicates, values that are the same as last log entry for the same path
							auto it = snapshot_ctx.snapshot.find(e.path);
							if (it != snapshot_ctx.snapshot.cend()) {
								if(it->second.value == e.value && e.domain == chainpack::Rpc::SIG_VAL_CHANGED) {
									logDShvJournal() << "\t Skipping DUP LOG entry:" << e.toRpcValueMap().toCpon();
									snapshot_ctx.snapshot.erase(it);
									continue;
								}
							}
						}
#endif
						logDShvJournal() << "\t LOG entry:" << entry.toRpcValueMap().toCpon();
						// keep updating snapshot to enable snapshot erasule on log merge
						addToSnapshot(snapshot_ctx.snapshot, entry);
						if(!append_log_entry(entry))
							goto log_finish;
					}
				}
			}
			catch (const shv::core::Exception &e) {
				shvError() << "Cannot read shv journal file:" << fn;
			}
next_file: ;
		}
	}
log_finish:
	// snapshot should be written already
	// this is only case, when log is empty and
	// only snapshot shall be returned
	if(!snapshot_ctx.snapshotWritten) {
		write_snapshot();
	}

	int64_t log_since_msec = params_since_msec;
	if (since_last) {
		log_since_msec = snapshot_ctx.snapshotMsec;
	}
	else if(log_since_msec < journal_start_msec) {
		log_since_msec = journal_start_msec;
	}
	int64_t log_until_msec = params_until_msec;
	if(params_until_msec == 0 || rec_cnt_limit_hit) {
		log_until_msec = last_record_msec;
	}
	RpcValue ret = log;
	ShvLogHeader log_header;
	{
		log_header.setDeviceId(journal_context.deviceId);
		log_header.setDeviceType(journal_context.deviceType);
		log_header.setDateTime(RpcValue::DateTime::now());
		log_header.setLogParams(params);
		log_header.setSince((log_since_msec > 0)? RpcValue(RpcValue::DateTime::fromMSecsSinceEpoch(log_since_msec)): RpcValue(nullptr));
		log_header.setUntil((log_until_msec > 0)? RpcValue(RpcValue::DateTime::fromMSecsSinceEpoch(log_until_msec)): RpcValue(nullptr));
		log_header.setRecordCount((int)log.size());
		log_header.setRecordCountLimit(rec_cnt_limit);
		log_header.setRecordCountLimitHit(rec_cnt_limit_hit);
		log_header.setWithSnapShot(params.withSnapshot);
		log_header.setWithPathsDict(params.withPathsDict);

		using Column = ShvLogHeader::Column;
		RpcValue::List fields;
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ValueFlags)}});
		fields.push_back(RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::UserId)}});
		log_header.setFields(std::move(fields));
	}
	if(params.withPathsDict) {
		logMShvJournal() << "Generating paths dict size:" << path_cache.size();
		RpcValue::IMap path_dict;
		for(auto kv : path_cache) {
			//logIShvJournal() << "Adding record to paths dict:" << kv.second.toInt() << "-->" << kv.first;
			path_dict[kv.second.toInt()] = kv.first;
		}
		log_header.setPathDict(std::move(path_dict));
	}
	if(params.withTypeInfo) {
		log_header.setTypeInfo(journal_context.typeInfo);
	}
	ret.setMetaData(log_header.toMetaData());
	logIShvJournal() << "result record cnt:" << log.size();
	return ret;
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

} // namespace utils
} // namespace core
} // namespace shv
