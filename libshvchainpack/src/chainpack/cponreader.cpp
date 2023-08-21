#include "cponreader.h"
#include "exception.h"
#include "../../c/ccpon.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <array>

namespace shv::chainpack {

#define PARSE_EXCEPTION(msg) {\
	std::array<char, 40> buff; \
	auto err_pos = m_in.tellg(); \
	auto l = m_in.readsome(buff.data(), buff.size() - 1); \
	buff[l] = 0; \
	if(exception_aborts) { \
		std::clog << __FILE__ << ':' << __LINE__;  \
		std::clog << ' ' << (msg) << " at pos: " << err_pos << " near to: " << buff.data() << std::endl; \
		abort(); \
	} \
	else { \
		throw ParseException(m_inCtx.err_no, std::string("Cpon ") \
			+ msg \
			+ std::string(" at pos: ") + std::to_string(err_pos) \
			+ std::string(" line: ") + std::to_string(m_inCtx.parser_line_no) \
			+ " near to: " + buff.data(), err_pos); \
	} \
}

namespace {
enum {exception_aborts = 0};
}

CponReader::CponReader(std::istream &in)
	: Super(in)
{
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

void CponReader::read(RpcValue &val)
{
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
	else
		throw shv::chainpack::Exception(err_msg);
	return {};
}

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

void CponReader::parseMap(RpcValue &out_val)
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
	out_val = map;
}

void CponReader::parseIMap(RpcValue &out_val)
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
	out_val = map;
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

} // namespace shv
