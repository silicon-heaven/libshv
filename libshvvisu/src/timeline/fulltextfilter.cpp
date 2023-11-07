#include <shv/visu/timeline/fulltextfilter.h>

#include <QRegularExpression>

namespace shv::visu::timeline {

FullTextFilter::FullTextFilter() = default;

const QString &FullTextFilter::pattern() const
{
	return m_pattern;
}

void FullTextFilter::setPattern(const QString &pattern)
{
	m_pattern = pattern;
	initRegexp();
}

bool FullTextFilter::isCaseSensitive() const
{
	return m_isCaseSensitive;
}

void FullTextFilter::setCaseSensitive(bool case_sensitive)
{
	m_isCaseSensitive = case_sensitive;
	initRegexp();
}

bool FullTextFilter::isRegularExpression() const
{
	return m_isRegularExpression;
}

void FullTextFilter::setRegularExpression(bool regular_expression)
{
	m_isRegularExpression = regular_expression;
	initRegexp();
}

bool FullTextFilter::matches(const QString &value) const
{
	if (m_pattern.isEmpty()) {
		return true;
	}
	if (m_isRegularExpression) {
		QRegularExpressionMatch match = m_regexp.match(value, 0, QRegularExpression::PartialPreferFirstMatch);
		return match.hasMatch();
	}

	return value.contains(m_pattern, m_isCaseSensitive? Qt::CaseSensitive: Qt::CaseInsensitive);
}

void FullTextFilter::initRegexp()
{
	if (m_isRegularExpression) {
		QRegularExpression::PatternOptions options = QRegularExpression::DontCaptureOption;
		if (!m_isCaseSensitive) {
			options |= QRegularExpression::CaseInsensitiveOption;
		}
		m_regexp = QRegularExpression(m_pattern, options);
	}
}

}
