#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/ccpcp.h>

#include <ostream>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamWriter
{
	friend void pack_overflow_handler(ccpcp_pack_context *ctx, size_t size_hint);
public:
	AbstractStreamWriter(std::ostream &out);
	virtual ~AbstractStreamWriter();

	virtual void write(const RpcValue::MetaData &meta_data) = 0;
	virtual void write(const RpcValue &val) = 0;

	virtual void writeMapKey(const std::string &key) = 0;
	virtual void writeIMapKey(RpcValue::Int key) = 0;
	virtual void writeContainerBegin(RpcValue::Type container_type, bool is_oneliner = false) = 0;
	virtual void writeListElement(const RpcValue &val) = 0;
	virtual void writeMapElement(const std::string &key, const RpcValue &val) = 0;
	virtual void writeMapElement(RpcValue::Int key, const RpcValue &val) = 0;
	virtual void writeContainerEnd() = 0;
	virtual void writeRawData(const std::string &data) = 0;

	void flush();
protected:
	static constexpr bool WRITE_INVALID_AS_NULL = true;
protected:
	std::ostream &m_out;
	char m_packBuff[32];
	ccpcp_pack_context m_outCtx;
};
} // namespace shv::chainpack
