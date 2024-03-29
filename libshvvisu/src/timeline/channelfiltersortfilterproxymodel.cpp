#include "channelfiltersortfilterproxymodel.h"

namespace shv::visu::timeline {

ChannelFilterSortFilterProxyModel::ChannelFilterSortFilterProxyModel(QObject *parent) :
	Super(parent)
{

}

void ChannelFilterSortFilterProxyModel::setFilterString(const QString &text)
{
	m_filterString = text;
	invalidateFilter();
}

bool ChannelFilterSortFilterProxyModel::isFilterAcceptedByParent(const QModelIndex &ix) const
{
	if (!ix.isValid())
		return false;

	if (sourceModel()->data(ix).toString().contains(m_filterString, filterCaseSensitivity())) {
		return true;
	}

	return isFilterAcceptedByParent(ix.parent());
}

bool ChannelFilterSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex ix = sourceModel()->index(source_row, 0, source_parent);
	if (sourceModel()->data(ix).toString().contains(m_filterString, filterCaseSensitivity())) {
		return true;
	}

	return isFilterAcceptedByParent(ix.parent());
}

}
