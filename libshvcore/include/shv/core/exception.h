#pragma once

#include <shv/core/shvcoreglobal.h>
#include <shv/core/utils.h>

#include <stdexcept>
#include <string>

#include <shv/chainpack/exception.h>

namespace shv::core {

class SHVCORE_DECL_EXPORT Exception : public shv::chainpack::Exception
{
	using Super = shv::chainpack::Exception;
public:
	using Super::Super;
};

}

#define SHV_EXCEPTION(msg) throw shv::core::Exception(msg, std::string(__FILE__) + ":" + std::to_string(__LINE__))
#define SHV_EXCEPTION_V(msg, topic) throw shv::core::Exception(msg, std::string(__FILE__) + ":" + std::to_string(__LINE__), topic)
