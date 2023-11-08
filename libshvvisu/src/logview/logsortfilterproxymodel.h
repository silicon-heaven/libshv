#pragma once

#include "../shvvisuglobal.h"
#include "../timeline/channelfilter.h"
#include "../timeline/fulltextfilter.h"

#include <QSortFilterProxyModel>

namespace shv {
namespace visu {
namespace logview {

class SHVVISU_DECL_EXPORT LogSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	using Super = QSortFilterProxyModel;
public:
	explicit LogSortFilterProxyModel(QObject *parent = nullptr);

	void setChannelFilter(const std::optional<timeline::ChannelFilter> &filter);
	void setShvPathColumn(int column);
	void setValueColumn(int column);
	void setFulltextFilter(const shv::visu::timeline::FullTextFilter &filter);

	bool filterAcceptsRow(int source_rrow, const QModelIndex &source_parent) const override;

private:
	std::optional<shv::visu::timeline::ChannelFilter> m_channelFilter;
	shv::visu::timeline::FullTextFilter m_fulltextFilter;
	int m_shvPathColumn = -1;
	int m_valueColumn = -1;
};

}}}
