#pragma once

#include "../shvcoreglobal.h"

#include "abstractshvjournal.h"
#include "patternmatcher.h"
#include "shvgetlogparams.h"

namespace shv::core::utils {

class SHVCORE_DECL_EXPORT ShvLogFilter
{
public:
	ShvLogFilter(const ShvGetLogParams &input_params);

	bool match(const ShvJournalEntry &entry) const;

protected:
	PatternMatcher m_patternMatcher;
	int64_t m_inputFilterSinceMsec = 0LL;
	int64_t m_inputFilterUntilMsec = 0LL;
};
}
