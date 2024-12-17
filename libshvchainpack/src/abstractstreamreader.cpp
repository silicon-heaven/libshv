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
	: Super([&msg, &dump, err_code, pos](){
		auto res = std::string("Parse error: ")  + std::to_string(err_code) + " " + ccpcp_error_string(err_code)
				+ " at pos: " + std::to_string(pos)
				+ " - " + msg;
		if (!dump.empty()) {
			res += " near to:\n" + dump;
		}
		return res;
	}())
	, m_errCode(err_code)
	, m_pos(pos)
{
}

int ParseException::errCode() const
{
	return m_errCode;
}

long long ParseException::pos() const
{
	return m_pos;
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

int64_t AbstractStreamReader::readCount() const
{
	auto count = m_in.tellg();
	if (count >= 0) {
		count -= m_inCtx.end - m_inCtx.current;
	}
	return count;
}

} // namespace shv
