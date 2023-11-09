#pragma once

#include "../shvvisuglobal.h"

#include <shv/visu/timeline/graph.h>

#include <QWidget>

namespace shv {
namespace visu {
namespace logview {

namespace Ui {
class DataViewWidget;
}

class SHVVISU_DECL_EXPORT DataViewWidget : public QWidget
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

	Ui::DataViewWidget *ui;
};

}}}
