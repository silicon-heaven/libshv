#pragma once

#include "../shvvisuglobal.h"
#include <shv/visu/timeline/graph.h>


#include <QDialog>

namespace shv {
namespace visu {
namespace timeline {

namespace Ui {
class ChannelFilterDialog;
}

class ChannelFilterModel;
class ChannelFilterSortFilterProxyModel;

class SHVVISU_DECL_EXPORT ChannelFilterDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChannelFilterDialog(QWidget *parent = nullptr);
	~ChannelFilterDialog();

	void init(const QString &site_path, const QString &visual_settings_id, Graph *graph);
	std::optional<ChannelFilter> filter();
	QString currentVisualSettingsId();

private:
	void applyTextFilter();
	void reloadDataViewsCombobox();

	void saveView();
	void saveViewAs();
	void discardUserChanges();
	void deleteView();
	void exportView();
	void importView();

	void updateContextMenuActionsAvailability();

	void setVisibleItemsCheckState(Qt::CheckState state);
	void setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state);
	void setPermittedChannelsFromGraph();

	void onDataViewChanged(int index);
	void onCustomContextMenuRequested(QPoint pos);

	void onChbFilterEnabledClicked(int state);
	void onPbCheckItemsClicked();
	void onPbUncheckItemsClicked();
	void onPbClearMatchingTextClicked();
	void onLeMatchingFilterTextChanged(const QString &text);
	void onChbFindRegexChanged(int state);

	Ui::ChannelFilterDialog *ui;
	shv::visu::timeline::Graph *m_graph = nullptr;
	ChannelFilterModel *m_channelsFilterModel = nullptr;
	ChannelFilterSortFilterProxyModel *m_channelsFilterProxyModel = nullptr;
	QString m_sitePath;

	QAction *m_saveViewAction = nullptr;
	QAction *m_saveViewAsAction = nullptr;
	QAction *m_revertViewAction = nullptr;
	QAction *m_resetViewAction = nullptr;
	QAction *m_deleteViewAction = nullptr;
	QAction *m_exportViewAction = nullptr;
	QAction *m_importViewAction = nullptr;
};

}
}
}
