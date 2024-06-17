#include "log.h"

#include <shv/visu/svgscene/graphicsview.h>

#include <QMouseEvent>

#ifdef ANDROID
#include <QGestureEvent>
#include <QScroller>
#endif

namespace shv::visu::svgscene {

GraphicsView::GraphicsView(QWidget *parent)
	: Super(parent)
{
#ifdef ANDROID
	QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);
	grabGesture(Qt::PinchGesture);
#endif
}

void GraphicsView::zoomToFit()
{
	if(scene()) {
		QRectF sr = scene()->sceneRect();
		fitInView(sr, Qt::KeepAspectRatio);
	}
}

void GraphicsView::zoom(double delta, const QPoint &mouse_pos)
{
	nLogFuncFrame() << "delta:" << delta << "center_pos:" << mouse_pos.x() << mouse_pos.y();
	if(delta == 0)
		return;
	double factor = delta / 100;
	factor = 1 + factor;
	if(factor < 0)
		factor = 0.1;
	scale(factor, factor);

	QRect view_rect = QRect(viewport()->pos(), viewport()->size());
	QPoint view_center = view_rect.center();
 	QSize view_d(view_center.x() - mouse_pos.x(), view_center.y() - mouse_pos.y());
	view_d /= factor;
	view_center = QPoint(mouse_pos.x() + view_d.width(), mouse_pos.y() + view_d.height());
	QPointF new_scene_center = mapToScene(view_center);
	centerOn(new_scene_center);
}

void GraphicsView::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *ev)
{
	if(ev->modifiers() == Qt::ControlModifier) {
		double delta = ev->angleDelta().y();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	QPoint pos = ev->pos();
#else
	QPoint pos = ev->position().toPoint();
#endif
		zoom(delta / 10, pos);
		ev->accept();
		return;
	}
	Super::wheelEvent(ev);
}

void GraphicsView::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		m_dragMouseStartPos = ev->pos();
		setCursor(QCursor(Qt::ClosedHandCursor));
		ev->accept();
		return;
	}
	Super::mousePressEvent(ev);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		setCursor(QCursor());
	}
	Super::mouseReleaseEvent(ev);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* ev)
{
	if(ev->buttons() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		QPoint pos = ev->pos();
		QRect view_rect = QRect(viewport()->pos(), viewport()->size());
		QPoint view_center = view_rect.center();
		QPoint d(pos.x() - m_dragMouseStartPos.x(), pos.y() - m_dragMouseStartPos.y());
		view_center -= d;
		QPointF new_scene_center = mapToScene(view_center);
		centerOn(new_scene_center);
		m_dragMouseStartPos = pos;
		ev->accept();
		return;
	}
	Super::mouseMoveEvent(ev);
}

#ifdef ANDROID
bool GraphicsView::event(QEvent *event)
{
	if (event->type() == QEvent::Gesture) {
		auto *gesture_event = static_cast<QGestureEvent*>(event);
		if (QGesture *gesture = gesture_event->gesture(Qt::PinchGesture)) {
			auto *pinch_gesture = static_cast<QPinchGesture *>(gesture);

			qreal scale = pinch_gesture->lastScaleFactor();

			if (scale < 1) {
				scale = 1 / scale;
				scale = -scale;
			}
			zoom(scale, pinch_gesture->centerPoint().toPoint());
		}
	}
	return Super::event(event);
}
#endif

}
