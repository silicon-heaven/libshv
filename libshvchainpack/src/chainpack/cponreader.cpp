#include "cponreader.h"
//#include "cpon.h"
#include "../../c/ccpon.h"

#include <iostream>
#include <fstream>
#include <cmath>

namespace shv {
namespace chainpack {

#define PARSE_EXCEPTION(msg) {\
	char buff[40]; \
	int l = m_in.readsome(buff, sizeof(buff) - 1); \
	buff[l] = 0; \
	if(exception_aborts) { \
		std::clog << __FILE__ << ':' << __LINE__;  \
		std::clog << ' ' << (msg) << " at pos: " << m_in.tellg() << " near to: " << buff << std::endl; \
		abort(); \
	} \
	else { \
		throw ParseException(m_inCtx.err_no, std::string("Cpon ") \
			+ msg \
			+ std::string(" at pos: ") + std::to_string(m_in.tellg()) \
			+ std::string(" line: ") + std::to_string(m_inCtx.parser_line_no) \
			+ " near to: " + buff, m_in.tellg()); \
	} \
}

namespace {
enum {exception_aborts = 0};
/*
const int MAX_RECURSION_DEPTH = 1000;

class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};
*/
}

CponReader &CponReader::operator >>(RpcValue &value)
{
	read(value);
	return *this;
}

CponReader &CponReader::operator >>(RpcValue::MetaData &meta_data)
{
	read(meta_data);
	return *this;
}

void CponReader::unpackNext()
{
	ccpon_unpack_next(&m_inCtx);
	if(m_inCtx.err_no != CCPCP_RC_OK)
		PARSE_EXCEPTION("Parse error: " + std::to_string(m_inCtx.err_no) + " " + ccpcp_error_string(m_inCtx.err_no) + " - " + std::string(m_inCtx.err_msg));
}
/*
void CponReader::read(RpcValue &val, std::string &err)
{
	err.clear();
	try {
		read(val);
	}
	catch (ParseException &e) {
		err = e.what();
	}
}

void CponReader::read(RpcValue &val, std::string *err)
{
	if(err)
		read(val, *err);
	else
		read(val);
}

RpcValue::DateTime CponReader::readDateTime()
{
	struct tm tm;
	int msec;
	int utc_offset;
	ccpon_unpack_date_time(&m_inCtx, &tm, &msec, &utc_offset);
	if(m_inCtx.err_no != CCPCP_RC_OK)
		return RpcValue::DateTime();
	return RpcValue::DateTime::fromMSecsSinceEpoch(msec, utc_offset);
}
*/
void CponReader::read(RpcValue &val)
{
	//if (m_depth > MAX_RECURSION_DEPTH)
	//	PARSE_EXCEPTION("maximum nesting depth exceeded");
	//DepthScope{m_depth};

	RpcValue::MetaData md;
	read(md);

	unpackNext();

	switch(m_inCtx.item.type) {
	case CCPCP_ITEM_INVALID: {
		// end of input
		break;
	}
	case CCPCP_ITEM_LIST: {
		parseList(val);
		break;
	}
	case CCPCP_ITEM_MAP: {
		parseMap(val);
		break;
	}
	case CCPCP_ITEM_IMAP: {
		parseIMap(val);
		break;
	}
	case CCPCP_ITEM_CONTAINER_END: {
		break;
	}
	case CCPCP_ITEM_NULL: {
		val = RpcValue(nullptr);
		break;
	}
	case CCPCP_ITEM_BLOB: {
		ccpcp_string *it = &(m_inCtx.item.as.String);
		RpcValue::Blob blob;
		while(m_inCtx.item.type == CCPCP_ITEM_BLOB) {
			blob.insert(blob.end(), it->chunk_start, it->chunk_start + it->chunk_size);
			if(it->last_chunk)
				break;
			unpackNext();
			if(m_inCtx.item.type != CCPCP_ITEM_BLOB)
				PARSE_EXCEPTION("Unfinished blob key");
		}
		val = RpcValue(blob);
		break;
	}
	case CCPCP_ITEM_STRING: {
		ccpcp_string *it = &(m_inCtx.item.as.String);
		std::string str;
		while(m_inCtx.item.type == CCPCP_ITEM_STRING) {
			str += std::string(it->chunk_start, it->chunk_size);
			if(it->last_chunk)
				break;
			unpackNext();
			if(m_inCtx.item.type != CCPCP_ITEM_STRING)
				PARSE_EXCEPTION("Unfinished string key");
		}
		val = str;
		break;
	}
	case CCPCP_ITEM_BOOLEAN: {
		val = m_inCtx.item.as.Bool;
		break;
	}
	case CCPCP_ITEM_INT: {
		val = m_inCtx.item.as.Int;
		break;
	}
	case CCPCP_ITEM_UINT: {
		val = m_inCtx.item.as.UInt;
		break;
	}
	case CCPCP_ITEM_DECIMAL: {
		auto *it = &(m_inCtx.item.as.Decimal);
		val = RpcValue::Decimal(it->mantisa, it->exponent);
		break;
	}
	case CCPCP_ITEM_DOUBLE: {
		val = m_inCtx.item.as.Double;
		break;
	}
	case CCPCP_ITEM_DATE_TIME: {
		auto *it = &(m_inCtx.item.as.DateTime);
		val = RpcValue::DateTime::fromMSecsSinceEpoch(it->msecs_since_epoch, it->minutes_from_utc);
		break;
	}
	default:
		PARSE_EXCEPTION("Invalid type.");
	}
	if(!md.isEmpty()) {
		if(!val.isValid())
			PARSE_EXCEPTION(std::string("Attempt to set metadata to invalid RPC value. error - ") + m_inCtx.err_msg);
		val.setMetaData(std::move(md));
	}
}

RpcValue CponReader::readFile(const std::string &file_name, std::string *error)
{
	std::ifstream ifs(file_name);
	if(ifs) {
		CponReader rd(ifs);
		return rd.read(error);
	}
	std::string err_msg = "Cannot open file " + file_name + " for reading";
	if(error)
		*error = err_msg;
	return {};
}

/*
void CponReader::decodeUtf8(long pt, std::string &out)
{
	if (pt < 0)
		return;

	if (pt < 0x80) {
		out += static_cast<char>(pt);
	}
	else if (pt < 0x800) {
		out += static_cast<char>((pt >> 6) | 0xC0);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
	else if (pt < 0x10000) {
		out += static_cast<char>((pt >> 12) | 0xE0);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
	else {
		out += static_cast<char>((pt >> 18) | 0xF0);
		out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
}
*/

void CponReader::parseList(RpcValue &val)
{
	RpcValue::List lst;
	while (true) {
		RpcValue v;
		read(v);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID; // to parse something like [[]]
			break;
		}
		lst.push_back(v);
	}
	val = lst;
}

void CponReader::parseMetaData(RpcValue::MetaData &meta_data)
{
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		if(key.isString())
			meta_data.setValue(key.asString(), val);
		else
			meta_data.setValue(key.toInt(), val);
	}
}

void CponReader::parseMap(RpcValue &val)
{
	RpcValue::Map map;
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		map[key.asString()] = val;
	}
	val = map;
}

void CponReader::parseIMap(RpcValue &val)
{
	RpcValue::IMap map;
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		map[key.toInt()] = val;
	}
	val = map;
}

void CponReader::read(RpcValue::MetaData &meta_data)
{
	const char *c = ccpon_unpack_skip_insignificant(&m_inCtx);
	m_inCtx.current--;
	if(c && *c == '<') {
		ccpon_unpack_next(&m_inCtx);
		parseMetaData(meta_data);
	}
}

} // namespace chainpack
} // namespace shv
