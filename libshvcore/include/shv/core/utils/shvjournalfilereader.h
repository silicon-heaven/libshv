#pragma once

#include "../shvcoreglobal.h"
#include "shvjournalentry.h"
#include "shvtypeinfo.h"
#include "../exception.h"

#include <string>
#include <fstream>

namespace shv::core::utils {

class SHVCORE_DECL_EXPORT ShvJournalFileReader
{
public:
	ShvJournalFileReader(const std::string &file_name);
	ShvJournalFileReader(std::istream &istream);

	bool next();
	bool last();
	const ShvJournalEntry& entry();
	bool inSnapshot();
	int64_t snapshotMsec() const;

	static int64_t fileNameToFileMsec(const std::string &fn, bool throw_exc = shv::core::Exception::Throw);
	static std::string msecToBaseFileName(int64_t msec);
private:
	std::string m_fileName;
	std::ifstream m_inputFileStream;
	std::istream *m_istream = nullptr;
	ShvJournalEntry m_currentEntry;
	int64_t m_snapshotMsec = 0;
	bool m_inSnapshot = true;
};
} // namespace shv::core::utils
