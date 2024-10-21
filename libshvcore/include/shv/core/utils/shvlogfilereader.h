#pragma once

#include <shv/core/shvcoreglobal.h>

#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvjournalentry.h>

#include <shv/chainpack/chainpackreader.h>

#include <string>
#include <fstream>

namespace shv {
namespace chainpack { class ChainPackReader; }
namespace core::utils {

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

} // namespace core::utils
} // namespace shv
