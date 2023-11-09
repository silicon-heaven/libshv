#pragma once

#include "../shvvisuglobal.h"
#include <shv/visu/timeline/graph.h>

#include <QDialog>
#include <optional>

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
	explicit ChannelFilterDialog(QWidget *parent, const QString &site_path, Graph *graph);
	~ChannelFilterDialog();

	std::optional<ChannelFilter> channelFilter();

private:
	void applyTextFilter();
	void reloadDataViewsCombobox();

	void saveDataView();
	void saveDataViewAs();
	void discardUserChanges();
	void deleteDataView();
	void exportDataView();
	void importDataView();

	void refreshActions();

	void setVisibleItemsCheckState(Qt::CheckState state);
	void setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state);
	void loadChannelFilterFomGraph();

	void onDataViewComboboxChanged(int index);
	void onCustomContextMenuRequested(QPoint pos);

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
	QString m_recentSettingsDir;

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
