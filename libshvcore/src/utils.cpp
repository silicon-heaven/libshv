#include <shv/core/utils.h>
#include <shv/core/log.h>
#include <shv/core/stringview.h>

#include <shv/chainpack/compat.h>
#include <shv/chainpack/utils.h>
#include <algorithm>
#include <cstring>
#include <sstream>

#ifndef _MSC_VER
#include <unistd.h>
#endif

using namespace shv::chainpack;

namespace shv::core {

namespace {
template<typename Out>
void split(const std::string &s, char delim, Out result)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}
}

int Utils::versionStringToInt(const std::string &version_string)
{
	int ret = 0;
	for(const auto &s : split(version_string, '.')) {
		int i = ::atoi(s.c_str());
		ret = 100 * ret + i;
	}
	return ret;
}

std::string Utils::intToVersionString(int ver)
{
	std::string ret;
	while(ver) {
		int i = ver % 100;
		ver /= 100;
		std::string s = std::to_string(i);
		if(ret.empty())
			ret = s;
		else
			ret = s + '.' + ret;
	}
	return ret;
}

using shv::chainpack::utils::hexNibble;

std::string Utils::toHex(const std::string &bytes)
{
	std::string ret;
	for (char byte : bytes) {
		auto b = static_cast<unsigned char>(byte);
		char h = static_cast<char>(b / 16);
		char l = b % 16;
		ret += hexNibble(h);
		ret += hexNibble(l);
	}
	return ret;
}

std::string Utils::toHex(const std::basic_string<uint8_t> &bytes)
{
	std::string ret;
	for (unsigned char b : bytes) {
		char h = static_cast<char>(b / 16);
		char l = b % 16;
		ret += hexNibble(h);
		ret += hexNibble(l);
	}
	return ret;
}

namespace {
inline char unhex_char(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return static_cast<char>(c - 'a' + 10);
	if(c >= 'A' && c <= 'F')
		return static_cast<char>(c - 'A' + 10);
	return char(0);
}
}

std::string Utils::fromHex(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ) {
		auto u = static_cast<unsigned char>(unhex_char(bytes[i++]));
		u = 16 * u;
		if(i < bytes.size())
			u += static_cast<unsigned char>(unhex_char(bytes[i++]));
		ret.push_back(static_cast<char>(u));
	}
	return ret;
}

namespace {
void create_key_val(RpcValue &map, const StringViewList &path, const RpcValue &val)
{
	if(path.empty())
		return;
	if(path.size() == 1) {
		map.set(std::string{path[static_cast<size_t>(path.length() - 1)]}, val);
	}
	else {
		auto key = std::string{path[0]};
		RpcValue mval = map.at(key);
		if(!mval.isMap())
			mval = RpcValue::Map();
		create_key_val(mval, path.mid(1), val);
		map.set(key, mval);
	}
}
}

RpcValue Utils::foldMap(const chainpack::RpcValue::Map &plain_map, char key_delimiter)
{
	RpcValue ret = RpcValue::Map();
	for(const auto &kv : plain_map) {
		auto key = kv.first;
		StringViewList lst = utils::split(key, key_delimiter, '\0');
		create_key_val(ret, lst, kv.second);
	}
	return ret;
}

std::string utils::joinPath(const StringView &p1, const StringView &p2)
{
	StringView sv1(p1);
	while(sv1.ends_with('/'))
		sv1 = sv1.substr(0, sv1.size() - 1);
	StringView sv2(p2);
	while(sv2.starts_with('/'))
		sv2 = sv2.substr(1);
	if(sv2.empty())
		return std::string{sv1};
	if(sv1.empty())
		return std::string{sv2};
	return std::string{sv1} + '/' + std::string{sv2};
}

StringView utils::getToken(StringView strv, char delim, char quote)
{
	if(strv.empty())
		return strv;
	bool in_quotes = false;
	for (size_t i = 0; i < strv.length(); ++i) {
		char c = strv.at(i);
		if(quote) {
			if(c == quote) {
				in_quotes = !in_quotes;
				continue;
			}
		}
		if(c == delim && !in_quotes)
			return strv.substr(0, i);
	}
	return strv;
}

