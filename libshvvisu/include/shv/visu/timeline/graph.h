#pragma once

#include <shv/visu/timeline/channelfilter.h>
#include <shv/visu/timeline/channelprobe.h>
#include <shv/visu/timeline/graphchannel.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/sample.h>
#include <shv/visu/shvvisuglobal.h>

#include <shv/coreqt/utils.h>
#include <shv/core/exception.h>
#include <shv/core/utils/shvtypeinfo.h>

#include <optional>
#include <QObject>
#include <QVector>
#include <QVariantMap>
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QRect>
#if SHVVISU_HAS_TIMEZONE
#include <QTimeZone>
#endif

#ifdef WITH_SHV_SVG
class QSvgGenerator;
#endif

namespace shv::visu::timeline {

class SHVVISU_DECL_EXPORT Graph : public QObject
{
	Q_OBJECT
public:
	using TypeId = shv::core::utils::ShvTypeDescr::Type;
	struct SHVVISU_DECL_EXPORT DataRect
	{
		XRange xRange;
		YRange yRange;

		DataRect(); // create invalid DataRect
		DataRect(const XRange &xr, const YRange &yr);

		bool isValid() const;
	};

	using WidgetRange = Range<int>;

	static constexpr double MIN_VERTICAL_HEADER_WIDTH = 10;
	static constexpr double MAX_VERTICAL_HEADER_WIDTH = 25;

	static constexpr auto KEY_SAMPLE_TIME = "sampleTime";
	static constexpr auto KEY_SAMPLE_VALUE = "sampleValue";
	static constexpr auto KEY_SAMPLE_PRETTY_VALUE = "samplePrettyValue";

	class SHVVISU_DECL_EXPORT Style : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(int, u, setU, nitSize, 20) // px
		SHV_VARIANTMAP_FIELD2(double, h, setH, eaderInset, 0.2) // units
		SHV_VARIANTMAP_FIELD2(double, b, setB, uttonSize, 1.2) // units
		SHV_VARIANTMAP_FIELD2(double, l, setL, eftMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, r, setR, ightMargin, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, t, setT, opMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, b, setb, ottomMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, c, setC, hannelSpacing, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, x, setX, AxisHeight, 1.5) // units
		SHV_VARIANTMAP_FIELD2(double, y, setY, AxisWidth, 2.5) // units
		SHV_VARIANTMAP_FIELD2(double, m, setM, iniMapHeight, 2) // units
		SHV_VARIANTMAP_FIELD2(double, v, setV, erticalHeaderWidth, 15) // units
		SHV_VARIANTMAP_FIELD2(bool, is, set, SeparateChannels, true)
		SHV_VARIANTMAP_FIELD2(bool, is, set, YAxisVisible, true)
		SHV_VARIANTMAP_FIELD2(bool, is, set, RawDataVisible, true)

		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorForeground, QColor(0xc8, 0xc8, 0xc8))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorPanel, QColor(0x41, 0x43, 0x43))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::black))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorAxis, QColor(Qt::gray))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorCurrentTime, QColor(QStringLiteral("#cced5515")))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorCrossHair, QColor(QStringLiteral("white")))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorSelection, QColor(QStringLiteral("deepskyblue")))

		SHV_VARIANTMAP_FIELD(QFont, f, setF, ont)
	public:
		void init(QWidget *widget);
	};
public:
	static const QString DEFAULT_USER_PROFILE;

	class SHVVISU_DECL_EXPORT VisualSettings
	{
	public:
		class Channel
		{
		public:
			QString shvPath;
			GraphChannel::Style style;
		};

		QString toJson() const;
		static VisualSettings fromJson(const QString &json);
		QVector<Channel> channels;
		QString name;
	};

	Graph(QObject *parent = nullptr);
	~Graph() override;

	const Style& effectiveStyle() const;

	void setModel(GraphModel *model);
	GraphModel *model() const;

#if SHVVISU_HAS_TIMEZONE
	void setTimeZone(const QTimeZone &tz);
	QTimeZone timeZone() const;
