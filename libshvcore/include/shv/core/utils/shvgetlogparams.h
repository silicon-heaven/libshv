#ifndef SHV_CORE_UTILS_SHVGETLOGPARAMS_H
#define SHV_CORE_UTILS_SHVGETLOGPARAMS_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace core {
namespace utils {

struct SHVCORE_DECL_EXPORT ShvGetLogParams
{
	static constexpr auto KEY_HEADER_OPTIONS_DEPRECATED = "headerOptions";
	static constexpr auto KEY_MAX_RECORD_COUNT_DEPRECATED = "maxRecordCount";
	static constexpr auto KEY_RECORD_COUNT_LIMIT = "recordCountLimit";
	static constexpr auto KEY_WITH_SNAPSHOT = "withSnapshot";
	static constexpr auto KEY_WITH_PATHS_DICT = "withPathsDict";
	static constexpr auto KEY_WITH_TYPE_INFO = "withTypeInfo";
	static constexpr auto KEY_WITH_SINCE = "since";
	static constexpr auto KEY_WITH_UNTIL = "until";
	static constexpr auto KEY_PATH_PATTERN = "pathPattern";
	static constexpr auto KEY_PATH_PATTERN_TYPE = "pathPatternType";
	static constexpr auto KEY_DOMAIN_PATTERN = "domainPattern";

	static constexpr auto SINCE_LAST = "last";

	shv::chainpack::RpcValue since;
	shv::chainpack::RpcValue until;
	/// '*' and '**' wild-cards are supported
	/// '*' stands for single path segment, shv/pol/*/discon match shv/pol/ols/discon but not shv/pol/ols/depot/discon
	/// '**' stands for zero or more path segments, shv/pol/**/discon matches shv/pol/discon, shv/pol/ols/discon, shv/pol/ols/depot/discon
	/// std::regex::regex_math is checked for RegExp type
	std::string pathPattern;
	enum class PatternType {WildCard, RegEx};
	PatternType pathPatternType = PatternType::WildCard;
	static constexpr int DEFAULT_RECORD_COUNT_LIMIT = 1000;
	int recordCountLimit = DEFAULT_RECORD_COUNT_LIMIT;
	bool withSnapshot = false;
	std::string domainPattern; /// always regexp
	bool withTypeInfo = false;
	bool withPathsDict = true;

	ShvGetLogParams() = default;
	ShvGetLogParams(const shv::chainpack::RpcValue &opts);

	shv::chainpack::RpcValue toRpcValue(bool fill_legacy_fields = false) const;
	static ShvGetLogParams fromRpcValue(const shv::chainpack::RpcValue &v);

	bool isSinceLast() const;
};

} // namespace utils
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_UTILS_SHVGETLOGPARAMS_H
