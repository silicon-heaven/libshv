#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QString>
#include <QMap>

namespace shv::visu::svgscene {

struct LIBSHVVISU_EXPORT Types
{
	struct LIBSHVVISU_EXPORT DataKey
	{
		enum {
			XmlAttributes = 1,
			CssAttributes,
			Id,
			ChildId,
			ShvPath,
			ShvType,
			ShvVisuType,
		};
	};

	using XmlAttributes = QMap<QString, QString>;
	using CssAttributes = QMap<QString, QString>;

	static const QString ATTR_ID;
	static const QString ATTR_CHILD_ID;
	static const QString ATTR_SHV_PATH;
	static const QString ATTR_SHV_PROPERTY_PATH;
	static const QString ATTR_SHV_TYPE;
	static const QString ATTR_SHV_VISU_TYPE;
	static const QString ATTR_SHV_GRAPHICS_TYPE;
};
}
