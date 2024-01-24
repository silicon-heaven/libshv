#include <shv/core/utils/shvjournalcommon.h>

namespace shv::core::utils {

const char *ShvJournalCommon::Column::name(ShvJournalCommon::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::UpTime: return "upTime";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	case Column::Enum::Domain: return "domain";
	case Column::Enum::Course: return "course";
	}
	return "invalid";
}

} // namespace shv
