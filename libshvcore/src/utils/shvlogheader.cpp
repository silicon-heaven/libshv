#include <shv/core/utils/shvlogheader.h>

#include <shv/core/exception.h>

#include <algorithm>

namespace shv::core::utils {

const std::string ShvLogHeader::EMPTY_PREFIX_KEY;

const char *ShvLogHeader::Column::name(ShvLogHeader::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	case Column::Enum::Domain: return "domain";
	case Column::Enum::ValueFlags: return "valueFlags";
	case Column::Enum::UserId: return "userId";
	}
	return "invalid";
}

ShvLogHeader::ShvLogHeader() = default;

int64_t ShvLogHeader::sinceMsec() const
{
	return since().toDateTime().msecsSinceEpoch();
}

int64_t ShvLogHeader::untilMsec() const
{
	return until().toDateTime().msecsSinceEpoch();
}

ShvLogHeader ShvLogHeader::fromMetaData(const chainpack::RpcValue::MetaData &md)
{
	ShvLogHeader ret;
	const chainpack::RpcValue::Map &device = md.valref("device").asMap();
	ret.setDeviceId(device.value("id").asString());
	ret.setDeviceType(device.value("type").asString());
	ret.setLogVersion(md.value("logVersion").toInt());
	ShvGetLogParams params = ShvGetLogParams::fromRpcValue(md.value("logParams"));
	ret.setLogParams(std::move(params));
	ret.setRecordCount(md.value("recordCount").toInt());
	ret.setRecordCountLimit(md.value("recordCountLimit").toInt());
	ret.setRecordCountLimitHit(md.value("recordCountLimitHit").toBool());
	ret.setWithSnapShot(md.value(ShvGetLogParams::KEY_WITH_SNAPSHOT).toBool());
	ret.setWithPathsDict(md.value(ShvGetLogParams::KEY_WITH_PATHS_DICT, true).toBool());
	ret.setFields(md.value("fields").asList());
	ret.setPathDict(md.value("pathsDict").asIMap());
	{
		const chainpack::RpcValue::Map &m = md.valref("typeInfos").asMap();
		for(const auto &kv : m) {
			ret.m_typeInfos[kv.first] = ShvTypeInfo::fromRpcValue(kv.second);
		}
		chainpack::RpcValue ti = md.value("typeInfo");
		if(ti.isMap())
			ret.m_typeInfos[EMPTY_PREFIX_KEY] = ShvTypeInfo::fromRpcValue(ti);
	}
	ret.setDateTime(md.value("dateTime"));
	ret.setSince(md.value("since"));
	ret.setUntil(md.value("until"));
	return ret;
}

chainpack::RpcValue::MetaData ShvLogHeader::toMetaData() const
{
	chainpack::RpcValue::MetaData md;
	chainpack::RpcValue::Map device;
	if(!deviceId().empty())
		device["id"] = deviceId();
	if(!deviceType().empty())
		device["type"] = deviceType();
	if(!device.empty())
		md.setValue("device", device);
	md.setValue("logVersion", logVersion());
	md.setValue("logParams", logParams().toRpcValue(false));
	md.setValue("recordCount", recordCount());
	md.setValue("recordCountLimit", recordCountLimit());
	md.setValue("recordCountLimitHit", recordCountLimitHit());
	md.setValue(ShvGetLogParams::KEY_WITH_SNAPSHOT, withSnapShot());
	md.setValue(ShvGetLogParams::KEY_WITH_PATHS_DICT, withPathsDict());
	if(!fields().empty())
		md.setValue("fields", fields());
	if(!pathDict().empty())
		md.setValue("pathsDict", pathDict());
	if(m_typeInfos.size() == 1 && m_typeInfos.find(EMPTY_PREFIX_KEY) != m_typeInfos.end()) {
		md.setValue("typeInfo", m_typeInfos.at(EMPTY_PREFIX_KEY).toRpcValue());
	}
	else {
		if(!m_typeInfos.empty()) {
			chainpack::RpcValue::Map m;
			for(const auto &kv : m_typeInfos)
				m[kv.first] = kv.second.toRpcValue();
			md.setValue("typeInfos", m);
		}
	}
	md.setValue("dateTime", dateTime());
	md.setValue("since", since());
	md.setValue("until", until());
	return md;
}

void ShvLogHeader::copyTypeInfo(const ShvLogHeader &source)
{
	m_typeInfos = source.m_typeInfos;
}

const ShvTypeInfo &ShvLogHeader::typeInfo(const std::string &path_prefix) const
{
	auto it = m_typeInfos.find(path_prefix);
	if(it == m_typeInfos.end()) {
		static const ShvTypeInfo ti;
		return ti;
	}
	return m_typeInfos.at(path_prefix);
}

void ShvLogHeader::setTypeInfo(ShvTypeInfo &&ti, const std::string &path_prefix)
{
	m_typeInfos[path_prefix] = std::move(ti);
}

void ShvLogHeader::setTypeInfo(const ShvTypeInfo &ti, const std::string &path_prefix)
{
	m_typeInfos[path_prefix] = ti;
}
} // namespace shv
