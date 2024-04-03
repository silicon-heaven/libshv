#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

#include <optional>
#include <string_view>

namespace shv::chainpack {

enum class AccessLevel {
	None = 0,

	Browse = 1,
	Browse1,
	Browse2,
	Browse3,
	Browse4,
	Browse5,
	Browse6,

	Read = 8,
	Read1,
	Read2,
	Read3,
	Read4,
	Read5,
	Read6,
	Read7,

	Write = 16,
	Write1,
	Write2,
	Write3,
	Write4,
	Write5,
	Write6,
	Write7,

	Command = 24,
	Command1,
	Command2,
	Command3,
	Command4,
	Command5,
	Command6,
	Command7,

	Config = 32,
	Config1,
	Config2,
	Config3,
	Config4,
	Config5,
	Config6,
	Config7,

	Service = 40,
	Service1,
	Service2,
	Service3,
	Service4,
	Service5,
	Service6,
	Service7,

	SuperService = 48,
	SuperService1,
	SuperService2,
	SuperService3,
	SuperService4,
	SuperService5,
	SuperService6,
	SuperService7,

	Devel = 56,
	Devel1,
	Devel2,
	Devel3,
	Devel4,
	Devel5,
	Devel6,
	Devel7,

	Admin = 63,
};

SHVCHAINPACK_DECL_EXPORT const char* accessLevelToAccessString(AccessLevel access_level);
SHVCHAINPACK_DECL_EXPORT std::optional<AccessLevel> accessLevelFromAccessString(std::string_view s);
SHVCHAINPACK_DECL_EXPORT std::optional<AccessLevel> accessLevelFromInt(int i);

} // namespace chainpack
