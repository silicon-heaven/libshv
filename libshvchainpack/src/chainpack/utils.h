#pragma once

#include "../shvchainpackglobal.h"

#include <limits>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

#if defined LIBC_NEWLIB || defined ANDROID_BUILD
#include <sstream>
#endif

namespace shv {
namespace chainpack {

class RpcValue;

class SHVCHAINPACK_DECL_EXPORT Utils
{
public:
	static std::string removeJsonComments(const std::string &json_str);

	std::string binaryDump(const std::string &bytes);
	static std::string toHexElided(const std::string &bytes, size_t start_pos, size_t max_len = 0);
	static std::string toHex(const std::string &bytes, size_t start_pos = 0, size_t length = std::numeric_limits<size_t>::max());
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static char fromHex(char c);
	static std::string fromHex(const std::string &bytes);
	static std::string hexDump(const std::string &bytes);

	template<typename T>
	static std::string toString(T i, size_t prepend_0s_to_len = 0)
	{
#if defined LIBC_NEWLIB || defined ANDROID_BUILD
		std::ostringstream ss;
		ss << i;
		std::string ret = ss.str();
#else
		std::string ret = std::to_string(i); //not supported by newlib
#endif
		while(ret.size() < prepend_0s_to_len)
			ret = '0' + ret;
		return ret;
	}

	static RpcValue mergeMaps(const RpcValue &value_base, const RpcValue &value_over);

	template<typename M>
	static std::vector<std::string> mapKeys(const M &m, bool sorted = true)
	{
		std::vector<std::string> ret;
		ret.reserve(m.size());
		for(const auto &kv : m)
			ret.push_back(kv.first);
		if(sorted)
			std::sort(ret.begin(), ret.end());
		return ret;
	}
};


} // namespace chainpack
} // namespace shv

//#define SHV_QUOTE(x) #x

#define SHV_IMAP_FIELD_IMPL(ptype, int_key, getter_prefix, setter_prefix, name_rest) \
	SHV_IMAP_FIELD_IMPL2(ptype, int_key, getter_prefix, setter_prefix, name_rest, shv::chainpack::RpcValue())

#define SHV_IMAP_FIELD_IMPL2(ptype, int_key, getter_prefix, setter_prefix, name_rest, default_value) \
	public: bool getter_prefix##name_rest##_isset() const {return has(static_cast<shv::chainpack::RpcValue::Int>(int_key));} \
	public: ptype getter_prefix##name_rest() const {return rpcvalue_cast<ptype>(at(static_cast<shv::chainpack::RpcValue::Int>(int_key), default_value));} \
	public: shv::chainpack::RpcValue& setter_prefix##name_rest(const ptype &val) {set(static_cast<shv::chainpack::RpcValue::Int>(int_key), shv::chainpack::RpcValue(val)); return *this;}

