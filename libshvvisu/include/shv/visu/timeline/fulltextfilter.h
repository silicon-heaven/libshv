#pragma once

#include "../shvvisuglobal.h"

#include <QRegularExpression>
#include <QString>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT FullTextFilter
{
public:
	FullTextFilter();

	const QString &pattern() const;
	void setPattern(const QString &pattern);

	bool isCaseSensitive() const;
	void setCaseSensitive(bool case_sensitive);

	bool isRegularExpression() const;
	void setRegularExpression(bool regular_expression);

	bool matches(const QString &value) const;
private:
	void initRegexp();
private:
	QString m_pattern;
	QRegularExpression m_regexp;
	bool m_isCaseSensitive = false;
	bool m_isRegularExpression = false;
};

}
}
}
