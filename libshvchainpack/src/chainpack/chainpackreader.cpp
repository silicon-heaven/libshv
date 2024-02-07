#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/utils.h>
#include <shv/chainpack/cchainpack.h>

#include <iostream>
#include <array>

namespace shv::chainpack {

ChainPackReader::ChainPackReader(std::istream &in)
	: Super(in)
{
}

void ChainPackReader::throwParseException(const std::string &msg)
{
	std::array<char, 64> buff;
	auto err_pos = m_in.tellg();
	auto l = m_in.readsome(buff.data(), buff.size() - 1);
	buff[l] = 0;
	auto dump = shv::chainpack::utils::hexDump(buff.data(), l);

	std::string msg2 = m_inCtx.err_msg? m_inCtx.err_msg: "";
	if (!msg2.empty() && !msg.empty())
		msg2 += " - ";
	msg2 += msg;

	throw ParseException(m_inCtx.err_no, msg2, err_pos, dump);
}

ChainPackReader &ChainPackReader::operator >>(RpcValue &value)
{
	read(value);
	return *this;
}

ChainPackReader &ChainPackReader::operator >>(RpcValue::MetaData &meta_data)
{
	read(meta_data);
	return *this;
}

ChainPackReader::ItemType ChainPackReader::unpackNext()
{
	cchainpack_unpack_next(&m_inCtx);
	if(m_inCtx.err_no != CCPCP_RC_OK)
		throwParseException();
	return m_inCtx.item.type;
}

const char *ChainPackReader::itemTypeToString(ChainPackReader::ItemType it)
{
	return ccpcp_item_type_to_string(it);
}

ChainPackReader::ItemType ChainPackReader::peekNext()
{
	const char *p = ccpcp_unpack_peek_byte(&m_inCtx);
	if(!p)
		throwParseException();
	auto sch = static_cast<cchainpack_pack_packing_schema>(static_cast<uint8_t>(*p));
	switch(sch) {
	case CP_Null: return CCPCP_ITEM_NULL;
	case CP_UInt: return CCPCP_ITEM_UINT;
	case CP_Int: return CCPCP_ITEM_INT;
	case CP_Double: return CCPCP_ITEM_DOUBLE;
	case CP_Bool: return CCPCP_ITEM_BOOLEAN;
	case CP_String: return CCPCP_ITEM_STRING;
	case CP_List: return CCPCP_ITEM_LIST;
	case CP_Map: return CCPCP_ITEM_MAP;
	case CP_IMap: return CCPCP_ITEM_IMAP;
	case CP_MetaMap: return CCPCP_ITEM_META;
	case CP_Decimal: return CCPCP_ITEM_DECIMAL;
	case CP_DateTime: return CCPCP_ITEM_DATE_TIME;
	case CP_CString: return CCPCP_ITEM_STRING;
	case CP_FALSE:
	case CP_TRUE: return CCPCP_ITEM_BOOLEAN;
	case CP_TERM: return CCPCP_ITEM_CONTAINER_END;
	default: break;
	}
	return CCPCP_ITEM_INVALID;
}

uint64_t ChainPackReader::readUIntData(int *err_code)
{
	return cchainpack_unpack_uint_data2(&m_inCtx, err_code);
}

uint64_t ChainPackReader::readUIntData(std::istream &in, int *err_code)
{
	ChainPackReader rd(in);
	return rd.readUIntData(err_code);
}

void ChainPackReader::read(RpcValue &val)
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
	case CCPCP_ITEM_STRING: {
		ccpcp_string *it = &(m_inCtx.item.as.String);
		std::string str;
		while(m_inCtx.item.type == CCPCP_ITEM_STRING) {
			str += std::string(it->chunk_start, it->chunk_size);
			if(it->last_chunk)
				break;
			unpackNext();
			if(m_inCtx.item.type != CCPCP_ITEM_STRING)
				throwParseException("Unfinished string");
		}
		val = str;
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
				throwParseException("Unfinished blob");
		}
		val = blob;
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
		throwParseException("Invalid type.");
	}
	if(!md.isEmpty()) {
		if(!val.isValid())
			throwParseException("Attempt to set metadata to invalid RPC value.");
		val.setMetaData(std::move(md));
	}
}

void ChainPackReader::parseList(RpcValue &val)
{
	RpcValue::List lst;
	while (true) {
		RpcValue v;
		read(v);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		lst.push_back(v);
	}
	val = lst;
}

void ChainPackReader::parseMetaData(RpcValue::MetaData &meta_data)
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

void ChainPackReader::parseMap(RpcValue &out_val)
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

void ChainPackReader::parseIMap(RpcValue &out_val)
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

void ChainPackReader::read(RpcValue::MetaData &meta_data)
{
	auto b = reinterpret_cast<const uint8_t*>(ccpcp_unpack_take_byte(&m_inCtx));
	m_inCtx.current--;
	if(b && *b == CP_MetaMap) {
		cchainpack_unpack_next(&m_inCtx);
		parseMetaData(meta_data);
	}
}

} // namespace shv
