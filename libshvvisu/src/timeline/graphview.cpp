#include <shv/visu/timeline/graphview.h>
#include <shv/visu/timeline/graphwidget.h>

#include <shv/coreqt/log.h>

#include <QTimer>
#include <QScrollBar>

namespace shv::visu::timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{
}

void GraphView::makeLayout()
{
	if(auto *w = qobject_cast<GraphWidget*>(widget())) {
		auto size = geometry().size();
		//size -= QSize(15, 15);
		w->makeLayout(size); // space for scroll bar

		auto scrollbar_visible = (w->geometry().size() != size);

		//Set horizontal scroll bar allways off, use minimap instead
		setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy((scrollbar_visible)? Qt::ScrollBarPolicy::ScrollBarAlwaysOn: Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

		if (auto *sb = verticalScrollBar(); sb) {
			sb->setVisible(scrollbar_visible);
		}

		shvDebug() << "w geometry" << w->geometry().width() << "h" << w->geometry().height();
	}
}

QRect GraphView::widgetViewRect() const
{
	QRect view_rect = viewport()->geometry();
	view_rect.moveTop(-widget()->geometry().y());
	return view_rect;
}

void GraphView::resizeEvent(QResizeEvent *event)
{
	Super::resizeEvent(event);
	makeLayout();
}

void GraphView::scrollContentsBy(int dx, int dy)
{
	if(auto *gw = qobject_cast<GraphWidget*>(widget())) {
		const Graph *g = gw->graph();
		QRect r1 = g->southFloatingBarRect();
		Super::scrollContentsBy(dx, dy);
		if(dy < 0) {
			// scroll up, invalidate current floating bar pos
			// make it slightly higher to avoid artifacts
			widget()->update(r1.adjusted(0, -3, 0, 0));
		}
		else if(dy > 0) {
			// scroll down, invalidate new floating bar pos
			r1.setTop(r1.top() - dy);
			widget()->update(r1.adjusted(0, -3, 0, 0));
		}
	}
}

}
