#include <shv/visu/logview/logsortfilterproxymodel.h>

#include <shv/core/utils/shvpath.h>
#include <shv/core/log.h>

namespace shv::visu::logview {

LogSortFilterProxyModel::LogSortFilterProxyModel(QObject *parent) :
	Super(parent)
{

}

void LogSortFilterProxyModel::setChannelFilter(const std::optional<shv::visu::timeline::ChannelFilter> &filter)
{
	m_channelFilter = filter;
	invalidateFilter();
}

void LogSortFilterProxyModel::setChannelFilterPathColumn(int column)
{
	m_channelFilterPathColumn = column;
}

void LogSortFilterProxyModel::setValueColumn(int column)
{
	m_valueColumn = column;
}

void LogSortFilterProxyModel::setFulltextFilter(const timeline::FullTextFilter &filter)
{
	m_fulltextFilter = filter;
	invalidateFilter();
}

void LogSortFilterProxyModel::setFulltextFilterPathColumn(int column)
{
	m_fulltextFilterPathColumn = column;
}

bool LogSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	bool is_row_accepted = true;

	if (m_channelFilterPathColumn >= 0) {
		QModelIndex ix = sourceModel()->index(source_row, m_channelFilterPathColumn, source_parent);
		is_row_accepted = (m_channelFilter) ? m_channelFilter.value().isPathPermitted(sourceModel()->data(ix).toString()) : true;
	}
	if (is_row_accepted && !m_fulltextFilter.pattern().isEmpty()) {
		bool path_matches = true;
		bool value_matches = true;
		if (m_fulltextFilterPathColumn >= 0) {
			QModelIndex ix = sourceModel()->index(source_row, m_fulltextFilterPathColumn, source_parent);
			path_matches = m_fulltextFilter.matches(sourceModel()->data(ix).toString());
		}
		if (m_valueColumn >= 0) {
			QModelIndex ix = sourceModel()->index(source_row, m_valueColumn, source_parent);
			value_matches = m_fulltextFilter.matches(sourceModel()->data(ix).toString());
		}
		is_row_accepted = path_matches || value_matches;
	}
	return is_row_accepted;
}

}
