#pragma once

#include "shvcoreglobal.h"
#include "stringview.h"

#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <limits>

namespace shv::core {

class SHVCORE_DECL_EXPORT String : public std::string
{
	using Super = std::string;
public:
	enum CaseSensitivity {CaseSensitive, CaseInsensitive};
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	using Super::Super;
	String();
	String(const std::string &o);
	String(std::string &&o);

	static bool equal(const std::string &a, const std::string &b, String::CaseSensitivity case_sensitivity);
	static std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity = CaseSensitive);
	static std::string::size_type indexOf(const std::string &haystack, char needle);
	std::string::size_type indexOf(const std::string &needle, String::CaseSensitivity case_sensitivity = CaseSensitive) const;
	std::string::size_type indexOf(char needle) const;
	size_t lastIndexOf(char c) const;

	static constexpr auto WhiteSpaceChars = " \t\n\r\f\v";
	static std::string& rtrim(std::string& s, const char* t = WhiteSpaceChars);
	static std::string& ltrim(std::string& s, const char* t = WhiteSpaceChars);
	static std::string& trim(std::string& s, const char* t = WhiteSpaceChars);

	std::string mid(size_t pos, size_t cnt = std::string::npos) const;

	static std::pair<size_t, size_t> indexOfBrackets(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &open_bracket, const std::string &close_bracket);
	static std::vector<std::string> split(const std::string &str, char delim, SplitBehavior split_behavior = SkipEmptyParts);
	static std::string join(const std::vector<std::string> &lst, const std::string &delim);
	static std::string join(const std::vector<std::string> &lst, char delim);
	static std::string join(const std::vector<shv::core::StringView> &lst, char delim);
	static int replace(std::string &str, const std::string &from, const std::string &to);
	static int replace(std::string &str, const char from, const char to);

	static std::string& upper(std::string& s);
	static std::string toUpper(const std::string& s);
	static std::string& lower(std::string& s);
	static std::string toLower(const std::string& s);

	template<typename T>
	static std::string toString(T i, size_t width = 0, char fill_char = ' ')
	{
		std::ostringstream ss;
		ss << i;
		std::string ret = ss.str();
		while(ret.size() < width)
			ret = fill_char + ret;
		return ret;
	}
	static int toInt(const std::string &str, bool *ok = nullptr);
	static double toDouble(const std::string &str, bool *ok);

};

}
