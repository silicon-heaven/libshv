#include "exception.h"

#include <necrolog.h>

#include <cstdlib>

namespace shv::chainpack {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: Exception(_msg, _where, nullptr)
{
}

Exception::Exception(const RpcValue &_msg, const std::string &_where, const char *_log_topic)
	: m_msg(_msg)
	, m_where(_where)
{
	m_what = makeWhat();
	if(isAbortOnException() || !_where.empty() || (_log_topic && *_log_topic)) {
		if(_log_topic && *_log_topic)
			nCError(_log_topic) << "SHV_EXCEPTION:" << _where << m_what;
		else
			nError() << "SHV_EXCEPTION:" << _where << m_what;
	}
	if(isAbortOnException())
		std::abort();
}

std::string Exception::message() const
{
	return m_what;
}

const char *Exception::what() const noexcept
{
	return m_what.c_str();
}

std::string Exception::makeWhat() const
{
	if(m_msg.isValid()) {
		if(m_msg.isString())
			return m_msg.asString();
		return m_msg.toCpon();
	}
	return {};
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
