#include "exception.h"

#include <necrolog.h>

#include <cstdlib>

namespace shv::chainpack {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: Exception(_msg, {}, _where, {})
{
}

Exception::Exception(const std::string &_msg, const RpcValue &_data, const std::string &_where, const char *_log_topic)
	: m_msg(_msg)
	, m_data(_data)
	, m_where(_where)
{
	makeWhat();
	if(isAbortOnException() || !_where.empty() || (_log_topic && *_log_topic)) {
		if(_log_topic && *_log_topic)
			nCError(_log_topic) << "SHV_EXCEPTION:" << _where << m_what;
		else
			nError() << "SHV_EXCEPTION:" << _where << m_what;
	}
	if(isAbortOnException())
		std::abort();
}

const char *Exception::what() const noexcept
{
	return m_what.c_str();
}

void Exception::Exception::setAbortOnException(bool on)
{
	s_abortOnException = on;
}

bool Exception::Exception::isAbortOnException()
{
	return s_abortOnException;
}

void Exception::makeWhat()
{
	m_what = m_msg;
	if(m_data.isValid()) {
		m_what += " data: " + m_data.toCpon();
	}
}

} // namespace shv
