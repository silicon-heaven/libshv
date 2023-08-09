#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"
#include "utils.h"

#include <stdexcept>
#include <string>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Exception : public std::exception
{
public:
	static constexpr bool Throw = true;
public:
	Exception(const std::string& _msg, const std::string& _where = std::string());
	Exception(const shv::chainpack::RpcValue& _msg, const std::string& _where, const char *_log_topic);
	~Exception() override = default;
public:
	virtual std::string message() const;
	virtual std::string where() const {return m_where;}
	const char* what() const noexcept override;
	static void setAbortOnException(bool on);
	static bool isAbortOnException();
protected:
	std::string makeWhat() const;
protected:
	static bool s_abortOnException;
	shv::chainpack::RpcValue m_msg;
	std::string m_what;
	std::string m_where;
};

}}

#define SHVCHP_EXCEPTION(e) throw shv::chainpack::Exception(e, std::string(__FILE__) + ":" + shv::chainpack::Utils::toString(__LINE__))
#define SHVCHP_EXCEPTION_V(msg, topic) throw shv::core::Exception(msg, std::string(__FILE__) + ":" + shv::core::Utils::toString(__LINE__), topic)
