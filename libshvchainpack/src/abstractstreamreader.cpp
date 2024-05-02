#include <shv/chainpack/abstractstreamreader.h>

namespace shv::chainpack {

size_t unpack_underflow_handler(ccpcp_unpack_context *ctx)
{
	auto *rd = reinterpret_cast<AbstractStreamReader*>(ctx->custom_context);
	int c = rd->m_in.get();
	if(c < 0 || rd->m_in.eof()) {
		// id directory is open then c == -1 but eof() == false, strange
		return 0;
	}
	rd->m_unpackBuff = static_cast<char>(c);
	ctx->start = &rd->m_unpackBuff;
	ctx->current = ctx->start;
	ctx->end = ctx->start + 1;
	return 1;
}

ParseException::ParseException(int err_code, const std::string &msg, long long pos, const std::string &dump)
	: m_errCode(err_code), m_msg(msg), m_pos(pos)
{
	m_msg = std::string("Parse error: ")  + std::to_string(m_errCode) + " " + ccpcp_error_string(m_errCode)
			+ " at pos: " + std::to_string(m_pos)
			+ " - " + m_msg;
	if (!dump.empty()) {
		m_msg += " near to:\n" + dump;
	}
}

int ParseException::errCode() const
{
	return m_errCode;
}

long long ParseException::pos() const
{
	return m_pos;
}

const std::string& ParseException::msg() const
{
	return m_msg;
}

const char *ParseException::what() const noexcept
{
	 return m_msg.data();
}

AbstractStreamReader::AbstractStreamReader(std::istream &in)
	: m_in(in)
{
	// C++ implementation does not require container states stack
	ccpcp_unpack_context_init(&m_inCtx, &m_unpackBuff, 0, unpack_underflow_handler, nullptr);
	m_inCtx.custom_context = this;
}

AbstractStreamReader::~AbstractStreamReader() = default;

RpcValue AbstractStreamReader::read(std::string *error)
{
	RpcValue ret;
	if(error) {
		error->clear();
		try {
			read(ret);
		}
		catch (ParseException &e) {
			*error = e.what();
		}
	}
	else {
		read(ret);
	}
	return ret;
}

} // namespace shv
