#pragma once

#include <shv/visu/timeline/channelprobe.h>
#include <shv/visu/timeline/sample.h>

#include <shv/visu/shvvisuglobal.h>

#include <shv/coreqt/utils.h>

#include <QVariantMap>
#include <QColor>
#include <QRect>
#include <QObject>

namespace shv::visu::timeline {

class Graph;
class GraphButtonBox;

class SHVVISU_DECL_EXPORT GraphChannel : public QObject
{
	Q_OBJECT

	friend class Graph;
public:
	class SHVVISU_DECL_EXPORT Style : public QVariantMap
	{
	public:
		struct Interpolation { enum Enum {None = 0, Line, Stepped};};
		struct LineAreaStyle { enum Enum {Blank = 0, Filled};};
		static constexpr double DEFAULT_HEIGHT_MIN = 2.5;

		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, DEFAULT_HEIGHT_MIN) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 1000) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorAxis, QColor(Qt::gray))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(0x23, 0x23, 0x23))

		SHV_VARIANTMAP_FIELD2(bool, is, set, HideDiscreteValuesInfo, false)
		SHV_VARIANTMAP_FIELD2(int, i, setI, nterpolation, Interpolation::Stepped)
		SHV_VARIANTMAP_FIELD2(int, l, setL, ineAreaStyle, LineAreaStyle::Blank)
		SHV_VARIANTMAP_FIELD2(double, l, setL, ineWidth, 0.1)

	public:
		Style();
		Style(const QVariantMap &o);

		static constexpr double HEIGHT_HUGE = 10e3;

		double heightRange() const
		{
			if(!heightMax_isset())
				return HEIGHT_HUGE;
			double ret = heightMax() - heightMin();
			return ret < 0? 0: ret;
		}
	};
public:
	GraphChannel(Graph *graph);

	qsizetype modelIndex() const;
	void setModelIndex(qsizetype ix);

	QString shvPath() const;

	YRange yRange() const;
	YRange yRangeZoom() const;

	const Style& style() const;
	void setStyle(const Style& st);
	Style effectiveStyle() const;

	const QRect& graphAreaRect() const;
	const QRect& graphDataGridRect() const;
	const QRect& verticalHeaderRect() const;
	const QRect& yAxisRect() const;

	int valueToPos(double val) const;
	double posToValue(int y) const;

	struct YAxis {
		double tickInterval = 0;
		int subtickEvery = 1;
	};

	bool isMaximized() const;
	void setMaximized(bool b);

	Graph *graph() const;
protected:
	int graphChannelIndex() const;
protected:
	struct
	{
		YRange yRange;
		YRange yRangeZoom;
		YAxis axis;
		bool isMaximized = false;
	} m_state;

	struct
	{
		QRect graphAreaRect;
		QRect graphDataGridRect;
		QRect verticalHeaderRect;
		QRect yAxisRect;
	} m_layout;

	Style m_style;
	Style m_effectiveStyle;
	qsizetype m_modelIndex = 0;
};

} // namespace shv::visu::timeline
