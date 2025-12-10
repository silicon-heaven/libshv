#pragma once

#include <shv/core/shvcore_export.h>
#include <shv/chainpack/compat.h>

#include <string>
#include <vector>

namespace shv::core {

using StringView = std::string_view;
class LIBSHVCORE_EXPORT StringViewList : public std::vector<StringView>
{
	using Super = std::vector<StringView>;
public:
	using Super::Super;
	StringViewList();
	StringViewList(const std::vector<StringView> &o);

	StringView value(ssize_t ix) const;
	ssize_t indexOf(const std::string &str) const;

	StringViewList mid(size_t start) const;
	StringViewList mid(size_t start, size_t len) const;

	std::string join(const char delim) const;
	std::string join(const std::string &delim) const;

	bool startsWith(const StringViewList &lst) const;
	ssize_t length() const;
};

} // namespace shv::core

