#pragma once

#include <shv/core/shvcore_export.h>

#include <shv/core/utils/shvtypeinfo.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils.h>

#include <shv/chainpack/rpcvalue.h>

namespace shv::core::utils {

struct ShvGetLogParams;

class LIBSHVCORE_EXPORT ShvLogHeader //: public shv::chainpack::RpcValue::MetaData
{
	using Super = shv::chainpack::RpcValue::MetaData;

	SHV_FIELD_IMPL(std::string, d, D, eviceType)
	SHV_FIELD_IMPL(std::string, d, D, eviceId)
	SHV_FIELD_IMPL2(int, l, L, ogVersion, 2)
	SHV_FIELD_IMPL(ShvGetLogParams, l, L, ogParams)
	SHV_FIELD_IMPL2(int, r, R, ecordCount, 0)
	SHV_FIELD_IMPL2(int, r, R, ecordCountLimit, 0)
	SHV_FIELD_IMPL2(bool, r, R, ecordCountLimitHit, false)
	SHV_FIELD_IMPL2(bool, w, W, ithSnapShot, false)
	SHV_FIELD_IMPL2(bool, w, W, ithPathsDict, true)
	SHV_FIELD_IMPL(shv::chainpack::RpcList, f, F, ields)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue::IMap, p, P, athDict)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, ateTime)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, s, S, ince)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, u, U, ntil)
public:
	static const std::string EMPTY_PREFIX_KEY;
	struct Column
	{
		enum Enum {
			Timestamp = 0,
			Path,
			Value,
			ShortTime,
			Domain,
			ValueFlags,
			UserId,
		};
		static const char* name(Enum e);
	};
public:
	ShvLogHeader();

	int64_t sinceMsec() const;
	int64_t untilMsec() const;

	static ShvLogHeader fromMetaData(const chainpack::RpcValue::MetaData &md);
	chainpack::RpcValue::MetaData toMetaData() const;

	void copyTypeInfo(const ShvLogHeader &source);

	const ShvTypeInfo& typeInfo(const std::string &path_prefix = EMPTY_PREFIX_KEY) const;

	void setTypeInfo(ShvTypeInfo &&ti, const std::string &path_prefix = EMPTY_PREFIX_KEY);
	void setTypeInfo(const ShvTypeInfo &ti, const std::string &path_prefix = EMPTY_PREFIX_KEY);
private:
	std::map<std::string, ShvTypeInfo> m_typeInfos;
};
} // namespace shv::core::utils
