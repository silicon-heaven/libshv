#include <shv/coreqt/utils/versioninfo.h>

namespace shv::coreqt::utils {

VersionInfo::VersionInfo(int major, int minor, int patch, const QString &branch)
	: Super(major, minor, patch, branch.toStdString())
{
}

VersionInfo::VersionInfo(const QString &version, const QString &branch)
	: Super(version.toStdString(), branch.toStdString())
{
}

QString VersionInfo::branch() const
{
	return QString::fromStdString(Super::branch());
}

QString VersionInfo::toString() const
{
	return QString::fromStdString(Super::toString());
}

}
