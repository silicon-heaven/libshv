#include "versioninfo.h"

#include "../string.h"
#include "../utils.h"

#if defined LIBC_NEWLIB || defined SHV_ANDROID_BUILD
#include <cstdlib>
#endif

namespace shv::core::utils {

VersionInfo::VersionInfo(int major, int minor, int patch, const std::string &branch)
	: m_majorNumber(major)
	, m_minorNumber(minor)
	, m_patchNumber(patch)
	, m_branch(branch)
{
}

VersionInfo::VersionInfo(const std::string &version, const std::string &branch)
	: m_branch(branch)
{
	std::vector<std::string> parts = String::split(version, '.');
	while (parts.size() < 3) {
		parts.emplace_back("0");
	}
#if defined LIBC_NEWLIB || defined SHV_ANDROID_BUILD
	m_majorNumber = std::atoi(parts[0].c_str());
	m_minorNumber = std::atoi(parts[1].c_str());
	m_patchNumber = std::atoi(parts[2].c_str());
#else
	m_majorNumber = std::stoi(parts[0]);
	m_minorNumber = std::stoi(parts[1]);
	m_patchNumber = std::stoi(parts[2]);
#endif
}

std::string VersionInfo::toString() const
{
	return shv::core::Utils::toString(m_majorNumber) + '.' + shv::core::Utils::toString(m_minorNumber) + '.' + shv::core::Utils::toString(m_patchNumber);
}

int VersionInfo::toInt() const
{
	return m_majorNumber * 1000000 + m_minorNumber * 1000 + m_patchNumber;
}

bool VersionInfo::operator==(const VersionInfo &v) const
{
	return m_majorNumber == v.m_majorNumber && m_minorNumber == v.m_minorNumber && m_patchNumber == v.m_patchNumber && m_branch == v.m_branch;
}

bool VersionInfo::operator!=(const VersionInfo &v) const
{
	return !operator==(v);
}

const std::string &VersionInfo::branch() const
{
	return m_branch;
}

int VersionInfo::majorNumber() const
{
	return m_majorNumber;
}

int VersionInfo::minorNumber() const
{
	return m_minorNumber;
}

int VersionInfo::patchNumber() const
{
	return m_patchNumber;
}

}
