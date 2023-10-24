#pragma once

namespace shv {
namespace core {
namespace utils {

class ShvJournalCommon
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr int DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;

	static constexpr auto KEY_NAME = "name";
	static constexpr auto KEY_RECORD_COUNT = "recordCount";
	static constexpr auto KEY_PATHS_DICT = "pathsDict";

	struct Column
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
			Domain,
			Course,
		};
		static const char* name(Enum e);
	};
protected:
	int m_getLogRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv
