#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QScrollArea>

namespace shv::visu::timeline {

class SHVVISU_DECL_EXPORT GraphView : public QScrollArea
{
	Q_OBJECT

	using Super = QScrollArea;
public:
	GraphView(QWidget *parent = nullptr);

	void makeLayout();
	QRect widgetViewRect() const; // rect on widget() visible through this view
protected:
	void resizeEvent(QResizeEvent *event) override;
	void scrollContentsBy(int dx, int dy) override;
};

}
