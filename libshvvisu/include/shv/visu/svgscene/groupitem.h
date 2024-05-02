#pragma once

#include "../shvvisuglobal.h"

#include <QGraphicsItem>

namespace shv::visu::svgscene {

class SHVVISU_DECL_EXPORT GroupItem : public QGraphicsRectItem
{
	using Super = QGraphicsRectItem;
public:
	GroupItem(QGraphicsItem *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

} // namespace shv::visu::svgscene
