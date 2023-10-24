#pragma once

#include <QWidget>
#include <shv/visu/timeline/graph.h>

namespace shv {
namespace visu {
namespace logview {

namespace Ui {
class DataViewWidget;
}

class DataViewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DataViewWidget(QWidget *parent = nullptr);
	~DataViewWidget();

	void init(const QString &site_path, timeline::Graph *graph);

private:
	void enableOnDataViewChangedAction() {m_onDataViewChangedActionEnabled = true;}
	void disableOnDataViewChangedAction() {m_onDataViewChangedActionEnabled = false;}
	void reloadDataViewsCombobox();

	void onDataViewChanged(int index);
	void onShowChannelFilterClicked();

	timeline::Graph *m_graph = nullptr;
	QString m_sitePath;
	bool m_onDataViewChangedActionEnabled = true;

	Ui::DataViewWidget *ui;
};

}}}
