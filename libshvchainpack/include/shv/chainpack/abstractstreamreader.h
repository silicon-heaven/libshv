#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/ccpcp.h>

#include <istream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ParseException : public std::exception
{
	using Super = std::exception;
public:
	ParseException(int err_code, const std::string &msg, long long pos, const std::string &dump);

	const char *what() const noexcept override;
	int errCode() const;
	long long pos() const;
	const std::string& msg() const;
private:
	int m_errCode;
	std::string m_msg;
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
protected:
	std::istream &m_in;
	char m_unpackBuff[1];
	ccpcp_unpack_context m_inCtx;
};

} // namespace chainpack
} // namespace shv

