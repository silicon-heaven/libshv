#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/exception.h>

#include <shv/chainpack/ccpon.h>

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>

namespace shv::chainpack {
bool CponWriterOptions::isTranslateIds() const
{
	return m_translateIds;
}

CponWriterOptions& CponWriterOptions::setTranslateIds(bool b)
{
	m_translateIds = b; return *this;
}

bool CponWriterOptions::isHexBlob() const
{
	return m_hexBlob;
}

CponWriterOptions& CponWriterOptions::setHexBlob(bool b)
{
	m_hexBlob = b; return *this;
}

const std::string& CponWriterOptions::indent() const
{
	return m_indent;
}

CponWriterOptions& CponWriterOptions::setIndent(const std::string& i)
{
	m_indent = i; return *this;
}

bool CponWriterOptions::isJsonFormat() const
{
	return m_jsonFormat;
}

CponWriterOptions& CponWriterOptions::setJsonFormat(bool b)
{
	m_jsonFormat = b; return *this;
}

namespace {
bool is_oneline_list(const RpcValue::List &lst)
{
	if(lst.size() > 10)
		return false;
	for (const RpcValue &it : lst) {
		switch (it.type()) {
		case RpcValue::Type::Map:
		case RpcValue::Type::IMap:
		case RpcValue::Type::List:
			return false;
		default:
			break;
		}
	}
	return true;
}

template<typename T>
bool is_oneline_map(const T &map)
{
	if(map.size() > 5)
		return false;
	for (const auto &kv : map) {
		switch (kv.second.type()) {
		case RpcValue::Type::Map:
		case RpcValue::Type::IMap:
		case RpcValue::Type::List:
			return false;
		default:
			break;
		}
	}
	return true;
}

bool is_oneline_meta(const RpcValue::MetaData &meta)
{
	if(meta.size() > 5)
		return false;
	for (const auto &kv : meta.iValues()) {
		switch (kv.second.type()) {
		case RpcValue::Type::Map:
		case RpcValue::Type::IMap:
		case RpcValue::Type::List:
			return false;
		default:
			break;
		}
	}
	for (const auto &kv : meta.sValues()) {
		switch (kv.second.type()) {
		case RpcValue::Type::Map:
		case RpcValue::Type::IMap:
		case RpcValue::Type::List:
			return false;
		default:
			break;
		}
	}
	return true;
}
}

CponWriter::CponWriter(std::ostream &out)
	: Super(out)
{
}

CponWriter::CponWriter(std::ostream &out, const CponWriterOptions &opts)
	: Super(out)
	, m_opts(opts)
{
	m_outCtx.cpon_options.json_output = opts.isJsonFormat();
	m_outCtx.cpon_options.indent = m_opts.indent().empty()? nullptr: m_opts.indent().data();
}

bool CponWriter::writeFile(const std::string &file_name, const RpcValue &rv, std::string *err)
{
	std::ofstream ofs(file_name, std::ios::binary);
	if(ofs) {
		{
			CponWriterOptions opts;
			opts.setIndent("\t");
			CponWriter wr(ofs, opts);
			wr.write(rv);
		}
		if(!ofs) {
			if(err)
				*err = "Error write file '" + file_name + "'.";
			return false;
		}
		return true;
	}
	if(err)
		*err = "Cannot open file '" + file_name + "' for writing.";
	return false;
}

CponWriter& CponWriter::operator <<(const RpcValue &value)
{
	write(value);
	return *this;
}

CponWriter& CponWriter::operator <<(const RpcValue::MetaData &meta_data)
{
	write(meta_data);
	return *this;
}

void CponWriter::write(const RpcValue &value)
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
	case RpcValue::Type::List: write_p(value.asList()); break;
	case RpcValue::Type::Map: write_p(value.asMap()); break;
	case RpcValue::Type::IMap: write_p(value.asIMap(), &value.metaData()); break;
	case RpcValue::Type::Decimal: write_p(value.toDecimal()); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write_p(nullptr);
		}
		break;
	}
}

void CponWriter::write(const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		writeMetaBegin(is_oneline_meta(meta_data));
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty()) {
			for (const auto &kv : cim) {
				if(m_opts.isTranslateIds()) {
					ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
					ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0, cs.isOneLiner);
					int nsid = meta_data.metaTypeNameSpaceId();
					int mtid = meta_data.metaTypeId();
					int tag = kv.first;
					const meta::MetaInfo &tag_info = meta::registeredType(nsid, mtid).tagById(tag);
					if(tag_info.isValid())
						ccpcp_pack_copy_bytes(&m_outCtx, tag_info.name, ::strlen(tag_info.name));
					else
						write(tag);
					ccpon_pack_key_val_delim(&m_outCtx);
					RpcValue meta_val = kv.second;
					if(tag == meta::Tag::MetaTypeNameSpaceId) {
						int id = meta_val.toInt();
						const meta::MetaNameSpace &type = meta::registeredNameSpace(nsid);
						const char *n = type.name();
						if(n[0])
							ccpcp_pack_copy_bytes(&m_outCtx, n, ::strlen(n));
						else
							write(id);
					}
					else if(tag == meta::Tag::MetaTypeId) {
						int id = meta_val.toInt();
						const meta::MetaType &type = meta::registeredType(nsid, id);
						const char *n = type.name();
						if(n[0])
							ccpcp_pack_copy_bytes(&m_outCtx, n, ::strlen(n));
						else
							write(id);
					}
					else {
						write(meta_val);
					}
				}
				else {
					writeMapElement(kv.first, kv.second);
				}
			}
		}
		const RpcValue::Map &csm = meta_data.sValues();
		for (const auto &kv : csm) {
			writeMapElement(kv.first, kv.second);
		}
		writeMetaEnd();
	}
}