#endif

	void setSettingsUserName(const QString &user);
	
	enum class SortChannels { No = 0, Yes };
	void createChannelsFromModel(SortChannels sorted = SortChannels::Yes);
	void resetChannelsRanges();

	qsizetype channelCount() const;
	QVector<int> visibleChannels() const;
	void clearChannels();
	GraphChannel* appendChannel(qsizetype model_index = -1);
	GraphChannel* channelAt(qsizetype ix, bool throw_exc = shv::core::Exception::Throw);
	const GraphChannel* channelAt(qsizetype ix, bool throw_exc = shv::core::Exception::Throw) const;
	core::utils::ShvTypeDescr::Type channelTypeId(qsizetype ix) const;
	void moveChannel(qsizetype channel, qsizetype new_pos);
	const GraphModel::ChannelInfo& channelInfo(qsizetype channel) const;

	void showAllChannels();
	QSet<QString> channelPaths();
	QMap<QString, QStringList> localizedChannelPaths();
	QSet<QString> flatChannels();
	const std::optional<ChannelFilter> &channelFilter() const;
	void setChannelFilter(const std::optional<ChannelFilter> &filter);
	void setChannelVisible(qsizetype channel_ix, bool is_visible);
	void setChannelMaximized(qsizetype channel_ix, bool is_maximized);

	ChannelProbe *channelProbe(qsizetype channel_ix, timemsec_t time = 0);
	ChannelProbe *addChannelProbe(qsizetype channel_ix, timemsec_t time);
	void removeChannelProbe(ChannelProbe *probe);

	timemsec_t miniMapPosToTime(int pos) const;
	int miniMapTimeToPos(timemsec_t time) const;

	timemsec_t posToTime(int pos) const;
	int timeToPos(timemsec_t time) const;
	Sample timeToSample(qsizetype channel_ix, timemsec_t time) const;
	std::optional<std::pair<Sample, int>> posToSample(const QPoint &pos) const;
	Sample timeToPreviousSample(qsizetype channel_ix, timemsec_t time) const;
	std::optional<qsizetype> posToChannel(const QPoint &pos) const;
	std::optional<qsizetype> posToChannelHeader(const QPoint &pos) const;
	Sample posToData(const QPoint &pos) const;
	QPoint dataToPos(qsizetype ch_ix, const Sample &s) const;

	QString timeToStringTZ(timemsec_t time) const;
	virtual QVariantMap sampleValues(qsizetype channel_ix, const Sample &s) const;
	const QRect& rect() const;
	const QRect& miniMapRect() const;
	const QRect& cornerCellRect() const;
	QRect southFloatingBarRect() const;
	struct SHVVISU_DECL_EXPORT CrossHairPos
	{
		qsizetype channelIndex = -1;
		QPoint position;

		CrossHairPos();
		CrossHairPos(qsizetype ch_ix, const QPoint &pos);

		bool isValid() const;
	};
	CrossHairPos crossHairPos() const;
	void setCrossHairPos(const CrossHairPos &pos);

	void setCurrentTime(timemsec_t time);
	timemsec_t currentTime() const;

	void setSelectionRect(const QRect &rect);
	QRect selectionRect() const;

	XRange xRange() const;
	XRange xRangeZoom() const;
	void setXRange(const XRange &r, bool keep_zoom = false);
	void setXRangeZoom(const XRange &r);
	void resetXZoom();

	void setYAxisVisible(bool is_visible);
	bool isYAxisVisible();

	void setYRange(qsizetype channel_ix, const YRange &r);
	void enlargeYRange(qsizetype channel_ix, double step);
	YRange yRangeZoom(qsizetype channel_ix) const;
	void setYRangeZoom(qsizetype channel_ix, const YRange &r);
	void resetYZoom(qsizetype channel_ix);

	enum class ZoomType { Horizontal, Vertical, ZoomToRect };
	void zoomToSelection(ZoomType zoom_type);
	void zoomToPreviousZoom() { popZoomRange(); }

	const Style& style() const;
	void setStyle(const Style &st);
	void setDefaultChannelStyle(const GraphChannel::Style &st);
	GraphChannel::Style defaultChannelStyle() const;

	void makeLayout(const QRect &pref_rect);

	void draw(QPainter *painter, const QRect &dirty_rect, const QRect &view_rect);
#ifdef WITH_SHV_SVG
	void draw(QSvgGenerator *svg_generator, const QRect &rect);
#endif
	int u2px(double u) const;
	double u2pxf(double u) const;
	double px2u(int px) const;

	QString durationToString(timemsec_t duration);

	static std::function<QPoint (const Sample &s, TypeId meta_type_id)> dataToPointFn(const DataRect &src, const QRect &dest);
	static std::function<Sample (const QPoint &)> pointToDataFn(const QRect &src, const DataRect &dest);
	static std::function<timemsec_t (int)> posToTimeFn(const QPoint &src, const XRange &dest);
	static std::function<int (timemsec_t)> timeToPosFn(const XRange &src, const WidgetRange &dest);
	static std::function<int (double)> valueToPosFn(const YRange &src, const WidgetRange &dest);
	static std::function<double (int)> posToValueFn(const WidgetRange &src, const YRange &dest);

	Q_SIGNAL void presentationDirty(const QRect &rect);
	void emitPresentationDirty(const QRect &rect);
	Q_SIGNAL void styleChanged();
	Q_SIGNAL void layoutChanged();
	Q_SIGNAL void channelFilterChanged();
	Q_SIGNAL void channelContextMenuRequest(int channel_index, const QPoint &mouse_pos);
	void emitChannelContextMenuRequest(int channel_index, const QPoint &mouse_pos);
	Q_SIGNAL void graphContextMenuRequest(const QPoint &mouse_pos);

	static QString rectToString(const QRect &r);

	VisualSettings visualSettings() const;
	void setVisualSettingsAndChannelFilter(const VisualSettings &settings);
	void resizeChannelHeight(qsizetype ix, int delta_px);
	void resizeVerticalHeaderWidth(int delta_px);

	void saveVisualSettings(const QString &settings_id, const QString &name) const;
	void deleteVisualSettings(const QString &settings_id, const QString &name) const;
	QStringList savedVisualSettingsNames(const QString &settings_id) const;
	void loadVisualSettings(const QString &settings_id, const QString &name);
	QString loadedVisualSettingsId();
