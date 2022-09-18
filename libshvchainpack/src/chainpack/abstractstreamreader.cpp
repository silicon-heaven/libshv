#include "abstractstreamreader.h"

namespace shv {
namespace chainpack {

size_t unpack_underflow_handler(ccpcp_unpack_context *ctx)
{
	AbstractStreamReader *rd = reinterpret_cast<AbstractStreamReader*>(ctx->custom_context);
	int c = rd->m_in.get();
	if(c < 0 || rd->m_in.eof()) {
		// id directory is open then c == -1 but eof() == false, strange
		return 0;
	}
	rd->m_unpackBuff[0] = (char)c;
	ctx->start = rd->m_unpackBuff;
	ctx->current = ctx->start;
	ctx->end = ctx->start + 1;
	return 1;
}

const char *ParseException::what() const noexcept
{
	 return m_msg.data();
}

AbstractStreamReader::AbstractStreamReader(std::istream &in)
	: m_in(in)
{
	// C++ implementation does not require container states stack
	//ccpcp_container_stack_init(&m_containerStack, m_containerStates, CONTAINER_STATE_CNT, NULL);
	//ccpcp_unpack_context_init(&m_inCtx, m_unpackBuff, 0, unpack_underflow_handler, &m_containerStack);
	ccpcp_unpack_context_init(&m_inCtx, m_unpackBuff, 0, unpack_underflow_handler, nullptr);
	m_inCtx.custom_context = this;
}

AbstractStreamReader::~AbstractStreamReader()
{
}

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

} // namespace chainpack
} // namespace shv