StringViewList utils::split(StringView strv, char delim, char quote, SplitBehavior split_behavior, QuoteBehavior quotes_behavior)
{
	using namespace std;
	vector<StringView> ret;
	if(strv.empty())
		return ret;
	while(true) {
		StringView token = getToken(strv, delim, quote);
		if(split_behavior == SplitBehavior::KeepEmptyParts || !token.empty()) {
			if(quotes_behavior == QuoteBehavior::RemoveQuotes && token.size() >= 2 && token.at(0) == quote && token.at(token.size() - 1) == quote) {
				ret.push_back(token.substr(1, token.size() - 2));
			}
			else {
				ret.push_back(token);
			}
		}
		if(token.data() + token.size() >= strv.data() + strv.size())
			break;
		strv = strv.substr(token.length() + 1);
	}
	return ret;
}

StringView utils::slice(StringView s, int start, int end)
{
	start = std::clamp(start, 0, static_cast<int>(s.size()));
	end = static_cast<int>(s.size()) + end;
	end = std::clamp(end, 0, static_cast<int>(s.size()));
	end = std::max(end, start);
	return s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
}

std::string Utils::simplifyPath(const std::string &p)
{
	StringViewList ret;
	StringViewList lst = utils::split(std::string_view{p}, '/');
	for (auto s : lst) {
		if(s == ".")
			continue;
		if(s == "..") {
			if(!ret.empty())
				ret.pop_back();
			continue;
		}
		ret.push_back(s);
	}
	return ret.join('/');
}

#ifndef _MSC_VER
std::vector<char> Utils::readAllFd(int fd)
{
	/// will not work with blockong read !!!
	/// one possible solution for the blocking sockets, pipes, FIFOs, and tty's is
	/// ioctl(fd, FIONREAD, &n);
	static constexpr ssize_t CHUNK_SIZE = 1024;
	std::vector<char> ret;
	while(true) {
		size_t prev_size = ret.size();
		ret.resize(prev_size + CHUNK_SIZE);
		ssize_t n = ::read(fd, ret.data() + prev_size, CHUNK_SIZE);
		if(n < 0) {
			if(errno == EAGAIN) {
				if(prev_size && (prev_size % CHUNK_SIZE)) {
					shvError() << "no data available, returning so far read bytes:" << prev_size << "number of chunks:" << (prev_size/CHUNK_SIZE);
				}
				else {
					/// can happen if previous read returned exactly CHUNK_SIZE
				}
				ret.resize(prev_size);
				return ret;
			}

			shvError() << "error read fd:" << fd << std::strerror(errno);
			return std::vector<char>();
		}
		if(n < CHUNK_SIZE) {
			ret.resize(prev_size + static_cast<size_t>(n));
			break;
		}
#ifdef USE_IOCTL_FIONREAD
		else if(n == CHUNK_SIZE) {
			if(S_ISFIFO(mode) || S_ISSOCK(mode)) {
				if(::ioctl(fd, FIONREAD, &n) < 0) {
					shvError() << "error ioctl(FIONREAD) fd:" << fd << ::strerror(errno);
					return ret;
				}
				if(n == 0)
					return ret;
			}
		}
#endif
	}
	return ret;
}
#endif

namespace utils {
std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, CaseSensitivity case_sensitivity)
{
	auto it = std::search(
				  str_haystack.begin(), str_haystack.end(),
				  str_needle.begin(), str_needle.end(),
				  (case_sensitivity == CaseSensitivity::CaseInsensitive)
					? [](char a, char b) { return std::tolower(a) == std::tolower(b); }
					: [](char a, char b) { return a == b;}
	);
	return (it == str_haystack.end())? std::string::npos: static_cast<std::string::size_type>(it - str_haystack.begin());
}

std::string::size_type indexOf(const std::string &haystack, char needle)
{
	for (std::string::size_type i = 0; i < haystack.length(); i++)
		if(haystack[i] == needle)
			return i;
	return std::string::npos;
}

std::string mid(const std::string& str, size_t pos, size_t cnt)
{
	if(pos < str.size())
		return str.substr(pos, cnt);
	return {};
}

std::string& rtrim(std::string& s, const char* t)
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

std::string& ltrim(std::string& s, const char* t)
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

std::string& trim(std::string& s, const char* t)
{
	return ltrim(rtrim(s, t), t);
}

bool equal(std::string const& a, std::string const& b, CaseSensitivity case_sensitivity)
{
	if (a.length() == b.length()) {
		return std::equal(
					b.begin(), b.end(),
					a.begin(),
					(case_sensitivity == CaseSensitivity::CaseInsensitive) ?
						[](char x, char y) { return std::tolower(x) == std::tolower(y); }:
		[](char x, char y) { return x == y;}
		);
	}

	return false;
}