protected:
	void sanityXRangeZoom();

	void clearMiniMapCache();

	void drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor(), int inset = 0);
	void drawCenterTopText(QPainter *painter, const QPoint &top_center, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());
	void drawCenterBottomText(QPainter *painter, const QPoint &top_center, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());
	void drawLeftCenterText(QPainter *painter, const QPoint &left_center, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());
	void drawLeftBottomText(QPainter *painter, const QPoint &left_bottom, const QString &text, const QFont &font, const QColor &color, const QColor &background);
	void drawRightBottomText(QPainter *painter, const QPoint &right_bottom, const QString &text, const QFont &font, const QColor &color, const QColor &background);

	int maximizedChannelIndex() const;

	bool isChannelFlat(GraphChannel *ch);

	void drawBackground(QPainter *painter, const QRect &dirty_rect);
	virtual void drawCornerCell(QPainter *painter);
	virtual void drawMiniMap(QPainter *painter);
	virtual void drawXAxis(QPainter *painter);

	virtual void drawVerticalHeader(QPainter *painter, int channel);
	virtual void drawBackground(QPainter *painter, int channel);
	virtual void drawGrid(QPainter *painter, int channel);
	virtual void drawYAxis(QPainter *painter, int channel);
	virtual void drawSamples(QPainter *painter, int channel_ix
			, const DataRect &src_rect = DataRect()
			, const QRect &dest_rect = QRect()
			, const GraphChannel::Style &channel_style = GraphChannel::Style());
	virtual void drawSamplesMinimap(QPainter *painter, int channel_ix
			, const DataRect &src_rect = DataRect()
			, const QRect &dest_rect = QRect()
			, const GraphChannel::Style &channel_style = GraphChannel::Style());
	void drawDiscreteValueInfo(QPainter *painter, const QLine &arrow_line, const QVariant &pretty_value, bool shadowed_sample, bool is_repeated);
	void drawCrossHairTimeMarker(QPainter *painter);
	virtual void drawCrossHair(QPainter *painter, int channel_ix);
	virtual void drawProbes(QPainter *painter, int channel_ix);
	virtual void drawSelection(QPainter *painter);
	virtual void drawCurrentTime(QPainter *painter, int channel_ix);
	void drawCurrentTimeMarker(QPainter *painter, time_t time);

	virtual void applyCustomChannelStyle(GraphChannel *channel);

	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;
	void makeXAxis();
	void makeYAxis(qsizetype channel);

	void moveSouthFloatingBarBottom(int bottom);
	QString elidedText(const QString &text, const QFont &font, const QRect &rect);
protected:
	QVariantMap toolTipValues(const QPoint &pos) const;
protected:
	GraphModel *m_model = nullptr;

#if SHVVISU_HAS_TIMEZONE
	QTimeZone m_timeZone;
#endif

	Style m_style;
	GraphChannel::Style m_defaultChannelStyle;

	QVector<ChannelProbe*> m_channelProbes;
	QVector<GraphChannel*> m_channels;
	std::optional<ChannelFilter> m_channelFilter;

	struct SHVVISU_DECL_EXPORT XAxis
	{
		enum class LabelScale {MSec, Sec, Min, Hour, Day, Month, Year, Value, Kilo, Mega, Giga};
		timemsec_t tickInterval = 0;
		int subtickEvery = 1;
		double tickLen = 0.15;
		LabelScale labelScale = LabelScale::MSec;

		XAxis();
		XAxis(timemsec_t t, int se, LabelScale f);
		bool isValid() const;
	};
	struct
	{
		XRange xRange;
		XRange xRangeZoom;
		CrossHairPos crossHairPos;
		timemsec_t currentTime = 0;
		QRect selectionRect;
		XAxis xAxis;
	} m_state;

	struct
	{
		QRect rect;
		QRect miniMapRect;
		QRect xAxisRect;
		QRect cornerCellRect;
	} m_layout;

	QPixmap m_miniMapCache;
	QString m_settingsUserName = DEFAULT_USER_PROFILE;
private:
	struct ZoomRange {
		XRange xRange;
		YRange yRange;
		qsizetype channelIx = -1;
	};
	QList<ZoomRange> m_zoomStack;
private:
	void applyZoomRange(const ZoomRange &r);
	void pushZoomRange(const ZoomRange &r);
	void popZoomRange();
};

}
