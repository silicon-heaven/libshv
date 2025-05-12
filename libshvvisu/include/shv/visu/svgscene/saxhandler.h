#pragma once

#include <shv/visu/svgscene/types.h>

#include <QPen>
#include <QMap>
#include <QMetaType>
#include <QStack>

class QXmlStreamReader;
class QXmlStreamAttributes;
class QGraphicsScene;
class QGraphicsItem;
class QGraphicsSimpleTextItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QAbstractGraphicsShapeItem;

namespace shv::visu::svgscene {

using XmlAttributes = Types::XmlAttributes;
using CssAttributes = Types::CssAttributes;

class RectItem;

class SHVVISU_DECL_EXPORT SaxHandler
{
public:
	struct SvgElement
	{
		QString name;
		XmlAttributes xmlAttributes;
		CssAttributes styleAttributes;
		bool itemCreated = false;

		SvgElement() = default;
		SvgElement(const QString &n, bool created = false) : name(n), itemCreated(created) {}
	};
public:
	SaxHandler(QGraphicsScene *scene);
	virtual ~SaxHandler();

	void load(QXmlStreamReader *data, bool is_skip_definitions = false);

	static QString point2str(QPointF r);
	static QString rect2str(QRectF r);
protected:
	virtual RectItem *createRectItem(const SvgElement &el);
	virtual QGraphicsItem *createGroupItem(const SvgElement &el);
	virtual void installVisuController(QGraphicsItem *it, const SvgElement &el);
	virtual void setXmlAttributes(QGraphicsItem *git, const SvgElement &el);

	QGraphicsScene *m_scene;
private:
	void parse();
	XmlAttributes parseXmlAttributes(const QXmlStreamAttributes &attributes);
	void mergeCSSAttributes(CssAttributes &css_attributes, const QString &attr_name, const XmlAttributes &xml_attributes);

	void setTransform(QGraphicsItem *it, const QString &str_val);
	void setStyle(QAbstractGraphicsShapeItem *it, const CssAttributes &attributes);
	void setTextStyle(QFont &font, const CssAttributes &attributes);
	void setTextStyle(QGraphicsSimpleTextItem *text, const CssAttributes &attributes);
	void setTextStyle(QGraphicsTextItem *text, const CssAttributes &attributes);

	bool startElement();
	void addItem(QGraphicsItem *it);
private:
	QStack<SvgElement> m_elementStack;

	QGraphicsItem *m_topLevelItem = nullptr;
	QXmlStreamReader *m_xml = nullptr;
	QPen m_defaultPen;
	bool m_skipDefinitions = false;
};

}

Q_DECLARE_METATYPE(shv::visu::svgscene::XmlAttributes)
