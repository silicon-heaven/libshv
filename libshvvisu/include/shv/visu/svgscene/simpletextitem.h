#pragma once

#include "saxhandler.h"

#include <QGraphicsItem>

namespace shv {
namespace visu {
namespace svgscene {

class SHVVISU_DECL_EXPORT SimpleTextItem : public QGraphicsSimpleTextItem
{
	using Super = QGraphicsSimpleTextItem;
public:
	SimpleTextItem(const CssAttributes &css, QGraphicsItem *parent = nullptr);

	void setText(const QString text);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
	int m_alignment = Qt::AlignLeft;
	QTransform m_origTransform;
	bool m_origTransformLoaded = false;
};

}}} // namespace svgscene

