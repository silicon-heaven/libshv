#include <shv/chainpack/abstractstreamwriter.h>

namespace shv::chainpack {

void pack_overflow_handler(ccpcp_pack_context *ctx, size_t size_hint)
{
	(void)size_hint;
	auto *wr = reinterpret_cast<AbstractStreamWriter*>(ctx->custom_context);
	while(ctx->start < ctx->current) {
		wr->m_out << *ctx->start++;
	}
	ctx->start = wr->m_packBuff;
	ctx->current = ctx->start;
}

AbstractStreamWriter::AbstractStreamWriter(std::ostream &out)
	: m_out(out)
{
	ccpcp_pack_context_init(&m_outCtx, m_packBuff, sizeof(m_packBuff), pack_overflow_handler);
	m_outCtx.custom_context = this;
}

AbstractStreamWriter::~AbstractStreamWriter()
{
	flush();
}

void AbstractStreamWriter::flush()
{
	if(m_outCtx.handle_pack_overflow)
		m_outCtx.handle_pack_overflow(&m_outCtx, 0);
}

} // namespace shv
