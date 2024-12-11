#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/ccpcp.h>

#include <istream>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT ParseException : public std::runtime_error
{
	using Super = std::runtime_error;
public:
	ParseException(int err_code, const std::string &msg, long long pos, const std::string &dump);

	int errCode() const;
	long long pos() const;
private:
	int m_errCode;
	long long m_pos = -1;
};

class SHVCHAINPACK_DECL_EXPORT AbstractStreamReader
{
public:
	friend size_t unpack_underflow_handler(ccpcp_unpack_context *ctx);
public:
	AbstractStreamReader(std::istream &in);
	virtual ~AbstractStreamReader();

	RpcValue read(std::string *error = nullptr);

	virtual void read(RpcValue::MetaData &meta_data) = 0;
	virtual void read(RpcValue &val) = 0;

	ssize_t readCount() const;
protected:
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
	std::istream &m_in;
	char m_unpackBuff;
	ccpcp_unpack_context m_inCtx;
};
} // namespace shv::chainpack