std::vector<std::string> split(const std::string &str, char delim, SplitBehavior split_behavior)
{
	using namespace std;
	vector<std::string> ret;
	size_t pos = 0;
	while(true) {
		size_t pos2 = str.find_first_of(delim, pos);
		std::string s = (pos2 == std::string::npos)? str.substr(pos): str.substr(pos, pos2 - pos);
		if(split_behavior == SplitBehavior::KeepEmptyParts || !s.empty())
			ret.push_back(s);
		if(pos2 == std::string::npos)
			break;
		pos = pos2 + 1;
	}
	return ret;
}

std::string join(const std::vector<std::string> &lst, const std::string &delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s;
	}
	return ret;
}

std::string join(const std::vector<std::string> &lst, char delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s;
	}
	return ret;
}

std::string join(const std::vector<StringView> &lst, char delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s;
	}
	return ret;
}

int replace(std::string &str, const std::string &from, const std::string &to)
{
	int i = 0;
	size_t pos = 0;
	for (i = 0; ; ++i) {
		pos = str.find(from, pos);
		if(pos == std::string::npos)
			break;
		str.replace(pos, from.length(), to);
		pos += to.length();
	}
	return i;
}

int replace(std::string& str, const char from, const char to)
{
	int n = 0;
	for (char& i : str) {
		if(i == from) {
			i = to;
			n++;
		}
	}
	return n;
}

std::string &upper(std::string &s)
{
	for (char& i : s)
		i = static_cast<char>(std::toupper(i));
	return s;
}

std::string toUpper(const std::string& s)
{
	std::string ret(s);
	return upper(ret);
}

std::string &lower(std::string &s)
{
	for (char& i : s)
		i = static_cast<char>(std::tolower(i));
	return s;
}

std::string toLower(const std::string& s)
{
	std::string ret(s);
	return lower(ret);
}

int toInt(const std::string &str, bool *ok)
{
	int ret = 0;
	bool is_ok = false;
	try {
		size_t pos;
		ret = std::stoi(str, &pos);
		if(pos == str.length())
			is_ok = true;
		else
			ret = 0;
	}
	catch (...) {
		ret = 0;
	}
	if(ok)
		*ok = is_ok;
	return ret;
}

double toDouble(const std::string &str, bool *ok)
{
	double ret = 0;
	bool is_ok = false;
	try {
		size_t pos;
		ret = std::stod(str, &pos);
		if(pos == str.length())
			is_ok = true;
		else
			ret = 0;
	}
	catch (...) {
		ret = 0;
	}
	if(ok)
		*ok = is_ok;
	return ret;
}

namespace {
size_t find_str(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &needle)
{
	using namespace std;
	if(needle.empty())
		return std::string::npos;
	if(begin_pos > haystack.size())
		return std::string::npos;
	if(end_pos > haystack.size())
		return std::string::npos;
	if(end_pos < begin_pos)
		return std::string::npos;
	if(needle.size() > (end_pos - begin_pos))
		return std::string::npos;
	auto pos1 = begin_pos;
	auto pos2 = end_pos - needle.size() + 1;
	while(pos1 < pos2) {
		unsigned i = 0;
		for(; i<needle.size(); ++i) {
			if(haystack[pos1 + i] != needle[i])
				break;
		}
		if(i == needle.size())
			return pos1;
		++pos1;
	}
	return std::string::npos;
}
}

std::pair<size_t, size_t> indexOfBrackets(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &open_bracket, const std::string &close_bracket)
{
	if(begin_pos + open_bracket.size() > haystack.size())
		return std::pair<size_t, size_t>(std::string::npos, std::string::npos);
	end_pos = std::min(end_pos, haystack.size());
	if(end_pos < begin_pos)
		return std::pair<size_t, size_t>(std::string::npos, std::string::npos);
	auto open0 = find_str(haystack, begin_pos, end_pos, open_bracket);
	auto open1 = open0;
	auto close1 = find_str(haystack, open1, end_pos, close_bracket);
	int nest_cnt = 0;
	while(true) {
		if(open1 == std::string::npos || close1 == std::string::npos)
			return std::pair<size_t, size_t>(open0, close1);
		open1 = find_str(haystack, open1 + open_bracket.size(), close1, open_bracket);
		if(open1 == std::string::npos) {
			// no opening bracket before closing, we have found balaced pair if nest_cnt == 0
			if(nest_cnt == 0)
				return std::pair<size_t, size_t>(open0, close1);
			nest_cnt--;
			open1 = close1;
			close1 = find_str(haystack, close1 + close_bracket.size(), end_pos, close_bracket);
		}
		else {
			nest_cnt++;
		}
	}
}
}
}
