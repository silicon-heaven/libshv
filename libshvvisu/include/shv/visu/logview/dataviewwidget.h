#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <shv/visu/timeline/graph.h>

#include <QWidget>

namespace shv::visu::logview {

namespace Ui {
class DataViewWidget;
}

class SHVVISU_DECL_EXPORT DataViewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DataViewWidget(QWidget *parent = nullptr);
	~DataViewWidget() override;

	void init(const QString &site_path, timeline::Graph *graph);
	void setPredefinedViews(const QVector<timeline::Graph::VisualSettings> &views);
	const QVector<timeline::Graph::VisualSettings> &predefinedViews();
	void applyPredefinedView(const QString &name);

private:
	void onShowChannelFilterClicked();
	void onShowRawDataClicked();

	timeline::Graph *m_graph = nullptr;
	QString m_sitePath;
	QVector<timeline::Graph::VisualSettings> m_predefinedViews;

	Ui::DataViewWidget *ui;
};
}
