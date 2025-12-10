#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QSortFilterProxyModel>

namespace shv::visu::timeline {

class LIBSHVVISU_EXPORT ChannelFilterSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	using Super = QSortFilterProxyModel;
public:
	explicit ChannelFilterSortFilterProxyModel(QObject *parent = nullptr);

	void setFilterString(const QString &text);

	bool isFilterAcceptedByParent(const QModelIndex &ix) const;
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	QString m_filterString;
};
}
