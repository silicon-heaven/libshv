#include <shv/core/stringview.h>
#include <shv/chainpack/compat.h>

namespace shv::core {

StringViewList::StringViewList() = default;

StringViewList::StringViewList(const std::vector<StringView> &o)
	: Super(o)
{
}

StringView StringViewList::value(ssize_t ix) const
{
	if(ix < 0)
		ix = length() + ix;
	if(ix < 0 || ix >= length())
		return StringView();
	return at(static_cast<size_t>(ix));
}

ssize_t StringViewList::indexOf(const std::string &str) const
{
	for (ssize_t i = 0; i < length(); ++i) {
		const StringView &sv = at(static_cast<size_t>(i));
		if(sv == str)
			return i;
	}
	return -1;
}

StringViewList StringViewList::mid(size_t start, size_t len) const
{
	auto end = (start + len) > size()? size(): start + len;
	return StringViewList(begin() + static_cast<long>(start), begin() + static_cast<long>(end));
}

bool StringViewList::startsWith(const StringViewList &lst) const
{
	size_t i;
	for (i = 0; i < size() && i < lst.size(); ++i) {
		if(!(this->at(i) == lst.at(i)))
			return false;
	}
	return i == lst.size();
}

namespace {
std::string join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const char delim)
{
	std::string ret;
	int i = 0;
	while(first != last) {
		if(i++ > 0)
			ret += delim;
		ret += *first;
		first++;
	}
	return ret;
}

std::string join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const std::string &delim)
{
	std::string ret;
	int i = 0;
	while(first != last) {
		if(i++ > 0)
			ret += delim;
		ret += *first;
		first++;
	}
	return ret;
}
}

std::string StringViewList::join(const char delim) const
{
	return ::shv::core::join(cbegin(), cend(), delim);
}

std::string StringViewList::join(const std::string &delim) const
{
	return ::shv::core::join(begin(), end(), delim);
}

StringViewList StringViewList::mid(size_t start) const
{
	return mid(start, size());
}

ssize_t StringViewList::length() const
{
	return static_cast<ssize_t>(size());
}
} // namespace shv::core
