#pragma once

#include <shv/core/shvcore_export.h>
#include <shv/core/utils.h>

#include <shv/chainpack/exception.h>

namespace shv::core {

class LIBSHVCORE_EXPORT Exception : public shv::chainpack::Exception
{
	using Super = shv::chainpack::Exception;
public:
	using Super::Super;
};

}

#define SHV_EXCEPTION(msg) throw shv::core::Exception(msg, std::string(__FILE__) + ":" + std::to_string(__LINE__))
#define SHV_EXCEPTION_V(msg, topic) throw shv::core::Exception(msg, std::string(__FILE__) + ":" + std::to_string(__LINE__), topic)