void CponWriter::writeMetaBegin(bool is_oneliner)
{
	ccpon_pack_meta_begin(&m_outCtx);
	m_containerStates.emplace_back(RpcValue::Type::Invalid, is_oneliner);
}

void CponWriter::writeMetaEnd()
{
	const ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_meta_end(&m_outCtx, cs.isOneLiner);
	m_containerStates.pop_back();
}

void CponWriter::writeContainerBegin(RpcValue::Type container_type, bool is_oneliner)
{
	switch (container_type) {
	case RpcValue::Type::List:
		ccpon_pack_list_begin(&m_outCtx);
		break;
	case RpcValue::Type::Map:
		ccpon_pack_map_begin(&m_outCtx);
		break;
	case RpcValue::Type::IMap:
		ccpon_pack_imap_begin(&m_outCtx);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write begin of container type: ") + RpcValue::typeToName(container_type));
	}
	m_containerStates.emplace_back(container_type, is_oneliner);
}

void CponWriter::writeContainerEnd()
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	switch (cs.containerType) {
	case RpcValue::Type::List:
		ccpon_pack_list_end(&m_outCtx, cs.isOneLiner);
		break;
	case RpcValue::Type::Map:
		ccpon_pack_map_end(&m_outCtx, cs.isOneLiner);
		break;
	case RpcValue::Type::IMap:
		ccpon_pack_imap_end(&m_outCtx, cs.isOneLiner);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write end of container type: ") + RpcValue::typeToName(cs.containerType));
	}
	m_containerStates.pop_back();
}

void CponWriter::writeMapKey(const std::string &key)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0, cs.isOneLiner);
	write(key);
	ccpon_pack_key_val_delim(&m_outCtx);
}

void CponWriter::writeIMapKey(RpcValue::Int key)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0, cs.isOneLiner);
	write(key);
	ccpon_pack_key_val_delim(&m_outCtx);
}

void CponWriter::writeListElement(const RpcValue &val)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0, cs.isOneLiner);
	write(val);
}

void CponWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	writeMapKey(key);
	write(val);
}

void CponWriter::writeMapElement(RpcValue::Int key, const RpcValue &val)
{
	writeIMapKey(key);
	write(val);
}

void CponWriter::writeRawData(const std::string &data)
{
	ccpcp_pack_copy_bytes(&m_outCtx, data.data(), data.size());
}

CponWriter &CponWriter::write_p(std::nullptr_t)
{
	ccpon_pack_null(&m_outCtx);
	return *this;
}

CponWriter &CponWriter::write_p(bool value)
{
	ccpon_pack_boolean(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(int32_t value)
{
	ccpon_pack_int(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(uint32_t value)
{
	ccpon_pack_uint(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(int64_t value)
{
	ccpon_pack_int(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(uint64_t value)
{
	ccpon_pack_uint(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(double value)
{
	ccpon_pack_double(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(RpcValue::Decimal value)
{
	ccpon_pack_decimal(&m_outCtx, value.mantisa(), value.exponent());
	return *this;
}

CponWriter &CponWriter::write_p(RpcValue::DateTime value)
{
	ccpon_pack_date_time(&m_outCtx, value.msecsSinceEpoch(), value.minutesFromUtc());
	return *this;
}

CponWriter &CponWriter::write_p(const std::string &value)
{
	ccpon_pack_string(&m_outCtx, value.data(), value.size());
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::Blob &value)
{
	ccpon_pack_blob(&m_outCtx, value.data(), value.size());
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::Map &values)
{
	writeContainerBegin(RpcValue::Type::Map, is_oneline_map(values));
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd();
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	writeContainerBegin(RpcValue::Type::IMap, is_oneline_map(values));
	for (const auto &kv : values) {
		if(m_opts.isTranslateIds() && meta_data) {
			ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
			ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0, cs.isOneLiner);
			int mtid = meta_data->metaTypeId();
			int nsid = meta_data->metaTypeNameSpaceId();
			auto key = kv.first;
			const meta::MetaInfo key_info = meta::registeredType(nsid, mtid).keyById(key);
			if(key_info.isValid()) {
				ccpcp_pack_copy_bytes(&m_outCtx, key_info.name, ::strlen(key_info.name));
			}
			else {
				write(key);
			}
			ccpon_pack_key_val_delim(&m_outCtx);
			write(kv.second);
		}
		else {
			writeMapElement(kv.first, kv.second);
		}
	}
	writeContainerEnd();
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List, is_oneline_list(values));
	for (const auto& value : values) {
		writeListElement(value);
	}
	writeContainerEnd();
	return *this;
}

CponWriter::ContainerState::ContainerState() = default;

CponWriter::ContainerState::ContainerState(RpcValue::Type t, bool one_liner)
	: containerType(t)
	, isOneLiner(one_liner)
{
}

} // namespace shv
