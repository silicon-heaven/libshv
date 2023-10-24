#pragma once

#include "../shvvisuglobal.h"

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

	void init(const QString &site_path, const QStringList &logged_paths);

	QStringList selectedChannels();
	void setSelectedChannels(const QStringList &channels);

private:
	void applyTextFilter();

	void setVisibleItemsCheckState(Qt::CheckState state);
	void setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state);

	void onCustomContextMenuRequested(QPoint pos);

	void onPbCheckItemsClicked();
	void onPbUncheckItemsClicked();
	void onPbClearMatchingTextClicked();
	void onLeMatchingFilterTextChanged(const QString &text);
	void onChbFindRegexChanged(int state);

	Ui::ChannelFilterDialog *ui;
	ChannelFilterModel *m_channelsFilterModel = nullptr;
	ChannelFilterSortFilterProxyModel *m_channelsFilterProxyModel = nullptr;
	QString m_sitePath;
};

}
}
}
