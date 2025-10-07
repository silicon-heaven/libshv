#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

#include <limits>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>

namespace shv::chainpack {

class RpcValue;

namespace utils {

SHVCHAINPACK_DECL_EXPORT std::string hexArray(const char *bytes, size_t n);
SHVCHAINPACK_DECL_EXPORT std::string hexDump(const char *bytes, size_t n);
SHVCHAINPACK_DECL_EXPORT std::string hexDump(const std::string_view &bytes);
SHVCHAINPACK_DECL_EXPORT std::string byteToHex( uint8_t i );
SHVCHAINPACK_DECL_EXPORT void byteToHex( std::array<char, 2> &arr, uint8_t i );

template <typename I>
std::string intToHex(I n)
{
	constexpr auto N = sizeof(I) * 2;
	std::string ret(N, '0');
	size_t ix = N;
	std::array<char, 2> arr;
	while(n > 0) {
		auto b = static_cast<uint8_t>(n);
		byteToHex(arr, b);
		ret[--ix] = arr[1];
		ret[--ix] = arr[0];
		n >>= 8;
	}
	return ret;
}

}

class SHVCHAINPACK_DECL_EXPORT Utils
{
public:
	std::string binaryDump(const std::string &bytes);
	static std::string toHexElided(const std::string &bytes, size_t start_pos, size_t max_len = 0);
	static std::string toHex(const std::string &bytes, size_t start_pos = 0, size_t length = std::numeric_limits<size_t>::max());
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static char fromHex(char c);
	static std::string fromHex(const std::string &bytes);

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

namespace utils {
char SHVCHAINPACK_DECL_EXPORT hexNibble(char i);
}

} // namespace shv::chainpack


#define SHV_IMAP_FIELD_IMPL(ptype, int_key, getter_prefix, setter_prefix, name_rest) \
	SHV_IMAP_FIELD_IMPL2(ptype, int_key, getter_prefix, setter_prefix, name_rest, shv::chainpack::RpcValue())

#define SHV_IMAP_FIELD_IMPL2(ptype, int_key, getter_prefix, setter_prefix, name_rest, default_value) \
	public: bool getter_prefix##name_rest##_isset() const {return has(static_cast<shv::chainpack::RpcValue::Int>(int_key));} \
	public: ptype getter_prefix##name_rest() const {return at(static_cast<shv::chainpack::RpcValue::Int>(int_key), default_value).to<ptype>();} \
	public: shv::chainpack::RpcValue& setter_prefix##name_rest(const ptype &val) {set(static_cast<shv::chainpack::RpcValue::Int>(int_key), shv::chainpack::RpcValue(val)); return *this;}

