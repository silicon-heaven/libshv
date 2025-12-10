#pragma once

#include <shv/coreqt/shvcoreqt_export.h>

#include <shv/core/utils/versioninfo.h>

#include <QString>

namespace shv::coreqt::utils {

class LIBSHVCOREQT_EXPORT VersionInfo : public shv::core::utils::VersionInfo
{
	using Super = shv::core::utils::VersionInfo;

public:
	VersionInfo(int major = 0, int minor = 0, int patch = 0, const QString &branch = QString());
	VersionInfo(const QString &version, const QString &branch = QString());

	QString branch() const;

	QString toString() const;
};
}
