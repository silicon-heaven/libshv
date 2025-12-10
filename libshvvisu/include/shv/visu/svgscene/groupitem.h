#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QGraphicsItem>

namespace shv::visu::svgscene {

class LIBSHVVISU_EXPORT GroupItem : public QGraphicsRectItem
{
	using Super = QGraphicsRectItem;
public:
	GroupItem(QGraphicsItem *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

} // namespace shv::visu::svgscene
