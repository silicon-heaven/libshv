#pragma once

#include "../shvcoreglobal.h"

#include "shvgetlogparams.h"
#include "shvjournalentry.h"

#include <regex>

namespace shv::core::utils {

class SHVCORE_DECL_EXPORT PatternMatcher
{
public:
	PatternMatcher() = default;
	PatternMatcher(const ShvGetLogParams &filter);
	bool isEmpty() const;
	bool isRegexError() const;
	bool match(const ShvJournalEntry &entry) const;
	bool match(const std::string &path, const std::string &domain) const;

private:
	std::regex m_pathPatternRegEx;
	bool m_usePathPatternRegEx = false;
	std::string m_pathPatternWildCard;

	std::regex m_domainPatternRegEx;
	bool m_useDomainPatternregEx = false;

	bool m_regexError = false;
};
}
