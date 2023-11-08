#pragma once

#include "../shvvisuglobal.h"

#include <QSet>
#include <QRegularExpression>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT ChannelFilter
{
public:
	ChannelFilter();
	ChannelFilter(const QSet<QString> &permitted_paths, const QString &name = {});

	void addPermittedPath(const QString &path);
	void removePermittedPath(const QString &path);

	QSet<QString> permittedPaths() const;
	void setPermittedPaths(const QSet<QString> &paths);

	bool isPathPermitted(const QString &path) const;
	QString name();

private:
	QString m_name;
	QSet<QString> m_permittedPaths;
};

}
}
}
