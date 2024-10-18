#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvjournalentry.h>

#include <shv/core/exception.h>
#include <shv/core/log.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv::core::utils {

ShvLogFileReader::ShvLogFileReader(chainpack::ChainPackReader *reader)
	: m_reader(reader)
{
	init();
}

ShvLogFileReader::ShvLogFileReader(const std::string &file_name)
{
	m_readerCreated = true;
	m_ifstream = new std::ifstream;
	m_ifstream->open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");
	m_reader = new shv::chainpack::ChainPackReader(*m_ifstream);
	init();
}

void ShvLogFileReader::init()
{
	cp::RpcValue::MetaData md;
	m_reader->read(md);
	m_logHeader = ShvLogHeader::fromMetaData(md);

	chainpack::ChainPackReader::ItemType t = m_reader->unpackNext();
	if(t != chainpack::ChainPackReader::ItemType::CCPCP_ITEM_LIST)
		SHV_EXCEPTION("Log is corrupted!");
}

ShvLogFileReader::~ShvLogFileReader()
{
	if(m_readerCreated) {
		delete m_reader;
		delete m_ifstream;
	}
}

bool ShvLogFileReader::next()
{
	auto unmap_path = [this](const cp::RpcValue &p) {
		if(p.isInt())
			return m_logHeader.pathDictCRef().value(p.toInt()).asString();
		return p.asString();
	};
	while(true) {
		m_currentEntry = ShvJournalEntry();
		if(!m_ifstream)
			return false;
		chainpack::ChainPackReader::ItemType tt = m_reader->peekNext();
		if(tt == chainpack::ChainPackReader::ItemType::CCPCP_ITEM_CONTAINER_END)
			return false;

		cp::RpcValue val;
		m_reader->read(val);
		const chainpack::RpcList &row = val.asList();
		std::string err;
		m_currentEntry = ShvJournalEntry::fromRpcValueList(row, unmap_path, &err);
		if(!err.empty()) {
			logWShvJournal() << err;
			continue;
		}
		return true;
	}
}

const ShvJournalEntry &ShvLogFileReader::entry()
{
	return m_currentEntry;
}

const ShvLogHeader &ShvLogFileReader::logHeader() const
{
	return m_logHeader;
}

} // namespace shv
