#include <shv/core/utils/versioninfo.h>

#include <shv/core/utils.h>

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
	std::vector<std::string> parts = shv::core::utils::split(version, '.');
	while (parts.size() < 3) {
		parts.emplace_back("0");
	}
	m_majorNumber = std::stoi(parts[0]);
	m_minorNumber = std::stoi(parts[1]);
	m_patchNumber = std::stoi(parts[2]);
}

std::string VersionInfo::toString() const
{
	return std::to_string(m_majorNumber) + '.' + std::to_string(m_minorNumber) + '.' + std::to_string(m_patchNumber);
}

bool VersionInfo::operator==(const VersionInfo &v) const
{
	return std::tie(this->m_majorNumber, this->m_minorNumber, this->m_patchNumber) == std::tie(v.m_majorNumber, v.m_minorNumber, v.m_patchNumber);
}

std::strong_ordering VersionInfo::operator<=>(const VersionInfo &v) const
{
#if __ANDROID__
	if (operator==(v)) {
		return std::strong_ordering::equal;
	}

	if (std::tie(this->m_majorNumber, this->m_minorNumber, this->m_patchNumber) < std::tie(v.m_majorNumber, v.m_minorNumber, v.m_patchNumber)) {
		return std::strong_ordering::less;
	}

	return std::strong_ordering::greater;
#else
	return std::tie(this->m_majorNumber, this->m_minorNumber, this->m_patchNumber) <=> std::tie(v.m_majorNumber, v.m_minorNumber, v.m_patchNumber);
#endif
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
