#pragma once

#include "../shvvisuglobal.h"

#include <QGraphicsView>

namespace shv::visu::svgscene {

class SHVVISU_DECL_EXPORT GraphicsView : public QGraphicsView
{
	Q_OBJECT

	using Super = QGraphicsView;
public:
	GraphicsView(QWidget *parent = nullptr);

	void zoomToFit();
protected:
	void zoom(double delta, const QPoint &mouse_pos);

	void paintEvent(QPaintEvent *event) override;

	void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

	bool event(QEvent *event) override;

private:
	QPoint m_dragMouseStartPos;
};
}
