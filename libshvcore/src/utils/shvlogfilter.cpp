#include <shv/core/utils/shvlogfilter.h>

namespace shv::core::utils {

ShvLogFilter::ShvLogFilter(const ShvGetLogParams &input_params)
	: m_patternMatcher(input_params)
{
	if(input_params.since.isDateTime())
		m_inputFilterSinceMsec = input_params.since.toDateTime().msecsSinceEpoch();
	if(input_params.until.isDateTime())
		m_inputFilterUntilMsec = input_params.until.toDateTime().msecsSinceEpoch();
}

bool ShvLogFilter::match(const ShvJournalEntry &entry) const
{
	if (m_inputFilterSinceMsec && entry.epochMsec < m_inputFilterSinceMsec) {
		return false;
	}
	if (m_inputFilterUntilMsec && entry.epochMsec > m_inputFilterUntilMsec) {
		return false;
	}
	return m_patternMatcher.match(entry);
}

}

