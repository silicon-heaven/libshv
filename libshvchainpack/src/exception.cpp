#include <shv/chainpack/exception.h>

#include <necrolog.h>

#include <cstdlib>

namespace shv::chainpack {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: Exception(_msg, {}, _where, {})
{
}

Exception::Exception(const std::string& _msg, const std::string& _where, const char *_log_topic)
	: Exception(_msg, {}, _where, _log_topic)
{
}

Exception::Exception(const std::string &msg, const RpcValue &data, const std::string &where, const char *log_topic)
	: Super([&msg, &data](){
		if(data.isValid()) {
			return msg + " data: " + data.toCpon();
		}
		return msg;
	}())
	, m_msg(msg)
	, m_data(data)
	, m_where(where)
{
	if(isAbortOnException() || !where.empty() || (log_topic && *log_topic)) {
		if(log_topic && *log_topic)
			nCError(log_topic) << "SHV_EXCEPTION:" << where << what();
		else
			nError() << "SHV_EXCEPTION:" << where << what();
	}
	if(isAbortOnException())
		std::abort();
}

std::string Exception::message() const
{
	return m_msg;
}

std::string Exception::where() const
{
	return m_where;
}

shv::chainpack::RpcValue Exception::data() const
{
	return m_data;
}

void Exception::Exception::setAbortOnException(bool on)
{
	s_abortOnException = on;
}

bool Exception::Exception::isAbortOnException()
{
	return s_abortOnException;
}

} // namespace shv
