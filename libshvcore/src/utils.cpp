#include <shv/core/utils.h>
#include <shv/core/log.h>
#include <shv/core/stringview.h>

#include <shv/chainpack/compat.h>
#include <shv/chainpack/utils.h>
#include <cstring>
#include <regex>
#include <sstream>

using namespace std;
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
		string key = std::string{path[0]};
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
		string key = kv.first;
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
	if(start < 0)
		start = 0;
	if(start > static_cast<int>(s.size()))
		start = static_cast<int>(s.size());
	end = static_cast<int>(s.size()) + end;
	if(end < 0)
		end = 0;
	if(end > static_cast<int>(s.size()))
		end = static_cast<int>(s.size());
	if(end < start)
		end = start;
	return s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
}

std::string Utils::simplifyPath(const std::string &p)
{
	StringViewList ret;
	StringViewList lst = utils::split(p, '/');
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
}
