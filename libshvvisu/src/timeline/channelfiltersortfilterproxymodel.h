#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QSortFilterProxyModel>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT ChannelFilterSortFilterProxyModel : public QSortFilterProxyModel
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

}}}
