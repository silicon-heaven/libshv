#include <shv/core/utils/patternmatcher.h>
#include <shv/core/utils/shvpath.h>

#include <shv/core/log.h>
#include <shv/core/utils.h>

#define logWShvJournal() shvCWarning("ShvJournal")

namespace shv::core::utils {

PatternMatcher::PatternMatcher(const ShvGetLogParams &filter)
{
	shvLogFuncFrame() << "params:" << filter.toRpcValue().toCpon();
	if(!filter.pathPattern.empty()) {
		shvDebug() << "path pattern:" << filter.pathPattern;
		if(filter.pathPatternType == ShvGetLogParams::PatternType::RegEx) {
			shvDebug() << "\t regex";
			try {
				m_pathPatternRegEx = std::regex(filter.pathPattern);
				shvDebug() << "\t\t OK";
				m_usePathPatternRegEx = true;
			}
			catch (const std::regex_error &e) {
				m_regexError = true;
				logWShvJournal() << "Invalid path pattern regex:" << filter.pathPattern << " - " << e.what();
			}
		}
		else {
			shvDebug() << "\t wildcard";
			m_pathPatternWildCard = filter.pathPattern;
			shvDebug() << "\t\t OK";
		}
	}
	if(!filter.domainPattern.empty()) try {
		shvDebug() << "domain pattern:" << filter.domainPattern;
		shvDebug() << "\t regex";
		m_domainPatternRegEx = std::regex(filter.domainPattern);
		shvDebug() << "\t\t OK";
		m_useDomainPatternregEx = true;
	}
	catch (const std::regex_error &e) {
		m_regexError = true;
		logWShvJournal() << "Invalid domain pattern regex:" << filter.domainPattern << " - " << e.what();
	}
}

bool PatternMatcher::isEmpty() const
{
	return !isRegexError() && !m_usePathPatternRegEx && m_pathPatternWildCard.empty() && !m_useDomainPatternregEx;
}

bool PatternMatcher::isRegexError() const
{
	return  m_regexError;
}

bool PatternMatcher::match(const ShvJournalEntry &entry) const
{
	return match(entry.path, entry.domain);
}

bool PatternMatcher::match(const std::string &path, const std::string &domain) const
{
	if(isEmpty()) {
		return true;
	}
	if(m_usePathPatternRegEx) {
		std::smatch cmatch;
		if(!std::regex_search(path, cmatch, m_pathPatternRegEx))
			return false;
	}
	else if(!m_pathPatternWildCard.empty()) {
		const shv::core::StringViewList path_lst = shv::core::utils::ShvPath::split(path);
		const shv::core::StringViewList pattern_lst = shv::core::utils::split(std::string_view{m_pathPatternWildCard}, '/');
		if(!ShvPath::matchWild(path_lst, pattern_lst))
			return false;
	}
	if(m_useDomainPatternregEx) {
		return std::regex_match(domain, m_domainPatternRegEx);
	}
	return true;
}

}

