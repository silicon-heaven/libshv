#include <shv/chainpack/chainpack.h>

namespace shv::chainpack {

const char *ChainPack::PackingSchema::name(ChainPack::PackingSchema::Enum e)
{
	switch (e) {
	case INVALID: return "INVALID";
	case FALSE_SCHEMA: return "FALSE";
	case TRUE_SCHEMA: return "TRUE";

	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case CString: return "CString";
	case List: return "List";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTimeEpoch_depr: return "DateTimeEpoch_depr";
	case DateTime: return "DateTime";
	case MetaMap: return "MetaMap";
	case Decimal: return "Decimal";

	case TERM: return "TERM";
	}
	return "UNKNOWN";
}

}
