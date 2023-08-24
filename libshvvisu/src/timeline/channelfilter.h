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
	ChannelFilter(const QStringList &matching_paths);

	void addMatchingPath(const QString &shv_path);
	void removeMatchingPath(const QString &shv_path);

	QStringList matchingPaths() const;
	void setMatchingPaths(const QStringList &paths);

	bool isPathMatch(const QString &path) const;
	bool isValid() const;
private:
	QStringList m_matchingPaths;
	bool m_isValid;
};

}
}
}
