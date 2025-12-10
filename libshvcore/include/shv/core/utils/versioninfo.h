#pragma once

#include <shv/core/shvcore_export.h>

#include <compare>
#include <string>

namespace shv::core::utils {

class LIBSHVCORE_EXPORT VersionInfo
{
public:
	VersionInfo(int major = 0, int minor = 0, int patch = 0, const std::string &branch = std::string());
	VersionInfo(const std::string &version, const std::string &branch = std::string());

	int majorNumber() const;
	int minorNumber() const;
	int patchNumber() const;
	const std::string &branch() const;

	std::string toString() const;

	bool operator==(const VersionInfo &v) const;
	std::strong_ordering operator<=>(const VersionInfo &v) const;

private:
	int m_majorNumber;
	int m_minorNumber;
	int m_patchNumber;

	std::string m_branch;
};
}
