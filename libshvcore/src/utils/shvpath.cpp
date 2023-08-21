#include "shvpath.h"
#include "../utils.h"

#include "../stringview.h"

namespace shv::core::utils {

bool shvpath::startsWithPath(const std::string_view &str, const std::string_view &path, size_t *pos)
{
	auto set_pos = [pos](size_t val, bool ret_val) -> bool {
		if(pos)
			*pos = val;
		return ret_val;
	};
	if(path.empty())
		return set_pos(0, true);
	if(str.starts_with(path)) {
		if(str.size() == path.size())
			return set_pos(str.size(), true);
		if(str[path.size()] == ShvPath::SHV_PATH_DELIM)
			return set_pos(path.size() + 1, true);
		if(path[path.size() - 1] == ShvPath::SHV_PATH_DELIM) // path contains trailing /
			return set_pos(path.size(), true);
	}
	return set_pos(std::string::npos, false);
}

ShvPath::ShvPath()
	: Super()
{
}

ShvPath::ShvPath(shv::core::String &&o)
	: Super(std::move(o))
{
}

ShvPath::ShvPath(const shv::core::String &o)
	: Super(o)
{
}

const std::string &ShvPath::asString() const
{
	return *this;
}

bool ShvPath::startsWithPath(const StringView &path, size_t *pos) const
{
	return startsWithPath(*this, path, pos);
}

bool ShvPath::startsWithPath(const StringView &str, const StringView &path, size_t *pos)
{
	return shvpath::startsWithPath(str, path, pos);
}

ShvPath ShvPath::appendPath(const StringView &path) const
{
	return shv::core::utils::joinPath(*this, path);
}

core::StringViewList ShvPath::splitPath(const shv::core::StringView &shv_path)
{
	return core::utils::split(StringView{shv_path}, SHV_PATH_DELIM, SHV_PATH_QUOTE, core::utils::SplitBehavior::SkipEmptyParts, core::utils::QuoteBehavior::RemoveQuotes);
}

StringView ShvPath::firstDir(const StringView &shv_path, size_t *next_dir_pos)
{
	StringView dir = core::utils::getToken(shv_path, SHV_PATH_DELIM, SHV_PATH_QUOTE);
	size_t next_pos = dir.size() + 1;
	if(next_pos > shv_path.size())
		next_pos = shv_path.size();
	if(dir.size() >= 2 && dir.at(0) == SHV_PATH_QUOTE && dir.at(dir.size() - 1) == SHV_PATH_QUOTE) {
		dir = dir.substr(1, dir.size() - 2);
	}
	if(next_dir_pos)
		*next_dir_pos = next_pos;
	return dir;
}

StringView ShvPath::takeFirsDir(StringView &shv_path)
{
	size_t next_dir_pos;
	StringView first = firstDir(shv_path, &next_dir_pos);
	shv_path = shv_path.substr(next_dir_pos);
	return first;
}

ShvPath ShvPath::joinDirs(const std::vector<std::string> &dirs)
{
	ShvPath ret;
	for(const std::string &s : dirs) {
		if(s.empty())
			continue;
		bool need_quotes = false;
		if(s.find(SHV_PATH_DELIM) != std::string::npos)
			need_quotes = true;
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
		ret += s;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
	}
	return ret;
}

ShvPath ShvPath::joinDirs(const StringViewList &dirs)
{
	return joinDirs(dirs.cbegin(), dirs.cend());
}

static bool need_quotes(const StringView &dir)
{
	if(dir.find(ShvPath::SHV_PATH_DELIM) != std::string_view::npos) {
		if(dir.size() >= 2 && dir.at(0) == ShvPath::SHV_PATH_QUOTE && dir.at(dir.size() - 1) == ShvPath::SHV_PATH_QUOTE)
			return false;
		return true;
	}
	return false;
}

ShvPath ShvPath::joinDirs(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last)
{
	ShvPath ret;
	for(auto it = first; it != last; ++it) {
		if(it->empty())
			continue;
		bool quotes = need_quotes(*it);
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(quotes)
			ret += SHV_PATH_QUOTE;
		ret += std::string{*it};
		if(quotes)
			ret += SHV_PATH_QUOTE;
	}
	return ret;
}

ShvPath ShvPath::joinDirs(StringView dir1, StringView dir2)
{
	std::vector<StringView> array = {dir1, dir2};
	return joinDirs(array);
}

ShvPath ShvPath::appendDir(StringView path1, StringView dir)
{
	while(path1.ends_with('/'))
		path1 = path1.substr(0, path1.size() - 1);
	if(dir.empty())
		return std::string{path1};
	bool quotes = need_quotes(dir);
	std::string dir_str = std::string{dir};
	if(quotes)
		dir_str = SHV_PATH_QUOTE + dir_str + SHV_PATH_QUOTE;
	if(path1.empty())
		return dir_str;
	return std::string{path1} + SHV_PATH_DELIM + dir_str;
}

ShvPath ShvPath::appendDir(StringView dir) const
{
	return appendDir(*this, dir);
}

StringViewList ShvPath::split(const StringView &shv_path)
{
	return splitPath(shv_path);
}

bool ShvPath::matchWild(const std::string &pattern) const
{
	const shv::core::StringViewList ptlst = shv::core::utils::split(pattern, SHV_PATH_DELIM);
	return matchWild(ptlst);
}

bool ShvPath::matchWild(const shv::core::StringViewList &pattern_lst) const
{
	const shv::core::StringViewList path_lst = shv::core::utils::split(*this, SHV_PATH_DELIM);
	return matchWild(path_lst, pattern_lst);
}

bool ShvPath::matchWild(const core::StringViewList &path_lst, const core::StringViewList &pattern_lst)
{
	size_t ptix = 0;
	size_t phix = 0;
	while(true) {
		if(phix == path_lst.size() && ptix == pattern_lst.size())
			return true;
		if(ptix == pattern_lst.size() && phix < path_lst.size())
			return false;
		if(phix == path_lst.size() && ptix == pattern_lst.size() - 1 && pattern_lst[ptix] == "**")
			return true;
		if(phix == path_lst.size() && ptix < pattern_lst.size())
			return false;
		const shv::core::StringView &pt = pattern_lst[ptix];
		if(pt == "*") {
			// match exactly one path segment
		}
		else if(pt == "**") {
			// match zero or more path segments
			ptix++;
			if(ptix == pattern_lst.size())
				return true;
			const shv::core::StringView &pt2 = pattern_lst[ptix];
			do {
				const shv::core::StringView &ph = path_lst[phix];
				if(ph == pt2)
					break;
				phix++;
			} while(phix < path_lst.size());
			if(phix == path_lst.size())
				return false;
		}
		else {
			const shv::core::StringView &ph = path_lst[phix];
			if(!(ph == pt))
				return false;
		}
		ptix++;
		phix++;
	}
}

}

