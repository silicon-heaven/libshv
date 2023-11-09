#ifndef SHV_VISU_SVGSCENE_GROUPITEM_H
#define SHV_VISU_SVGSCENE_GROUPITEM_H

#include "../shvvisuglobal.h"

#include <QGraphicsItem>

namespace shv {
namespace visu {
namespace svgscene {

class SHVVISU_DECL_EXPORT GroupItem : public QGraphicsRectItem
{
	using Super = QGraphicsRectItem;
public:
	GroupItem(QGraphicsItem *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

} // namespace svgscene
} // namespace visu
} // namespace shv

#endif // SHV_VISU_SVGSCENE_GROUPITEM_H
