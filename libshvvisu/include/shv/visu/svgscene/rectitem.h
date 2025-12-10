#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QGraphicsRectItem>

namespace shv::visu::svgscene {

class LIBSHVVISU_EXPORT RectItem : public QGraphicsRectItem
{
	using Super = QGraphicsRectItem;
public:
	RectItem(QGraphicsItem *parent = nullptr);
	RectItem(qreal x_radius, qreal y_radius, QGraphicsItem *parent = nullptr);

	void setXRadius(qreal r)  { m_xRadius = r; }
	void setYRadius(qreal r)  { m_yRadius = r; }

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
	qreal m_xRadius = 0;
	qreal m_yRadius = 0;
};

}
