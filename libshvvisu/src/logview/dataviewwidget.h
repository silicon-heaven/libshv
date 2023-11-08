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
	void onShowChannelFilterClicked();

	timeline::Graph *m_graph = nullptr;
	QString m_sitePath;
	QString m_currentVisualSettingsId;

	Ui::DataViewWidget *ui;
};

}}}
