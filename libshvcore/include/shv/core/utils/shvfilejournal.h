#ifndef SHV_CORE_UTILS_FILESHVJOURNAL_H
#define SHV_CORE_UTILS_FILESHVJOURNAL_H

#include "../shvcoreglobal.h"
#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"

#include <fstream>
#include <functional>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvFileJournal : public AbstractShvJournal
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 1024 * 1024;
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * DEFAULT_FILE_SIZE_LIMIT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';
	static const std::string FILE_EXT;
public:
	ShvFileJournal();
	ShvFileJournal(std::string device_id);

	void setJournalDir(std::string s);
	const std::string& journalDir();
	void setFileSizeLimit(const std::string &n);
	void setFileSizeLimit(int64_t n);
	int64_t fileSizeLimit() const;
	void setJournalSizeLimit(const std::string &n);
	void setJournalSizeLimit(int64_t n);
	int64_t journalSizeLimit() const;
	void setTypeInfo(const ShvTypeInfo &i);
	const ShvTypeInfo& typeInfo() const;
	std::string deviceId() const;
	void setDeviceId(std::string id);
	std::string deviceType() const;
	void setDeviceType(std::string type);
	int64_t recentlyWrittenEntryDateTime() const;

	static int64_t findLastEntryDateTime(const std::string &fn, int64_t journal_start_msec, std::ifstream::pos_type *p_date_time_fpos = nullptr);
	void append(const ShvJournalEntry &entry) override;

	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params, bool is_record_count_limit_limitted = true) override;
	shv::chainpack::RpcValue getSnapShotMap() override;

	void convertLog1JournalDir();
public:
	struct TxtColumn
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
			Domain,
			ValueFlags,
			UserId,
		};
		static const char* name(Enum e);
	};
	static constexpr int TxtColumnCount = TxtColumn::UserId + 1;
	struct JournalContext
	{
		bool journalDirExists = false;
		std::vector<int64_t> files;
		int64_t journalSize = -1;
		int64_t lastFileSize = -1;
		int64_t recentTimeStamp = 0;
		std::string journalDir;

		std::string deviceId;
		std::string deviceType;
		ShvTypeInfo typeInfo;

		bool isConsistent() const;
		static int64_t fileNameToFileMsec(const std::string &fn);
		static std::string msecToBaseFileName(int64_t msec);
		static std::string fileMsecToFileName(int64_t msec);
		std::string fileMsecToFilePath(int64_t file_msec) const;
	};
	static constexpr bool Force = true;
	const JournalContext& checkJournalContext(bool force = !Force);
	void createNewLogFile(int64_t journal_file_start_msec = 0);
	static shv::chainpack::RpcValue getLog(const JournalContext &journal_context, const ShvGetLogParams &params, bool is_record_count_limit_limitted = true);
private:

	void checkJournalContext_helper(bool force = false);

	void rotateJournal();
	void updateJournalStatus();
	void checkRecentTimeStamp();
	void createJournalDirIfNotExist();
	bool journalDirExists();

	void appendThrow(const ShvJournalEntry &entry);
private:
	JournalContext m_journalContext;

	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_FILESHVJOURNAL_H
