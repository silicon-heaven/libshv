#pragma once

#include <shv/coreqt/shvcoreqtglobal.h>

#include <shv/core/utils/versioninfo.h>

#include <QString>

namespace shv {
namespace coreqt {
namespace utils {

class SHVCOREQT_DECL_EXPORT VersionInfo : public shv::core::utils::VersionInfo
{
	using Super = shv::core::utils::VersionInfo;

public:
	VersionInfo(int major = 0, int minor = 0, int patch = 0, const QString &branch = QString());
	VersionInfo(const QString &version, const QString &branch = QString());

	QString branch() const;

	QString toString() const;
};

}
}
}
