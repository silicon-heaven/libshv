#ifndef SHV_CORE_UTILS_SHVJOURNALFILEREADER_H
#define SHV_CORE_UTILS_SHVJOURNALFILEREADER_H

#include "../shvcoreglobal.h"
#include "shvjournalentry.h"
#include "shvlogtypeinfo.h"
#include "../exception.h"

#include <string>
#include <fstream>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvJournalFileReader
{
public:
	ShvJournalFileReader(const std::string &file_name);

	bool next();
	bool last();
	const ShvJournalEntry& entry();
	bool inSnapshot();
	int64_t snapshotMsec() const { return m_snapshotMsec; }

	static int64_t fileNameToFileMsec(const std::string &fn, bool throw_exc = shv::core::Exception::Throw);
	static std::string msecToBaseFileName(int64_t msec);
private:
	std::string m_fileName;
	std::ifstream m_ifstream;
	ShvJournalEntry m_currentEntry;
	int64_t m_snapshotMsec = -1;
	bool m_inSnapshot = true;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALFILEREADER_H
