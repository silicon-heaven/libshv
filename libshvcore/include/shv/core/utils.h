#pragma once

#include "shvcoreglobal.h"
#include "stringview.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <vector>

// do while is to suppress of semicolon warning SHV_SAFE_DELETE(ptr);
#define SHV_SAFE_DELETE(x) do if(x != nullptr) {delete x; x = nullptr;} while(false)

#define SHV_QUOTE(x) #x
#define SHV_EXPAND_AND_QUOTE(x) SHV_QUOTE(x)

#define SHV_FIELD_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	protected: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(const ptype &val) { m_##lower_letter##name_rest = val; } \
	public: void set##upper_letter##name_rest(ptype &&val) { m_##lower_letter##name_rest = std::move(val); }

#define SHV_FIELD_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	protected: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(const ptype &val) { m_##lower_letter##name_rest = val; } \
	public: void set##upper_letter##name_rest(ptype &&val) { m_##lower_letter##name_rest = std::move(val); }
#define SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	protected: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(bool val) { m_##lower_letter##name_rest = val; }

#define SHV_FIELD_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)



#define SHV_FIELD_CMP_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	protected: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	} \
	public: bool set##upper_letter##name_rest(ptype &&val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = std::move(val); return true; } \
		return false; \
	}

#define SHV_FIELD_CMP_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	protected: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	} \
	public: bool set##upper_letter##name_rest(ptype &&val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = std::move(val); return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_CMP_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	protected: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(bool val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_CMP_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_CMP_IMPL2(lower_letter, upper_letter, name_rest, false)

namespace shv::core {

class SHVCORE_DECL_EXPORT Utils
{
public:
	static int versionStringToInt(const std::string &version_string);
	static std::string intToVersionString(int ver);

	std::string binaryDump(const std::string &bytes);
	static std::string toHex(const std::string &bytes);
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static std::string fromHex(const std::string &bytes);

	static shv::chainpack::RpcValue foldMap(const shv::chainpack::RpcValue::Map &plain_map, char key_delimiter = '.');

	static std::string simplifyPath(const std::string &p);

	static std::vector<char> readAllFd(int fd);

	template<typename T>
	static T getIntLE(const char *buff, unsigned len)
	{
		using uT = std::make_unsigned_t<T>;
		uT val = 0;
		for (unsigned i = len; i > 0; --i) {
			val = val * 256 + static_cast<unsigned char>(buff[i - 1]);
		}
		if constexpr (std::is_signed_v<T>) {
			if (len < sizeof(T)) {
				uT mask = ~0;
				for (unsigned i = 0; i < len; i++)
					mask <<= 8;
				val |= mask;
			}
		}
		return static_cast<T>(val);
	}
};

namespace utils {
enum class SplitBehavior {
	KeepEmptyParts,
	SkipEmptyParts
};

enum class QuoteBehavior {
	KeepQuotes,
	RemoveQuotes
};
SHVCORE_DECL_EXPORT StringViewList split(StringView strv, char delim, char quote = '\0', SplitBehavior split_behavior = SplitBehavior::SkipEmptyParts, QuoteBehavior quotes_behavior = QuoteBehavior::KeepQuotes);
StringViewList split(std::string&& strv, char delim, char quote = '\0', SplitBehavior split_behavior = SplitBehavior::SkipEmptyParts, QuoteBehavior quotes_behavior = QuoteBehavior::KeepQuotes) = delete;
SHVCORE_DECL_EXPORT StringView getToken(StringView strv, char delim = ' ', char quote = '\0');
SHVCORE_DECL_EXPORT StringView slice(StringView s, int start, int end);

SHVCORE_DECL_EXPORT std::string joinPath(const StringView &p1, const StringView &p2);

template <typename... Types>
void joinPath(const Types& ...pack)
{
	static_assert(sizeof...(pack) >= 2, "Can't use joinPath with less than two paths.");
}

template <typename FirstStringType, typename SecondStringType, typename... RestStringTypes>
std::string joinPath(const FirstStringType& head, const SecondStringType& second, const RestStringTypes& ...rest)
{
	std::string res = joinPath(std::string_view(head), std::string_view(second));
	((res = joinPath(std::string_view(res), std::string_view(rest))), ...);
	return res;
}

template <typename Container>
auto findLongestPrefix(const Container& cont, std::string value) -> typename std::remove_reference_t<decltype(cont)>::const_iterator
{
	while (true) {
		if (auto it = cont.find(value); it != cont.end()) {
			return it;
		}

		auto last_slash_pos = value.rfind('/');
		if (last_slash_pos == std::string::npos) {
			break;
		}
		value.erase(last_slash_pos);
	}

	return cont.end();
}
}

}

