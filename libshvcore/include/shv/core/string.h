#pragma once

#include <shv/core/shvcoreglobal.h>
#include <shv/core/stringview.h>

#include <sstream>
#include <string>
#include <vector>

namespace shv::core::string {
	enum CaseSensitivity {CaseSensitive, CaseInsensitive};
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	SHVCORE_DECL_EXPORT bool equal(const std::string &a, const std::string &b, CaseSensitivity case_sensitivity);
	SHVCORE_DECL_EXPORT std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, CaseSensitivity case_sensitivity = CaseSensitive);
	SHVCORE_DECL_EXPORT std::string::size_type indexOf(const std::string &haystack, char needle);

	constexpr auto WhiteSpaceChars = " \t\n\r\f\v";
	SHVCORE_DECL_EXPORT std::string& rtrim(std::string& s, const char* t = WhiteSpaceChars);
	SHVCORE_DECL_EXPORT std::string& ltrim(std::string& s, const char* t = WhiteSpaceChars);
	SHVCORE_DECL_EXPORT std::string& trim(std::string& s, const char* t = WhiteSpaceChars);

	SHVCORE_DECL_EXPORT std::string mid(const std::string& str, size_t pos, size_t cnt = std::string::npos);

	SHVCORE_DECL_EXPORT std::pair<size_t, size_t> indexOfBrackets(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &open_bracket, const std::string &close_bracket);
	SHVCORE_DECL_EXPORT std::vector<std::string> split(const std::string &str, char delim, SplitBehavior split_behavior = SkipEmptyParts);
	SHVCORE_DECL_EXPORT std::string join(const std::vector<std::string> &lst, const std::string &delim);
	SHVCORE_DECL_EXPORT std::string join(const std::vector<std::string> &lst, char delim);
	SHVCORE_DECL_EXPORT std::string join(const std::vector<shv::core::StringView> &lst, char delim);
	SHVCORE_DECL_EXPORT int replace(std::string &str, const std::string &from, const std::string &to);
	SHVCORE_DECL_EXPORT int replace(std::string &str, const char from, const char to);

	SHVCORE_DECL_EXPORT std::string& upper(std::string& s);
	SHVCORE_DECL_EXPORT std::string toUpper(const std::string& s);
	SHVCORE_DECL_EXPORT std::string& lower(std::string& s);
	SHVCORE_DECL_EXPORT std::string toLower(const std::string& s);

	template<typename T>
	std::string toString(T i, size_t width = 0, char fill_char = ' ')
	{
		std::ostringstream ss;
		ss << i;
		std::string ret = ss.str();
		while(ret.size() < width)
			ret = fill_char + ret;
		return ret;
	}
	SHVCORE_DECL_EXPORT int toInt(const std::string &str, bool *ok = nullptr);
	SHVCORE_DECL_EXPORT double toDouble(const std::string &str, bool *ok);

}
