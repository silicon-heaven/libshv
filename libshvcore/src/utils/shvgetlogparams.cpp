#include "shvgetlogparams.h"

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

const char *ShvGetLogParams::KEY_HEADER_OPTIONS_DEPRECATED = "headerOptions";
const char *ShvGetLogParams::KEY_MAX_RECORD_COUNT_DEPRECATED = "maxRecordCount";
const char *ShvGetLogParams::KEY_RECORD_COUNT_LIMIT = "recordCountLimit";
const char *ShvGetLogParams::KEY_WITH_SNAPSHOT = "withSnapshot";
const char *ShvGetLogParams::KEY_WITH_PATHS_DICT = "withPathsDict";

//const char *ShvGetLogParams::KEY_WITH_UPTIME = "withUptime";
static const char *KEY_WITH_TYPE_INFO = "withTypeInfo";

const char *ShvGetLogParams::KEY_WITH_SINCE = "since";
const char *ShvGetLogParams::KEY_WITH_UNTIL = "until";
const char *ShvGetLogParams::KEY_PATH_PATTERN_TYPE = "pathPatternType";
const char *ShvGetLogParams::KEY_PATH_PATTERN = "pathPattern";
const char *ShvGetLogParams::KEY_DOMAIN_PATTERN = "domainPattern";

static const char *SINCE_NOW = "now";
const char *ShvGetLogParams::SINCE_LAST = "last";

static const char REG_EX[] = "regex";

ShvGetLogParams::ShvGetLogParams(const chainpack::RpcValue &opts)
	: ShvGetLogParams()
{
	*this = fromRpcValue(opts);
}

enum HeaderOptions : unsigned {
	BasicInfo = 1 << 0,
	FieldInfo = 1 << 1,
	TypeInfo = 1 << 2,
	PathsDict = 1 << 3,
	CompleteInfo = BasicInfo | FieldInfo | TypeInfo | PathsDict,
};

chainpack::RpcValue ShvGetLogParams::toRpcValue( bool fill_legacy_fields ) const
{
	cp::RpcValue::Map m;
	if(since.isValid())
		m[KEY_WITH_SINCE] = since;
	if(until.isValid())
		m[KEY_WITH_UNTIL] = until;
	if(!pathPattern.empty()) {
		m[KEY_PATH_PATTERN] = pathPattern;
		if(pathPatternType == PatternType::RegEx)
			m[KEY_PATH_PATTERN_TYPE] = REG_EX;
	}
	if(!domainPattern.empty())
		m[KEY_DOMAIN_PATTERN] = domainPattern;
	m[KEY_RECORD_COUNT_LIMIT] = recordCountLimit;
	m[KEY_WITH_SNAPSHOT] = withSnapshot;
	m[KEY_WITH_PATHS_DICT] = withPathsDict;
	m[KEY_WITH_TYPE_INFO] = withTypeInfo;
	if(fill_legacy_fields) {
		//for compatibility with legacy devices (don't remove ! :-))
		unsigned flags = HeaderOptions::BasicInfo | HeaderOptions::FieldInfo
				| (withTypeInfo? (unsigned)HeaderOptions::TypeInfo: 0)
				| (withPathsDict? (unsigned)HeaderOptions::PathsDict: 0) ;
		m[KEY_HEADER_OPTIONS_DEPRECATED] = flags;
		m[KEY_MAX_RECORD_COUNT_DEPRECATED] = recordCountLimit;
	}
	return chainpack::RpcValue{m};
}

ShvGetLogParams ShvGetLogParams::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvGetLogParams ret;
	const cp::RpcValue::Map &m = v.asMap();
	ret.since = m.value(KEY_WITH_SINCE);
	ret.until = m.value(KEY_WITH_UNTIL);
	ret.pathPattern = m.value(KEY_PATH_PATTERN).toString();
	ret.pathPatternType = m.value(KEY_PATH_PATTERN_TYPE).toString() == REG_EX? PatternType::RegEx: PatternType::WildCard;
	ret.domainPattern = m.value(KEY_DOMAIN_PATTERN).toString();

	//for compatibility with legacy devices
	ret.recordCountLimit = m.value(KEY_RECORD_COUNT_LIMIT, m.value(KEY_MAX_RECORD_COUNT_DEPRECATED, DEFAULT_RECORD_COUNT_LIMIT)).toInt();
	if(m.hasKey(KEY_HEADER_OPTIONS_DEPRECATED)) {
		unsigned flags = m.value(KEY_HEADER_OPTIONS_DEPRECATED).toUInt();
		ret.withTypeInfo = flags & HeaderOptions::TypeInfo;
		ret.withPathsDict = flags & HeaderOptions::PathsDict;
	}
	// new settings keys will override the legacy ones, if set
	ret.withTypeInfo = m.value(KEY_WITH_TYPE_INFO,ret.withTypeInfo).toBool();
	ret.withSnapshot = m.value(KEY_WITH_SNAPSHOT,ret.withSnapshot).toBool();
	ret.withPathsDict = m.value(KEY_WITH_PATHS_DICT, ret.withPathsDict).toBool();
	return ret;
}

bool ShvGetLogParams::isSinceLast() const
{
	return since.asString() == SINCE_NOW || since.asString() == SINCE_LAST;
}

} // namespace utils
} // namespace iotqt
} // namespace shv
