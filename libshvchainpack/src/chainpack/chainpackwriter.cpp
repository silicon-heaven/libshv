#include "chainpackwriter.h"
#include "cpon.h"
#include "exception.h"

#include "../../c/cchainpack.h"

#include <iostream>
#include <cmath>
#include <sstream>

#include <string.h>

namespace shv {
namespace chainpack {

void ChainPackWriter::writeUIntData(uint64_t n)
{
	cchainpack_pack_uint_data(&m_outCtx, n);
}

void ChainPackWriter::write(const RpcValue &value)
{
	if(!value.metaData().isEmpty()) {
		write(value.metaData());
	}
	switch (value.type()) {
	case RpcValue::Type::Null: write_p(nullptr); break;
	case RpcValue::Type::UInt: write_p(value.toUInt64()); break;
	case RpcValue::Type::Int: write_p(value.toInt64()); break;
	case RpcValue::Type::Double: write_p(value.toDouble()); break;
	case RpcValue::Type::Bool: write_p(value.toBool()); break;
	case RpcValue::Type::Blob: write_p(value.asBlob()); break;
	case RpcValue::Type::String: write_p(value.asString()); break;
	case RpcValue::Type::DateTime: write_p(value.toDateTime()); break;
	case RpcValue::Type::List: write_p(value.toList()); break;
	case RpcValue::Type::Map: write_p(value.asMap()); break;
	case RpcValue::Type::IMap: write_p(value.asIMap()); break;
	case RpcValue::Type::Decimal: write_p(value.toDecimal()); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write_p(nullptr);
		}
		break;
	}
}

void ChainPackWriter::write(const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		cchainpack_pack_meta_begin(&m_outCtx);
		const RpcValue::IMap &cim = meta_data.iValues();
		for (const auto &kv : cim) {
			writeMapElement(kv.first, kv.second);
		}
		const RpcValue::Map &csm = meta_data.sValues();
		for (const auto &kv : csm) {
			writeMapElement(kv.first, kv.second);
		}
		cchainpack_pack_container_end(&m_outCtx);
	}
}

void ChainPackWriter::writeContainerBegin(RpcValue::Type container_type, bool )
{
	switch (container_type) {
	case RpcValue::Type::List:
		cchainpack_pack_list_begin(&m_outCtx);
		break;
	case RpcValue::Type::Map:
		cchainpack_pack_map_begin(&m_outCtx);
		break;
	case RpcValue::Type::IMap:
		cchainpack_pack_imap_begin(&m_outCtx);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write begin of container type: ") + RpcValue::typeToName(container_type));
	}
}

void ChainPackWriter::writeContainerEnd()
{
	cchainpack_pack_container_end(&m_outCtx);
}

void ChainPackWriter::writeMapKey(const std::string &key)
{
	write(key);
}

void ChainPackWriter::writeIMapKey(RpcValue::Int key)
{
	write(key);
}

void ChainPackWriter::writeListElement(const RpcValue &val)
{
	write(val);
}

void ChainPackWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	writeMapKey(key);
	write(val);
}

void ChainPackWriter::writeMapElement(RpcValue::Int key, const RpcValue &val)
{
	writeIMapKey(key);
	write(val);
}

void ChainPackWriter::writeRawData(const std::string &data)
{
	ccpcp_pack_copy_bytes(&m_outCtx, data.data(), data.size());
}

ChainPackWriter &ChainPackWriter::write_p(std::nullptr_t)
{
	cchainpack_pack_null(&m_outCtx);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(bool value)
{
	cchainpack_pack_boolean(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(int32_t value)
{
	cchainpack_pack_int(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(uint32_t value)
{
	cchainpack_pack_uint(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(int64_t value)
{
	cchainpack_pack_int(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(uint64_t value)
{
	cchainpack_pack_uint(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(double value)
{
	cchainpack_pack_double(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(RpcValue::Decimal value)
{
	cchainpack_pack_decimal(&m_outCtx, value.mantisa(), value.exponent());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(RpcValue::DateTime value)
{
	cchainpack_pack_date_time(&m_outCtx, value.msecsSinceEpoch(), value.minutesFromUtc());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const std::string &value)
{
	cchainpack_pack_string(&m_outCtx, value.data(), value.size());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::Blob &value)
{
	cchainpack_pack_blob(&m_outCtx, value.data(), value.size());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::Map &values)
{
	writeContainerBegin(RpcValue::Type::Map);
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd();
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::IMap &values)
{
	writeContainerBegin(RpcValue::Type::IMap);
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd();
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List);
	for (size_t ix = 0; ix < values.size(); ix++) {
		const RpcValue &value = values[ix];
		writeListElement(value);
	}
	writeContainerEnd();
	return *this;
}

} // namespace chainpack
} // namespace shv
