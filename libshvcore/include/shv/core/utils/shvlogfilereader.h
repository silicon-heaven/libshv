#pragma once

#include "../shvcoreglobal.h"

#include "shvlogheader.h"
#include "shvjournalentry.h"

#include <shv/chainpack/chainpackreader.h>

#include <string>
#include <fstream>

namespace shv {
namespace chainpack { class ChainPackReader; }
namespace core {
namespace utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvLogFileReader
{
public:
	ShvLogFileReader(shv::chainpack::ChainPackReader *reader);
	ShvLogFileReader(const std::string &file_name);
	~ShvLogFileReader();

	bool next();
	const ShvJournalEntry& entry();

	const ShvLogHeader &logHeader() const;
private:
	void init();
private:
	ShvLogHeader m_logHeader;

	shv::chainpack::ChainPackReader *m_reader = nullptr;
	bool m_readerCreated = false;
	std::ifstream *m_ifstream = nullptr;

	ShvJournalEntry m_currentEntry;
};

} // namespace utils
} // namespace core
} // namespace shv
