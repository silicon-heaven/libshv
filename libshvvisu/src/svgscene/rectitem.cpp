#include <shv/visu/svgscene/rectitem.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>


namespace shv::visu::svgscene {

RectItem::RectItem(QGraphicsItem *parent)
	: RectItem(0, 0, parent)
{
}

RectItem::RectItem(qreal x_radius, qreal y_radius, QGraphicsItem *parent)
	: Super(parent)
	, m_xRadius(x_radius)
	, m_yRadius(y_radius)
{
}

void RectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->setPen(pen());
	painter->setBrush(brush());
	painter->drawRoundedRect(rect(), m_xRadius, m_yRadius);
	//if (option->state & QStyle::State_Selected) {
	//	qt_graphicsItem_highlightSelected(this, painter, option);
	//}
}

}
