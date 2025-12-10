#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QRegularExpression>
#include <QString>

namespace shv::visu::timeline {

class LIBSHVVISU_EXPORT FullTextFilter
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
