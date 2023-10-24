#include <shv/visu/svgscene/groupitem.h>

#include <QPainter>

namespace shv::visu::svgscene {

GroupItem::GroupItem(QGraphicsItem *parent)
	: Super(parent)
{

}

void GroupItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Super::paint(painter, option, widget);
}

} // namespace shv
