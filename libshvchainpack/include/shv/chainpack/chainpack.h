#pragma once

#include <shv/chainpack/cchainpack.h>
#include <shv/chainpack/shvchainpackglobal.h>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPack
{
public:
	// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
	// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
	static constexpr int64_t SHV_EPOCH_MSEC = 1517529600000;
public:
	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
	static constexpr uint8_t STRING_META_KEY_PREFIX = 0xFE;
	struct SHVCHAINPACK_DECL_EXPORT PackingSchema {
		enum Enum {
			INVALID = -1,

			Null = CP_Null,
			UInt = CP_UInt,
			Int = CP_Int,
			Double = CP_Double,
			Bool = CP_Bool,
			Blob = CP_Blob,
			String = CP_String,
			DateTimeEpoch_depr = CP_DateTimeEpoch_depr,
			List = CP_List,
			Map = CP_Map,
			IMap = CP_IMap,
			MetaMap = CP_MetaMap,
			Decimal = CP_Decimal,
			DateTime = CP_DateTime,
			CString = CP_CString,

			FALSE_SCHEMA = CP_FALSE,
			TRUE_SCHEMA = CP_TRUE,
			TERM = CP_TERM,
		};
		static const char* name(Enum e);
	};
};

}}

