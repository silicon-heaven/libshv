#pragma once

#include <shv/visu/shvvisu_export.h>

#include <shv/visu/timeline/graph.h>

#include <QWidget>

namespace shv::visu::logview {

namespace Ui {
class DataViewFilterSelector;
}

class LIBSHVVISU_EXPORT DataViewFilterSelector : public QWidget
{
	Q_OBJECT

public:
	explicit DataViewFilterSelector(QWidget *parent = nullptr);
	~DataViewFilterSelector() override;

	void init(const QString &site_path, timeline::Graph *graph);
	void setPredefinedViews(const QVector<timeline::Graph::VisualSettings> &views);
	void applyPredefinedView(const QString &name);

private:
	void onShowChannelFilterClicked();
	void onShowRawDataClicked();

	timeline::Graph *m_graph = nullptr;
	QString m_sitePath;
	QVector<timeline::Graph::VisualSettings> m_predefinedViews;

	Ui::DataViewFilterSelector *ui;
};
}
