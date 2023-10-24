#include <shv/visu/svgscene/visucontroller.h>
#include <shv/visu/svgscene/types.h>

#include <shv/coreqt/log.h>

#include <QPainter>

namespace shv::visu::svgscene {
//===========================================================================
// VisuController
//===========================================================================
VisuController::VisuController(QGraphicsItem *graphics_item, QObject *parent)
	: Super(parent)
	, m_graphicsItem(graphics_item)
{
	setId(graphics_item->data(Types::DataKey::Id).toString());
	setObjectName(id());
	setShvType(graphics_item->data(Types::DataKey::ShvType).toString());
	setShvPath(graphics_item->data(Types::DataKey::ShvPath).toString());
}

QString VisuController::graphicsItemAttributeValue(const QGraphicsItem *it, const QString &attr_name, const QString &default_value)
{
	auto attrs = qvariant_cast<svgscene::XmlAttributes>(it->data(Types::DataKey::XmlAttributes));
	return attrs.value(attr_name, default_value);
}

QString VisuController::graphicsItemCssAttributeValue(const QGraphicsItem *it, const QString &attr_name, const QString &default_value)
{
	auto attrs = qvariant_cast<svgscene::CssAttributes>(it->data(Types::DataKey::CssAttributes));
	return attrs.value(attr_name, default_value);
}


}

