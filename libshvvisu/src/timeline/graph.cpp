#include "channelprobe.h"
#include "graphwidget.h"
#include "graphmodel.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QFontMetrics>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QPainterPath>
#include <QSettings>

#include <cmath>

static const QString USER_PROFILES_KEY = QStringLiteral("userProfiles");
static const QString SITES_KEY = QStringLiteral("sites");
static const QString VIEWS_KEY = QStringLiteral("channelViews");

namespace shv::visu::timeline {

const QString Graph::DEFAULT_USER_PROFILE = QStringLiteral("default");

static const int VALUE_NOT_AVILABLE_Y = std::numeric_limits<int>::max();

//==========================================
// Graph::GraphStyle
//==========================================
void Graph::Style::init(QWidget *widget)
{
	QFont f = widget->font();
	setFont(f);
	QFontMetrics fm(f, widget);
	setUnitSize(fm.lineSpacing());
}

//==========================================
// Graph
//==========================================
Graph::DataRect::DataRect() = default;
Graph::DataRect::DataRect(const XRange &xr, const YRange &yr)
	: xRange(xr), yRange(yr)
{
}

bool Graph::DataRect::isValid() const
{
	return xRange.isValid() && yRange.isValid();
}

Graph::Graph(QObject *parent)
	: QObject(parent)
	, m_cornerCellButtonBox(new GraphButtonBox({GraphButtonBox::ButtonId::Menu}, this))
{
	m_cornerCellButtonBox->setObjectName("cornerCellButtonBox");
	m_cornerCellButtonBox->setAutoRaise(false);
	connect(m_cornerCellButtonBox, &GraphButtonBox::buttonClicked, this, &Graph::onButtonBoxClicked);
}

Graph::~Graph()
{
	clearChannels();
}

const Graph::Style& Graph::effectiveStyle() const
{
	return  m_style;
}

void Graph::setModel(GraphModel *model)
{
	if(m_model)
		m_model->disconnect(this);
	m_model = model;
}

GraphModel *Graph::model() const
{
	return m_model;
}

void Graph::setSettingsUserName(const QString &user)
{
	m_settingsUserName = user;
}

#if SHVVISU_HAS_TIMEZONE
void Graph::setTimeZone(const QTimeZone &tz)
{
	shvDebug() << "set timezone:" << tz.id();
	m_timeZone = tz;
	emit presentationDirty(QRect());
}

QTimeZone Graph::timeZone() const
{
	return m_timeZone;
}
#endif

bool Graph::isInitialView() const
{
	if (m_channelFilter.isValid()) {
		return false;
	}
	GraphChannel::Style default_style;

	QMap<QString, qsizetype> path_to_model_index;
	for (qsizetype i = 0; i < m_model->channelCount(); ++i) {
		QString shv_path = m_model->channelShvPath(i);
		path_to_model_index[shv_path] = i;
	}
	QString previous_shv_path;
	for (GraphChannel *channel : m_channels) {
		if (channel->style().heightMax() != default_style.heightMax()) {
			return false;
		}
		QString channel_shv_path = m_model->channelShvPath(channel->modelIndex());
		if (channel_shv_path < previous_shv_path) {
			return false;
		}
		previous_shv_path = channel_shv_path;
	}
	return true;
}

void Graph::reset()
{
	createChannelsFromModel();
	m_channelFilter = ChannelFilter();
	Q_EMIT layoutChanged();
	Q_EMIT channelFilterChanged();
}

void Graph::createChannelsFromModel(shv::visu::timeline::Graph::SortChannels sorted)
{
	static QVector<QColor> colors {
		Qt::magenta,
		Qt::cyan,
		Qt::blue,
		QColor(0xe6, 0x3c, 0x33), //red
		QColor("orange"),
		QColor(0x6d, 0xa1, 0x3a), // green
	};
	QMap<QString, GraphChannel::Style> orig_styles;
	for (int i = 0; i < channelCount(); ++i) {
		auto *ch = channelAt(i);
		orig_styles[ch->shvPath()] = ch->style();
	}
	clearChannels();
	if(!m_model)
		return;
	QList<qsizetype> model_ixs;
	if(sorted == SortChannels::Yes) {
		// sort channels alphabetically
		QMap<QString, qsizetype> path_to_model_index;
		for (qsizetype i = 0; i < m_model->channelCount(); ++i) {
			QString shv_path = m_model->channelShvPath(i);
			path_to_model_index[shv_path] = i;
		}
		model_ixs = path_to_model_index.values();
	}
	else {
		for (qsizetype i = 0; i < m_model->channelCount(); ++i) {
			model_ixs << i;
		}
	}
	for(auto model_ix : model_ixs) {
		GraphChannel *ch = appendChannel(model_ix);
		auto channel_ix = channelCount() - 1;
		if (orig_styles.contains(ch->shvPath())) {
			ch->setStyle(orig_styles[ch->shvPath()]);
		}
		else {
			GraphChannel::Style style = ch->style();
			style.setColor(colors.value(channel_ix % colors.count()));
			ch->setStyle(style);
		}
	}
	resetChannelsRanges();
}

void Graph::resetChannelsRanges()
{
	if(!m_model)
		return;
	XRange x_range;
	for (qsizetype channel_ix = 0; channel_ix < channelCount(); ++channel_ix) {
		GraphChannel *ch = channelAt(channel_ix);
		auto model_ix = ch->modelIndex();
		YRange yrange = m_model->yRange(model_ix);
		if(yrange.isEmpty()) {
			if(yrange.max > 1)
				yrange = YRange{0, yrange.max};
			else if(yrange.max < -1)
				yrange = YRange{yrange.max, 0};
			else if(yrange.max < 0)
				yrange = YRange{-1, 0};
			else
				yrange = YRange{0, 1};
		}
		x_range = x_range.united(m_model->xRange(model_ix));
		setYRange(channel_ix, yrange);
	}
	setXRange(x_range);
}

qsizetype Graph::channelCount() const
{
	return  m_channels.count();
}

void Graph::clearChannels()
{
	qDeleteAll(m_channels);
	m_channels.clear();
}

shv::visu::timeline::GraphChannel *Graph::appendChannel(qsizetype model_index)
{
	if(model_index >= 0) {
		if(model_index >= m_model->channelCount())
			SHV_EXCEPTION("Invalid model index: " + std::to_string(model_index));
	}
	m_channels.append(new GraphChannel(this));
	GraphChannel *ch = m_channels.last();
	ch->setModelIndex(model_index < 0? m_channels.count() - 1: model_index);
	return ch;
}

shv::visu::timeline::GraphChannel *Graph::channelAt(qsizetype ix, bool throw_exc)
{
	if(ix < 0 || ix >= m_channels.count()) {
		if(throw_exc)
			SHV_EXCEPTION("Index out of range.");
		return nullptr;
	}
	return m_channels[ix];
}

const GraphChannel *Graph::channelAt(qsizetype ix, bool throw_exc) const
{
	if(ix < 0 || ix >= m_channels.count()) {
		if(throw_exc)
			SHV_EXCEPTION("Index out of range.");
		return nullptr;
	}
	return m_channels[ix];
}

shv::core::utils::ShvTypeDescr::Type Graph::channelTypeId(qsizetype ix) const
{
	if(!m_model)
		SHV_EXCEPTION("Graph model is NULL");
	const GraphChannel *ch = channelAt(ix);
	return m_model->channelInfo(ch->modelIndex()).typeDescr.type();
}

void Graph::moveChannel(qsizetype channel, qsizetype new_pos)
{
	if (channel > new_pos) {
		m_channels.insert(new_pos, m_channels.takeAt(channel));
	}
	else {
		m_channels.insert(new_pos - 1, m_channels.takeAt(channel));
	}
	emit presentationDirty(QRect());
	emit layoutChanged();
}

void Graph::showAllChannels()
{
	m_channelFilter.setMatchingPaths(channelPaths());

	emit layoutChanged();
	emit channelFilterChanged();
}

QStringList Graph::channelPaths()
{
	QStringList shv_paths;

	for (int i = 0; i < m_channels.count(); ++i) {
		shv_paths << m_channels[i]->shvPath();
	}

	return shv_paths;
}

void Graph::hideFlatChannels()
{
	QStringList matching_paths = (m_channelFilter.isValid()) ? m_channelFilter.matchingPaths() : channelPaths();

	for (qsizetype i = 0; i < m_channels.count(); ++i) {
		GraphChannel *ch = m_channels[i];
		if(isChannelFlat(ch)) {
			matching_paths.removeOne(ch->shvPath());
		}
	}

	m_channelFilter.setMatchingPaths(matching_paths);

	emit layoutChanged();
	emit channelFilterChanged();
}

bool Graph::isChannelFlat(GraphChannel *ch)
{
	YRange yrange = m_model->yRange(ch->modelIndex());
	return yrange.isEmpty();
}

void Graph::setChannelFilter(const ChannelFilter &filter)
{
	m_channelFilter = filter;
	emit layoutChanged();
	emit channelFilterChanged();
}

const ChannelFilter& Graph::channelFilter() const
{
	return m_channelFilter;
}

void Graph::setChannelVisible(qsizetype channel_ix, bool is_visible)
{
	GraphChannel *ch = channelAt(channel_ix);

	if (!m_channelFilter.isValid()) {
		m_channelFilter.setMatchingPaths(channelPaths());
	}
	if (is_visible) {
		m_channelFilter.addMatchingPath(ch->shvPath());
	}
	else {
		m_channelFilter.removeMatchingPath(ch->shvPath());
	}

	emit layoutChanged();
	emit channelFilterChanged();
}

void Graph::setChannelMaximized(qsizetype channel_ix, bool is_maximized)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->setMaximized(is_maximized);
	emit layoutChanged();
}

ChannelProbe *Graph::channelProbe(qsizetype channel_ix, timemsec_t time)
{
	ChannelProbe *probe = nullptr;

	for (ChannelProbe *p: m_channelProbes) {
		if (p->channelIndex() == channel_ix) {
			if ((probe == nullptr) || (qAbs(p->currentTime()-time) < (qAbs(probe->currentTime()-time))))
				probe = p;
		}
	}

	return probe;
}

ChannelProbe *Graph::addChannelProbe(qsizetype channel_ix, timemsec_t time)
{
	auto *probe = new ChannelProbe(this, channel_ix, time);
	m_channelProbes.push_back(probe);
	return probe;
}

void Graph::removeChannelProbe(ChannelProbe *probe)
{
	m_channelProbes.removeOne(probe);
	probe->deleteLater();
}

void Graph::setYAxisVisible(bool is_visible)
{
	m_style.setYAxisVisible(is_visible);
	emit layoutChanged();
}

bool Graph::isYAxisVisible()
{
	return 	m_style.yAxisVisible();
}

timemsec_t Graph::miniMapPosToTime(int pos) const
{
	auto pos2time = posToTimeFn(QPoint{m_layout.miniMapRect.left(), m_layout.miniMapRect.right()}, xRange());
	return pos2time? pos2time(pos): 0;
}

int Graph::miniMapTimeToPos(timemsec_t time) const
{
	auto time2pos = timeToPosFn(xRange(), WidgetRange{m_layout.miniMapRect.left(), m_layout.miniMapRect.right()});
	return time2pos? time2pos(time): 0;
}

timemsec_t Graph::posToTime(int pos) const
{
	auto pos2time = posToTimeFn(QPoint{m_layout.xAxisRect.left(), m_layout.xAxisRect.right()}, xRangeZoom());
	return pos2time? pos2time(pos): 0;
}

int Graph::timeToPos(timemsec_t time) const
{
	auto time2pos = timeToPosFn(xRangeZoom(), WidgetRange{m_layout.xAxisRect.left(), m_layout.xAxisRect.right()});
	return time2pos? time2pos(time): 0;
}

Sample Graph::timeToSample(qsizetype channel_ix, timemsec_t time) const
{
	GraphModel *m = model();
	const GraphChannel *ch = channelAt(channel_ix);
	qsizetype model_ix = ch->modelIndex();
	qsizetype ix1 = m->lessOrEqualTimeIndex(model_ix, time);
	if(ix1 < 0)
		return Sample();
	int interpolation = ch->m_effectiveStyle.interpolation();
	if(interpolation == GraphChannel::Style::Interpolation::None) {
		Sample s = m->sampleAt(model_ix, ix1);
		if(s.time == time)
			return s;
	}
	else if(interpolation == GraphChannel::Style::Interpolation::Stepped) {
		Sample s = m->sampleAt(model_ix, ix1);
		s.time = time;
		return s;
	}
	else if(interpolation == GraphChannel::Style::Interpolation::Line) {
		auto ix2 = ix1 + 1;
		if(ix2 >= m->count(model_ix))
			return Sample();
		Sample s1 = m->sampleAt(model_ix, ix1);
		Sample s2 = m->sampleAt(model_ix, ix2);
		if(s1.time == s2.time)
			return Sample();
		double d = s1.value.toDouble() + static_cast<double>(time - s1.time) * (s2.value.toDouble() - s1.value.toDouble()) / static_cast<double>(s2.time - s1.time);
		return Sample(time, d);
	}
	return Sample();
}

Sample Graph::timeToNearestSample(qsizetype channel_ix, timemsec_t time) const
{
	GraphModel *m = model();
	const GraphChannel *ch = channelAt(channel_ix);
	qsizetype model_ix = ch->modelIndex();
	qsizetype ix1 = m->lessOrEqualTimeIndex(model_ix, time);

	Sample s1;
	Sample s2;
	if (ix1 >= 0) {
		s1 = m->sampleAt(model_ix, ix1);
	}
	if (ix1 + 1 == m->count(model_ix)) {
		return s1;
	}
	s2 = m->sampleAt(model_ix, ix1 + 1);
	if (s1.isValid() && time - s1.time < s2.time - time) {
		return s1;
	}

	return s2;
}

Sample Graph::posToData(const QPoint &pos) const
{
	qsizetype ch_ix = posToChannel(pos);
	if(ch_ix < 0)
		return Sample();
	const GraphChannel *ch = channelAt(ch_ix);
	auto point2data = pointToDataFn(ch->graphDataGridRect(), DataRect{xRangeZoom(), ch->yRangeZoom()});
	return point2data? point2data(pos): Sample();
}

QPoint Graph::dataToPos(qsizetype ch_ix, const Sample &s) const
{
	const GraphChannel *ch = channelAt(ch_ix);
	auto data2point = dataToPointFn(DataRect{xRangeZoom(), ch->yRangeZoom()}, ch->graphDataGridRect());
	return data2point? data2point(s, channelTypeId(ch_ix)): QPoint();
}

Graph::CrossHairPos Graph::crossHairPos() const
{
	return m_state.crossHairPos;
}

void Graph::setCrossHairPos(const Graph::CrossHairPos &pos)
{
	m_state.crossHairPos = pos;
	QRect dirty_rect;
	for (qsizetype i = 0; i < channelCount(); ++i) {
		const GraphChannel *ch = channelAt(i, !shv::core::Exception::Throw);
		if(ch) {
			const QRect r = ch->graphDataGridRect();
			if(r.top() < m_layout.xAxisRect.top())
				dirty_rect = dirty_rect.united(r);
		}
	}
	dirty_rect = dirty_rect.united(m_layout.xAxisRect);
	emit presentationDirty(dirty_rect);
}

timemsec_t Graph::currentTime() const
{
	return m_state.currentTime;
}

void Graph::setCurrentTime(timemsec_t new_time)
{
	auto dirty_rect = [this](timemsec_t time) {
		QRect r;
		if(time > 0) {
			r = m_layout.rect;
			r.setX(timeToPos(time));
			r.adjust(-10, 0, 10, 0);
		}
		return r;
	};
	emit presentationDirty(dirty_rect(m_state.currentTime));
	m_state.currentTime = new_time;
	emit presentationDirty(dirty_rect(m_state.currentTime));
	emit presentationDirty(m_layout.xAxisRect);
}

void Graph::setSelectionRect(const QRect &rect)
{
	m_state.selectionRect = rect;
}

QRect Graph::selectionRect() const
{
	return m_state.selectionRect;
}

qsizetype Graph::posToChannel(const QPoint &pos) const
{
	for (qsizetype i = 0; i < channelCount(); ++i) {
		const GraphChannel *ch = channelAt(i);
		const QRect r = ch->graphAreaRect();
		if(r.contains(pos)) {
			return i;
		}
	}
	return -1;
}

qsizetype Graph::posToChannelHeader(const QPoint &pos) const
{
	for (qsizetype i = 0; i < channelCount(); ++i) {
		const GraphChannel *ch = channelAt(i);

		if(ch->verticalHeaderRect().contains(pos)) {
			return i;
		}
	}

	return -1;
}

XRange Graph::xRange() const
{
	return m_state.xRange;
}
XRange Graph::xRangeZoom() const
{
	return m_state.xRangeZoom;
}

void Graph::setXRange(const XRange &r, bool keep_zoom)
{
	const auto old_r = m_state.xRange;
	m_state.xRange = r;
	if(!m_state.xRangeZoom.isValid() || !keep_zoom) {
		m_state.xRangeZoom = m_state.xRange;
	}
	else {
		if(m_state.xRangeZoom.max == old_r.max) {
			const auto old_zr = m_state.xRangeZoom;
			m_state.xRangeZoom = m_state.xRange;
			m_state.xRangeZoom.min = m_state.xRangeZoom.max - old_zr.interval();
		}
	}
	sanityXRangeZoom();
	makeXAxis();
	clearMiniMapCache();
	emit presentationDirty(rect());
}

void Graph::setXRangeZoom(const XRange &r)
{
	XRange prev_r = m_state.xRangeZoom;
	XRange new_r = r;
	if(!m_state.xRange.contains(new_r.min))
		new_r.min = m_state.xRange.min;
	if(!m_state.xRange.contains(new_r.max))
		new_r.max = m_state.xRange.max;
	if(prev_r.max < new_r.max && prev_r.interval() == new_r.interval()) {
		/// if zoom is just moved right (not scaled), snap to xrange max
		const auto diff = m_state.xRange.max - new_r.max;
		if(diff < m_state.xRange.interval() / 20) {
			new_r.min += diff;
			new_r.max += diff;
		}
	}
	m_state.xRangeZoom = new_r;
	makeXAxis();
}

void Graph::resetXZoom()
{
	setXRangeZoom(xRange());
}

void Graph::setYRange(qsizetype channel_ix, const YRange &r)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->m_state.yRange = r;
	resetYZoom(channel_ix);
}

void Graph::enlargeYRange(qsizetype channel_ix, double step)
{
	GraphChannel *ch = channelAt(channel_ix);
	YRange r = ch->m_state.yRange;
	r.min -= step;
	r.max += step;
	setYRange(channel_ix, r);
}

void Graph::setYRangeZoom(qsizetype channel_ix, const YRange &r)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->m_state.yRangeZoom = r;
	if(ch->m_state.yRangeZoom.min < ch->m_state.yRange.min)
		ch->m_state.yRangeZoom.min = ch->m_state.yRange.min;
	if(ch->m_state.yRangeZoom.max > ch->m_state.yRange.max)
		ch->m_state.yRangeZoom.max = ch->m_state.yRange.max;
	makeYAxis(channel_ix);
}

void Graph::resetYZoom(qsizetype channel_ix)
{
	GraphChannel *ch = channelAt(channel_ix);
	setYRangeZoom(channel_ix, ch->yRange());
}

void Graph::zoomToSelection(bool zoom_vertically)
{
	shvLogFuncFrame();
	XRange xrange;
	xrange.min = posToTime(selectionRect().left());
	xrange.max = posToTime(selectionRect().right());
	xrange.normalize();
	if(zoom_vertically) {
		qsizetype ch1 = posToChannel(selectionRect().topLeft());
		qsizetype ch2 = posToChannel(selectionRect().bottomRight());
		if(ch1 == ch2 && ch1 >= 0) {
			const GraphChannel *ch = channelAt(ch1);
			if(ch) {
				YRange yrange;
				yrange.min = ch->posToValue(selectionRect().top());
				yrange.max = ch->posToValue(selectionRect().bottom());
				yrange.normalize();
				setYRangeZoom(ch1, yrange);
			}

		}
	}
	setXRangeZoom(xrange);
}

void Graph::sanityXRangeZoom()
{
	if(m_state.xRangeZoom.min < m_state.xRange.min)
		m_state.xRangeZoom.min = m_state.xRange.min;
	if(m_state.xRangeZoom.max > m_state.xRange.max)
		m_state.xRangeZoom.max = m_state.xRange.max;
}

void Graph::clearMiniMapCache()
{
	m_miniMapCache = QPixmap();
}

const Graph::Style& Graph::style() const
{
	return m_style;
}

void Graph::setStyle(const Graph::Style &st)
{
	m_style = st;
}

void Graph::setDefaultChannelStyle(const GraphChannel::Style &st)
{
	m_defaultChannelStyle = st;
}

GraphChannel::Style Graph::defaultChannelStyle() const
{
	return m_defaultChannelStyle;
}

QVariantMap Graph::mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const
{
	QVariantMap ret = base;
	QMapIterator<QString, QVariant> it(overlay);
	while(it.hasNext()) {
		it.next();
		ret[it.key()] = it.value();
	}
	return ret;
}

void Graph::makeXAxis()
{
	shvLogFuncFrame();
	static constexpr int64_t MSec = 1;
	static constexpr int64_t Sec = 1000 * MSec;
	static constexpr int64_t Min = 60 * Sec;
	static constexpr int64_t Hour = 60 * Min;
	static constexpr int64_t Day = 24 * Hour;
	static constexpr int64_t Month = 30 * Day;
	static constexpr int64_t Year = 365 * Day;
	static const std::map<int64_t, XAxis> intervals
	{
		{1 * MSec, {0, 1, XAxis::LabelScale::MSec}},
		{2 * MSec, {0, 2, XAxis::LabelScale::MSec}},
		{5 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{10 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{20 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{50 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{100 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{200 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{500 * MSec, {0, 5, XAxis::LabelScale::MSec}},
		{1 * Sec, {0, 1, XAxis::LabelScale::Sec}},
		{2 * Sec, {0, 2, XAxis::LabelScale::Sec}},
		{5 * Sec, {0, 5, XAxis::LabelScale::Sec}},
		{10 * Sec, {0, 5, XAxis::LabelScale::Sec}},
		{20 * Sec, {0, 5, XAxis::LabelScale::Sec}},
		{30 * Sec, {0, 3, XAxis::LabelScale::Sec}},
		{1 * Min, {0, 1, XAxis::LabelScale::Min}},
		{2 * Min, {0, 2, XAxis::LabelScale::Min}},
		{5 * Min, {0, 5, XAxis::LabelScale::Min}},
		{10 * Min, {0, 5, XAxis::LabelScale::Min}},
		{20 * Min, {0, 5, XAxis::LabelScale::Min}},
		{30 * Min, {0, 3, XAxis::LabelScale::Min}},
		{1 * Hour, {0, 1, XAxis::LabelScale::Hour}},
		{2 * Hour, {0, 2, XAxis::LabelScale::Hour}},
		{3 * Hour, {0, 3, XAxis::LabelScale::Hour}},
		{6 * Hour, {0, 6, XAxis::LabelScale::Hour}},
		{12 * Hour, {0, 6, XAxis::LabelScale::Hour}},
		{1 * Day, {0, 1, XAxis::LabelScale::Day}},
		{2 * Day, {0, 2, XAxis::LabelScale::Day}},
		{5 * Day, {0, 5, XAxis::LabelScale::Day}},
		{10 * Day, {0, 5, XAxis::LabelScale::Day}},
		{20 * Day, {0, 5, XAxis::LabelScale::Day}},
		{1 * Month, {0, 1, XAxis::LabelScale::Month}},
		{3 * Month, {0, 1, XAxis::LabelScale::Month}},
		{6 * Month, {0, 1, XAxis::LabelScale::Month}},
		{1 * Year, {0, 1, XAxis::LabelScale::Year}},
		{2 * Year, {0, 1, XAxis::LabelScale::Year}},
		{5 * Year, {0, 1, XAxis::LabelScale::Year}},
		{10 * Year, {0, 1, XAxis::LabelScale::Year}},
		{20 * Year, {0, 1, XAxis::LabelScale::Year}},
		{50 * Year, {0, 1, XAxis::LabelScale::Year}},
		{100 * Year, {0, 1, XAxis::LabelScale::Year}},
		{200 * Year, {0, 1, XAxis::LabelScale::Year}},
		{500 * Year, {0, 1, XAxis::LabelScale::Year}},
		{1000 * Year, {0, 1, XAxis::LabelScale::Year}},
	};
	int tick_units = 5;
	int tick_px = u2px(tick_units);
	timemsec_t t1 = posToTime(0);
	timemsec_t t2 = posToTime(tick_px);
	int64_t interval = t2 - t1;
	if(interval > 0) {
		auto lb = intervals.lower_bound(interval);
		if(lb == intervals.end())
			lb = --intervals.end();
		XAxis &axis = m_state.xAxis;
		axis = lb->second;
		axis.tickInterval = lb->first;
		shvDebug() << "interval:" << axis.tickInterval;
	}
	else {
		XAxis &axis = m_state.xAxis;
		axis.tickInterval = 0;
	}
}

void Graph::makeYAxis(qsizetype channel)
{
	shvLogFuncFrame();
	GraphChannel *ch = channelAt(channel);
	if(ch->yAxisRect().height() == 0)
		return;
	YRange range = ch->yRangeZoom();
	if(qFuzzyIsNull(range.interval()))
		return;
	int tick_units = 1;
	int tick_px = u2px(tick_units);
	double d1 = ch->posToValue(0);
	double d2 = ch->posToValue(tick_px);
	double tick_interval = d1 - d2;
	if(qFuzzyIsNull(tick_interval)) {
		shvError() << "channel:" << channel << "Y axis interval == 0";
		return;
	}
	shvDebug() << "range min:" << range.min << "max:"<< range.max << "interval:" << range.interval() << "tick interval:" << tick_interval;
	double pow = 1;
	if( tick_interval >= 1 ) {
		while(tick_interval >= 7) {
			tick_interval /= 10;
			pow *= 10;
		}
	}
	else {
		while(tick_interval > 0 && tick_interval < 1) {
			tick_interval *= 10;
			pow /= 10;
		}
	}

	GraphChannel::YAxis &axis = ch->m_state.axis;
	// snap to closest 1, 2, 5
	if(tick_interval < 1.5) {
		axis.tickInterval = 1 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(tick_interval < 3) {
		axis.tickInterval = 2 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(tick_interval < 7) {
		axis.tickInterval = 5 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(tick_interval < 10) {
		axis.tickInterval = 5 * pow * 10;
		axis.subtickEvery = tick_units;
	}
	else
		shvWarning() << "snapping interval error, interval:" << tick_interval;
	shvDebug() << channel << "axis.tickInterval:" << axis.tickInterval << "subtickEvery:" << axis.subtickEvery;
}

void Graph::moveSouthFloatingBarBottom(int bottom)
{
	// unfortunatelly called from draw() method
	m_layout.miniMapRect.moveBottom(bottom);
	m_layout.xAxisRect.moveBottom(m_layout.miniMapRect.top());
	m_layout.cornerCellRect.moveBottom(m_layout.miniMapRect.bottom());
	{
		int inset = m_cornerCellButtonBox->buttonSpace();
		m_cornerCellButtonBox->moveTopRight(m_layout.cornerCellRect.topRight() + QPoint(-inset, inset));
	}
}

std::pair<Sample, int> Graph::posToSample(const QPoint &pos) const
{
	auto ch_ix = posToChannel(pos);
	timemsec_t time = posToTime(pos.x());
	const GraphChannel *ch = channelAt(ch_ix);
	const GraphModel::ChannelInfo channel_info = model()->channelInfo(ch->modelIndex());
	shvDebug() << channel_info.shvPath << channel_info.typeDescr.toRpcValue().toCpon();
	auto channel_sample_type = channel_info.typeDescr.sampleType();
	Sample s;
	if (channel_sample_type == shv::core::utils::ShvTypeDescr::SampleType::Discrete) {
		s = timeToNearestSample(ch_ix, time);
	}
	else {
		s = timeToSample(ch_ix, time);
	}
	return {s, ch_ix};
}

QVariantMap Graph::sampleValues(qsizetype channel_ix, const shv::visu::timeline::Sample &s) const
{
	QVariantMap ret;
	const GraphChannel *ch = channelAt(channel_ix);
	const GraphModel::ChannelInfo channel_info = model()->channelInfo(ch->modelIndex());
	shvDebug() << channel_info.shvPath << channel_info.typeDescr.toRpcValue().toCpon();

	QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.time);
#if SHVVISU_HAS_TIMEZONE
	dt = dt.toTimeZone(timeZone());
#endif
	ret["sampleTime"] = dt;
	ret["sampleValue"] = s.value;
	auto rv = shv::coreqt::Utils::qVariantToRpcValue(s.value);
	const auto &type_info = model()->typeInfo();
	shvDebug() << "1:" << rv.toCpon();
	rv = type_info.applyTypeDescription(rv, channel_info.typeDescr);
	shvDebug() << "2:" << rv.toCpon();
	auto qv = shv::coreqt::Utils::rpcValueToQVariant(rv);
	ret["samplePrettyValue"] = qv;
	return ret;
}

const QRect& Graph::rect() const
{
	return  m_layout.rect;
}

const QRect& Graph::miniMapRect() const
{
	return  m_layout.miniMapRect;
}

const QRect& Graph::cornerCellRect() const
{
	return  m_layout.cornerCellRect;
}

QVariantMap Graph::toolTipValues(const QPoint &pos) const
{
	const auto[sample, channel_ix] = posToSample(pos);
	return sampleValues(channel_ix, sample);
}

QRect Graph::southFloatingBarRect() const
{
	QRect ret = m_layout.miniMapRect.united(m_layout.xAxisRect);
	ret.setX(0);
	return ret;
}

Graph::CrossHairPos::CrossHairPos() = default;

Graph::CrossHairPos::CrossHairPos(qsizetype ch_ix, const QPoint &pos)
	: channelIndex(ch_ix), possition(pos)
{
}

bool Graph::CrossHairPos::isValid() const
{
	return channelIndex >= 0 && !possition.isNull();
}

int Graph::u2px(double u) const
{
	return static_cast<int>(u2pxf(u));
}

double Graph::u2pxf(double u) const
{
	int sz = m_style.unitSize();
	return sz * u;
}

double Graph::px2u(int px) const
{
	double sz = m_style.unitSize();
	return (px / sz);
}

QString Graph::durationToString(timemsec_t duration)
{
	static constexpr timemsec_t SEC = 1000;
	static constexpr timemsec_t MIN = 60 * SEC;
	static constexpr timemsec_t HOUR = 60 * MIN;
	static constexpr timemsec_t DAY = 24 * HOUR;
	if(duration < MIN) {
		return tr("%1.%2 sec").arg(duration / SEC).arg(duration % SEC, 3, 10, QChar('0'));
	}
	if(duration < HOUR) {
		const auto min = duration / MIN;
		const auto sec = (duration % MIN) / SEC;
		const auto msec = duration % SEC;
		auto ret = tr("%1:%2.%3 min").arg(min).arg(sec, 2, 10, QChar('0')).arg(msec, 3, 10, QChar('0'));
		return ret;
	}
	if(duration < DAY) {
		const auto hour = duration / HOUR;
		const auto min = (duration % HOUR) / MIN;
		const auto sec = (duration % MIN) / SEC;
		return tr("%1:%2:%3", "time").arg(hour).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
	}
	{
		const auto day = duration / DAY;
		const auto hour = (duration % DAY) / HOUR;
		const auto min = (duration % HOUR) / MIN;
		const auto sec = (duration % MIN) / SEC;
		return tr("%1 day %1:%2:%3").arg(day).arg(hour).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
	}
}

static QString rstr(const QRect &r)
{
	return QStringLiteral("[%1, %2](%3, %4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

void Graph::makeLayout(const QRect &pref_rect)
{
	shvLogFuncFrame() << rstr(pref_rect);
	clearMiniMapCache();

	QSize graph_size;
	graph_size.setWidth(pref_rect.width());
	int grid_w = graph_size.width();
	int x_axis_pos = 0;
	x_axis_pos += u2px(m_style.leftMargin());
	x_axis_pos += u2px(m_style.verticalHeaderWidth());

	if (m_style.yAxisVisible())
		x_axis_pos += u2px(m_style.yAxisWidth());

	grid_w -= x_axis_pos;
	grid_w -= u2px(m_style.rightMargin());
	m_layout.miniMapRect.setHeight(u2px(m_style.miniMapHeight()));
	m_layout.miniMapRect.setLeft(x_axis_pos);
	m_layout.miniMapRect.setWidth(grid_w);

	m_layout.xAxisRect = m_layout.miniMapRect;
	m_layout.xAxisRect.setHeight(u2px(m_style.xAxisHeight()));

	m_layout.cornerCellRect = m_layout.xAxisRect;
	m_layout.cornerCellRect.setHeight(m_layout.cornerCellRect.height() + m_layout.miniMapRect.height());
	m_layout.cornerCellRect.setLeft(u2px(m_style.leftMargin()));
	m_layout.cornerCellRect.setWidth(m_layout.xAxisRect.left() - m_layout.cornerCellRect.left());

	// hide clickable areas, visible channels will show them again
	for (GraphChannel *ch : m_channels) {
		ch->m_layout.verticalHeaderRect = QRect();
		if(auto *bbx = ch->buttonBox())
			bbx->hide();
	}

	// set height of all channels to 0
	// if some channel is maximized, hidden channel must not interact with mouse
	for (int i = 0; i < m_channels.count(); ++i) {
		GraphChannel *ch = channelAt(i);
		ch->m_layout.graphAreaRect.setHeight(0);
	}
	QVector<int> visible_channels = visibleChannels();
	int sum_h_min = 0;
	struct Rest { int index; int rest; };
	QVector<Rest> rests;
	for (int i : visible_channels) {
		GraphChannel *ch = channelAt(i);
		ch->m_effectiveStyle = mergeMaps(m_defaultChannelStyle, ch->style());
		int ch_h = u2px(ch->m_effectiveStyle.heightMin());
		if(ch->isMaximized())
			rests << Rest{i, pref_rect.height()};
		else
			rests << Rest{i, u2px(ch->m_effectiveStyle.heightRange())};
		ch->m_layout.graphAreaRect.setLeft(x_axis_pos);
		ch->m_layout.graphAreaRect.setWidth(grid_w);
		ch->m_layout.graphAreaRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(m_style.channelSpacing());
	}
	sum_h_min += u2px(m_style.xAxisHeight());
	sum_h_min += u2px(m_style.miniMapHeight());
	qsizetype h_rest = pref_rect.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(m_style.topMargin());
	h_rest -= u2px(m_style.bottomMargin());
	if(h_rest > 0) {
		// distribute free widget height space to channel's rects heights
		std::sort(rests.begin(), rests.end(), [](const Rest &a, const Rest &b) {
			return a.rest < b.rest;
		});
		for (qsizetype i = 0; i < rests.count(); ++i) {
			auto fair_rest = h_rest / (rests.count() - i);
			const Rest &r = rests[i];
			GraphChannel *ch = channelAt(r.index);
			qsizetype h = u2px(ch->m_effectiveStyle.heightRange());
			if(h > fair_rest)
				h = fair_rest;
			ch->m_layout.graphAreaRect.setHeight(static_cast<int>(ch->m_layout.graphAreaRect.height() + h));
			h_rest -= h;
		}
	}
	// shift channel rects
	int widget_height = 0;
	widget_height += u2px(m_style.topMargin());
	for (auto i = visible_channels.count() - 1; i >= 0; --i) {
		GraphChannel *ch = channelAt(visible_channels[i]);

		ch->m_layout.graphAreaRect.moveTop(widget_height);

		ch->m_layout.verticalHeaderRect = ch->m_layout.graphAreaRect;
		ch->m_layout.verticalHeaderRect.setX(u2px(m_style.leftMargin()));
		ch->m_layout.verticalHeaderRect.setWidth(u2px(m_style.verticalHeaderWidth()));

		ch->m_layout.yAxisRect = ch->m_layout.verticalHeaderRect;
		ch->m_layout.yAxisRect.moveLeft(ch->m_layout.verticalHeaderRect.right());
		ch->m_layout.yAxisRect.setWidth((m_style.yAxisVisible()) ? u2px(m_style.yAxisWidth()) : 0);

		widget_height += ch->m_layout.graphAreaRect.height();
		if(i > 0)
			widget_height += u2px(m_style.channelSpacing());
	}

	// make data area rect a bit slimmer to not clip wide graph line
	for (int i : visible_channels) {
		GraphChannel *ch = channelAt(i);
		int line_width = u2px(ch->m_effectiveStyle.lineWidth());
		ch->m_layout.graphDataGridRect = ch->m_layout.graphAreaRect.adjusted(0, line_width, 0, -line_width);
		shvDebug() << "channel:" << i << "graphDataGridRect:" << rstr(ch->m_layout.graphDataGridRect);
		// place buttons
		GraphButtonBox *bbx = ch->buttonBox();
		if(bbx) {
			int inset = bbx->buttonSpace();
			bbx->moveTopRight(ch->m_layout.verticalHeaderRect.topRight() + QPoint(-inset, inset));
		}
	}

	m_layout.xAxisRect.moveTop(widget_height);
	widget_height += u2px(m_style.xAxisHeight());
	m_layout.miniMapRect.moveTop(widget_height);
	widget_height += u2px(m_style.miniMapHeight());
	widget_height += u2px(m_style.bottomMargin());

	graph_size.setHeight(widget_height);
	shvDebug() << "\tw:" << graph_size.width() << "h:" << graph_size.height();
	m_layout.rect = pref_rect;
	m_layout.rect.setSize(graph_size);
	shvDebug() << "m_layout.rect:" << rstr(m_layout.rect);

	makeXAxis();
	for (auto i = visible_channels.count() - 1; i >= 0; --i)
		makeYAxis(i);
}

void Graph::drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background, int inset)
{
	painter->save();
	if(background.isValid())
		painter->fillRect(rect, background);
	QPen pen;
	pen.setColor(color);
	painter->setPen(pen);
	painter->drawRect(rect);
	painter->setFont(font);
	painter->drawText(rect.adjusted(inset/2, 0, 0, 0), text);
	painter->restore();
}

void Graph::drawCenterTopText(QPainter *painter, const QPoint &top_center, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	QFontMetrics fm(font);
	QRect br = fm.boundingRect(text);
	int inset = u2px(0.2)*2;
	br.adjust(-inset, 0, inset, 0);
	br.moveCenter(top_center);
	br.moveTop(top_center.y());
	drawRectText(painter, br, text, font, color, background, inset);
}

void Graph::drawCenterBottomText(QPainter *painter, const QPoint &top_center, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	QFontMetrics fm(font);
	QRect br = fm.boundingRect(text);
	int inset = u2px(0.2)*2;
	br.adjust(-inset, 0, inset, 0);
	br.moveCenter(top_center);
	br.moveBottom(top_center.y());
	drawRectText(painter, br, text, font, color, background, inset);
}

void Graph::drawLeftCenterText(QPainter *painter, const QPoint &left_center, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	QFontMetrics fm(font);
	QRect br = fm.boundingRect(text);
	int inset = u2px(0.2)*2;
	br.adjust(-inset, 0, inset, 0);
	br.moveCenter(left_center);
	br.moveLeft(left_center.x());
	drawRectText(painter, br, text, font, color, background, inset);
}

QVector<int> Graph::visibleChannels() const
{
	QVector<int> visible_channels;
	int maximized_channel = maximizedChannelIndex();

	if (maximized_channel >= 0) {
		visible_channels << maximized_channel;
	}
	else if(m_channelFilter.isValid()) {
		for (int i = 0; i < m_channels.count(); ++i) {
			QString shv_path = model()->channelInfo(m_channels[i]->modelIndex()).shvPath;
			if(m_channelFilter.isPathMatch(shv_path)) {
				visible_channels << i;
			}
		}
	}
	else {
		for (int i = 0; i < m_channels.count(); ++i)
			visible_channels << i;
	}

	return visible_channels;
}

void Graph::setVisualSettings(const VisualSettings &settings)
{
	if (settings.isValid()) {
		QStringList new_filter;
		createChannelsFromModel();
		for (int i = 0; i < settings.channels.count(); ++i) {
			const VisualSettings::Channel &channel_settings = settings.channels[i];
			for (int j = i; j < m_channels.count(); ++j) {
				GraphChannel *channel = m_channels[j];
				if (channel->shvPath() == channel_settings.shvPath) {
					new_filter << channel_settings.shvPath;
					channel->setStyle(channel_settings.style);
					m_channels.insert(i, m_channels.takeAt(j));
					break;
				}
			}
		}

		m_channelFilter.setMatchingPaths(new_filter);
		Q_EMIT layoutChanged();
		Q_EMIT channelFilterChanged();
	}
	else {
		reset();
	}
}

void Graph::resizeChannelHeight(qsizetype ix, int delta_px)
{
	GraphChannel *ch = channelAt(ix);

	if (ch != nullptr) {
		GraphChannel::Style ch_style = ch->style();

		double h = px2u(ch->verticalHeaderRect().height() + (delta_px));

		if (h > GraphChannel::Style::DEFAULT_HEIGHT_MIN) {
			ch_style.setHeightMax(h);
			ch_style.setHeightMin(h);
			ch->setStyle(ch_style);
			Q_EMIT layoutChanged();
		}
	}
}

void Graph::resizeVerticalHeaderWidth(int delta_px)
{
	double w = m_style.verticalHeaderWidth() + px2u(delta_px);

	if (w > MIN_VERTICAL_HEADER_WIDTH && w < MAX_VERTICAL_HEADER_WIDTH) {
		m_style.setVerticalHeaderWidth(w);
		Q_EMIT layoutChanged();
	}
}

Graph::VisualSettings Graph::visualSettings() const
{
	VisualSettings view;
	if (!isInitialView()) {
		for (int ix : visibleChannels()) {
			const GraphChannel *channel = channelAt(ix);
			view.channels << VisualSettings::Channel{ channel->shvPath(), channel->style() };
		}
	}
	return view;
}

int Graph::maximizedChannelIndex() const
{
	for (int i = 0; i < m_channels.count(); ++i) {
		GraphChannel *ch = m_channels[i];
		if(ch->isMaximized()) {
			return i;
		}
	}

	return -1;
}

void Graph::draw(QPainter *painter, const QRect &dirty_rect, const QRect &view_rect)
{
	drawBackground(painter, dirty_rect);
	bool draw_cross_hair_time_marker = false;
	for (int i : visibleChannels()) {
		const GraphChannel *ch = channelAt(i);
		if(dirty_rect.intersects(ch->graphAreaRect())) {
			drawBackground(painter, i);
			drawGrid(painter, i);
			drawSamples(painter, i);
			drawProbes(painter, i);
			drawCrossHair(painter, i);
			draw_cross_hair_time_marker = true;
			drawCurrentTime(painter, i);
		}
		if(dirty_rect.intersects(ch->verticalHeaderRect()))
			drawVerticalHeader(painter, i);
		if(dirty_rect.intersects(ch->yAxisRect()))
			drawYAxis(painter, i);
	}
	if(draw_cross_hair_time_marker)
		drawCrossHairTimeMarker(painter);
	int minimap_bottom = view_rect.height() + view_rect.y();
	moveSouthFloatingBarBottom(minimap_bottom);
	if(dirty_rect.intersects(m_layout.cornerCellRect))
		drawCornerCell(painter);
	if(dirty_rect.intersects(miniMapRect()))
		drawMiniMap(painter);
	if(dirty_rect.intersects(m_layout.xAxisRect))
		drawXAxis(painter);
	if(dirty_rect.intersects(m_state.selectionRect))
		drawSelection(painter);
}

void Graph::drawBackground(QPainter *painter, const QRect &dirty_rect)
{
	painter->fillRect(dirty_rect, m_style.colorBackground());
}

void Graph::drawCornerCell(QPainter *painter)
{
	painter->fillRect(m_layout.cornerCellRect, m_style.colorPanel());

	QPen pen;
	pen.setWidth(u2px(0.1));
	pen.setColor(m_style.colorBackground());
	painter->setPen(pen);
	painter->drawRect(m_layout.cornerCellRect);
	m_cornerCellButtonBox->draw(painter);
}

void Graph::drawMiniMap(QPainter *painter)
{
	if(m_layout.miniMapRect.width() <= 0)
		return;
	if(m_miniMapCache.isNull()) {
		shvDebug() << "creating minimap cache";
		m_miniMapCache = QPixmap(m_layout.miniMapRect.width(), m_layout.miniMapRect.height());
		QRect mm_rect(QPoint(), m_layout.miniMapRect.size());
		int inset = mm_rect.height() / 10;
		mm_rect.adjust(0, inset, 0, -inset);
		QPainter p(&m_miniMapCache);
		QPainter *painter2 = &p;
		painter2->fillRect(mm_rect, m_defaultChannelStyle.colorBackground());
		QVector<int> visible_channels = visibleChannels();
		for (int i : visible_channels) {
			GraphChannel *ch = channelAt(i);
			GraphChannel::Style ch_st = ch->m_effectiveStyle;
			ch_st.setLineAreaStyle(GraphChannel::Style::LineAreaStyle::Blank);
			DataRect drect{xRange(), ch->yRange()};
			drawSamples(painter2, i, drect, mm_rect, ch_st);
		}
	}
	painter->drawPixmap(m_layout.miniMapRect.topLeft(), m_miniMapCache);
	int x1 = miniMapTimeToPos(xRangeZoom().min);
	int x2 = miniMapTimeToPos(xRangeZoom().max);
	painter->save();
	QPen pen;
	pen.setWidth(u2px(0.2));
	QPoint p1{x1, m_layout.miniMapRect.top()};
	QPoint p2{x1, m_layout.miniMapRect.bottom()};
	QPoint p3{x2, m_layout.miniMapRect.bottom()};
	QPoint p4{x2, m_layout.miniMapRect.top()};
	QColor bc(Qt::darkGray);
	bc.setAlphaF(0.8F);
	painter->fillRect(QRect{m_layout.miniMapRect.topLeft(), p2}, bc);
	painter->fillRect(QRect{p4, m_layout.miniMapRect.bottomRight()}, bc);
	pen.setColor(Qt::gray);
	painter->setPen(pen);
	painter->drawLine(m_layout.miniMapRect.topLeft(), p1);
	pen.setColor(Qt::white);
	painter->setPen(pen);
	painter->drawLine(p1, p2);
	painter->drawLine(p2, p3);
	painter->drawLine(p3, p4);
	pen.setColor(Qt::gray);
	painter->setPen(pen);
	painter->drawLine(p4, m_layout.miniMapRect.topRight());
	painter->restore();
}

QString Graph::channelName(qsizetype channel) const
{
	const GraphChannel *ch = channelAt(channel);
	const GraphModel::ChannelInfo& chi = model()->channelInfo(ch->modelIndex());
	QString name = chi.name;
	QString shv_path = chi.shvPath;
	if(name.isEmpty())
		name = shv_path;
	if(name.isEmpty())
		name = tr("<no name>");
	return name;
}

void Graph::drawVerticalHeader(QPainter *painter, int channel)
{
	int header_inset = u2px(m_style.headerInset());
	GraphChannel *ch = channelAt(channel);
	QColor c = m_style.color();
	QColor bc = m_style.colorPanel();
	QPen pen;
	pen.setColor(c);
	painter->setPen(pen);

	QString name = channelName(channel);
	QFont font = m_style.font();
	painter->save();
	painter->fillRect(ch->m_layout.verticalHeaderRect, bc);

	painter->setClipRect(ch->m_layout.verticalHeaderRect);
	{
		QRect r = ch->m_layout.verticalHeaderRect;
		r.setWidth(header_inset);
		painter->fillRect(r, ch->m_effectiveStyle.color());
	}

	font.setBold(true);
	painter->setFont(font);
	QRect text_rect = ch->m_layout.verticalHeaderRect.adjusted(2*header_inset, header_inset, -header_inset, -header_inset);
	painter->drawText(text_rect, name);

	painter->restore();
	GraphButtonBox *bbx = ch->buttonBox();
	if(bbx)
		bbx->draw(painter);
}

void Graph::drawBackground(QPainter *painter, int channel)
{
	const GraphChannel *ch = channelAt(channel);
	painter->fillRect(ch->m_layout.graphAreaRect, ch->m_effectiveStyle.colorBackground());
}

void Graph::drawGrid(QPainter *painter, int channel)
{
	const GraphChannel *ch = channelAt(channel);
	const XAxis &x_axis = m_state.xAxis;
	if(!x_axis.isValid()) {
		drawRectText(painter, ch->m_layout.graphAreaRect, "grid", m_style.font(), ch->m_effectiveStyle.colorGrid());
		return;
	}
	QColor gc = ch->m_effectiveStyle.colorGrid();
	if(!gc.isValid())
		return;
	painter->save();
	QPen pen_solid;
	pen_solid.setWidth(1);
	pen_solid.setColor(gc);
	painter->setPen(pen_solid);
	painter->drawRect(ch->m_layout.graphAreaRect);
	QPen pen_dot = pen_solid;
	pen_dot.setStyle(Qt::DotLine);
	painter->setPen(pen_dot);
	{
		// draw X-axis grid
		const XRange range = xRangeZoom();
		timemsec_t t1 = range.min / x_axis.tickInterval;
		t1 *= x_axis.tickInterval;
		if(t1 < range.min)
			t1 += x_axis.tickInterval;
		for (timemsec_t t = t1; t <= range.max; t += x_axis.tickInterval) {
			int x = timeToPos(t);
			QPoint p1{x, ch->graphDataGridRect().top()};
			QPoint p2{x, ch->graphDataGridRect().bottom()};
			painter->drawLine(p1, p2);
		}
	}
	{
		// draw Y-axis grid
		const YRange range = ch->yRangeZoom();
		const GraphChannel::YAxis &y_axis = ch->m_state.axis;
		double d1 = std::ceil(range.min / y_axis.tickInterval);
		d1 *= y_axis.tickInterval;
		if(d1 < range.min)
			d1 += y_axis.tickInterval;
		for (double d = d1; d <= range.max; d += y_axis.tickInterval) {
			int y = ch->valueToPos(d);
			QPoint p1{ch->graphDataGridRect().left(), y};
			QPoint p2{ch->graphDataGridRect().right(), y};
			if(qFuzzyIsNull(d)) {
				painter->setPen(pen_solid);
				painter->drawLine(p1, p2);
				painter->setPen(pen_dot);
			}
			else {
				painter->drawLine(p1, p2);
			}
		}
	}
	painter->restore();
}

void Graph::drawXAxis(QPainter *painter)
{
	painter->fillRect(m_layout.xAxisRect, m_style.colorPanel());
	const XAxis &axis = m_state.xAxis;
	if(!axis.isValid()) {
		return;
	}
	painter->save();
	painter->setClipRect(m_layout.xAxisRect);
	QFont font = m_style.font();
	QFontMetrics fm(font);
	QPen pen;
	pen.setWidth(u2px(0.1));
	int tick_len = u2px(axis.tickLen);

	pen.setColor(m_style.colorAxis());
	painter->setPen(pen);
	painter->drawLine(m_layout.xAxisRect.topLeft(), m_layout.xAxisRect.topRight());

	const XRange range = xRangeZoom();
	if(axis.subtickEvery > 1) {
		timemsec_t subtick_interval = axis.tickInterval / axis.subtickEvery;
		timemsec_t t0 = range.min / subtick_interval;
		t0 *= subtick_interval;
		if(t0 < range.min)
			t0 += subtick_interval;
		for (timemsec_t t = t0; t <= range.max; t += subtick_interval) {
			int x = timeToPos(t);
			QPoint p1{x, m_layout.xAxisRect.top()};
			QPoint p2{p1.x(), p1.y() + tick_len};
			painter->drawLine(p1, p2);
		}
	}
	timemsec_t t1 = range.min / axis.tickInterval;
	t1 *= axis.tickInterval;
	if(t1 < range.min)
		t1 += axis.tickInterval;
	for (timemsec_t t = t1; t <= range.max; t += axis.tickInterval) {
		int x = timeToPos(t);
		QPoint p1{x, m_layout.xAxisRect.top()};
		QPoint p2{p1.x(), p1.y() + 2*tick_len};
		painter->drawLine(p1, p2);
		auto date_time_tz = [this](timemsec_t epoch_msec) {
			QDateTime dt = QDateTime::fromMSecsSinceEpoch(epoch_msec);
#if SHVVISU_HAS_TIMEZONE
			if(m_timeZone.isValid())
				dt = dt.toTimeZone(m_timeZone);
#endif
			return dt;
		};
		QString text;
		switch (axis.labelScale) {
		case XAxis::LabelScale::MSec:
			text = QStringLiteral("%1.%2").arg((t / 1000) % 1000).arg(t % 1000, 3, 10, QChar('0'));
			break;
		case XAxis::LabelScale::Sec:
			text = QString::number((t / 1000) % 1000);
			break;
		case XAxis::LabelScale::Min:
		case XAxis::LabelScale::Hour: {
			QTime tm = date_time_tz(t).time();
			text = QStringLiteral("%1:%2").arg(tm.hour()).arg(tm.minute(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelScale::Day: {
			QDate dt = date_time_tz(t).date();
			text = QStringLiteral("%1/%2").arg(dt.month()).arg(dt.day(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelScale::Month: {
			QDate dt = date_time_tz(t).date();
			text = QStringLiteral("%1-%2").arg(dt.year()).arg(dt.month(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelScale::Year: {
			QDate dt = date_time_tz(t).date();
			text = QStringLiteral("%1").arg(dt.year());
			break;
		}
		}
		QRect r = fm.boundingRect(text);
		int inset = u2px(0.2);
		r.adjust(-inset, -inset, inset, inset);
		r.moveTopLeft(p2 + QPoint{-r.width() / 2, 0});
		painter->drawText(r, text);
	}
	{
		QString text;
		switch (axis.labelScale) {
		case XAxis::LabelScale::MSec:
		case XAxis::LabelScale::Sec:
			text = QStringLiteral("sec");
			break;
		case XAxis::LabelScale::Min:
		case XAxis::LabelScale::Hour: {
			text = QStringLiteral("hour:min");
			break;
		}
		case XAxis::LabelScale::Day: {
			text = QStringLiteral("month/day");
			break;
		}
		case XAxis::LabelScale::Month: {
			text = QStringLiteral("year-month");
			break;
		}
		case XAxis::LabelScale::Year: {
			text = QStringLiteral("year");
			break;
		}
		}
		text = '[' + text + ']';
		QRect r = fm.boundingRect(text);
		int inset = u2px(0.2);
		r.adjust(-inset, 0, inset, 0);
		r.moveTopLeft(m_layout.xAxisRect.topRight() + QPoint{-r.width() - u2px(0.2), 2*tick_len});
		painter->fillRect(r, m_style.colorPanel());
		painter->drawText(r, text);
	}
	auto current_time = m_state.currentTime;
	if(current_time > 0) {
		int x = timeToPos(current_time);
		if(x > m_layout.xAxisRect.left() + 10) {
			// draw marker + time string only if current time is slightly greater than graph leftmost time
			drawCurrentTimeMarker(painter, current_time);
		}
	}
	painter->restore();
}

void Graph::drawYAxis(QPainter *painter, int channel)
{
	if(!isYAxisVisible())
		return;

	const GraphChannel *ch = channelAt(channel);
	const GraphChannel::YAxis &axis = ch->m_state.axis;
	if(qFuzzyCompare(axis.tickInterval, 0)) {
		drawRectText(painter, ch->m_layout.yAxisRect, "y-axis", m_style.font(), ch->m_effectiveStyle.colorAxis());
		return;
	}
	painter->save();
	painter->fillRect(ch->m_layout.yAxisRect, m_style.colorPanel());
	QFont font = m_style.font();
	QFontMetrics fm(font);
	QPen pen;
	pen.setWidth(u2px(0.1));
	pen.setColor(ch->m_effectiveStyle.colorAxis());
	int tick_len = u2px(0.15);
	painter->setPen(pen);
	painter->drawLine(ch->m_layout.yAxisRect.bottomRight(), ch->m_layout.yAxisRect.topRight());

	const YRange range = ch->yRangeZoom();
	if(axis.subtickEvery > 1) {
		double subtick_interval = axis.tickInterval / axis.subtickEvery;
		double d0 = std::ceil(range.min / subtick_interval);
		d0 *= subtick_interval;
		if(d0 < range.min)
			d0 += subtick_interval;
		for (double d = d0; d <= range.max; d += subtick_interval) {
			int y = ch->valueToPos(d);
			QPoint p1{ch->m_layout.yAxisRect.right(), y};
			QPoint p2{p1.x() - tick_len, p1.y()};
			painter->drawLine(p1, p2);
		}
	}
	double d1 = std::ceil(range.min / axis.tickInterval);
	d1 *= axis.tickInterval;
	if(d1 < range.min)
		d1 += axis.tickInterval;
	for (double d = d1; d <= range.max; d += axis.tickInterval) {
		int y = ch->valueToPos(d);
		QPoint p1{ch->m_layout.yAxisRect.right(), y};
		QPoint p2{p1.x() - 2*tick_len, p1.y()};
		painter->drawLine(p1, p2);
		QString s = QString::number(d);
		QPoint label_pos;
		label_pos.setX(p2.x() - u2px(0.2) - fm.boundingRect(s).width());
		int digit_height = fm.ascent();
		if (d == d1) {
			label_pos.setY(p1.y());
		}
		else if (d == range.max) {
			label_pos.setY(p1.y() + digit_height);
		}
		else {
			label_pos.setY(p1.y() + digit_height/2);
		}
		painter->drawText(label_pos, s);
	}
	painter->restore();
}

std::function<QPoint (const Sample &s, Graph::TypeId meta_type_id)> Graph::dataToPointFn(const DataRect &src, const QRect &dest)
{
	using Int = int;
	Int le = dest.left();
	Int ri = dest.right();
	Int to = dest.top();
	Int bo = dest.bottom();

	timemsec_t t1 = src.xRange.min;
	timemsec_t t2 = src.xRange.max;
	double d1 = src.yRange.min;
	double d2 = src.yRange.max;

	if(t2 - t1 == 0)
		return nullptr;
	double kx = static_cast<double>(ri - le) / static_cast<double>(t2 - t1);
	if(std::abs(d2 - d1) < 1e-6)
		return nullptr;
	double ky = (to - bo) / (d2 - d1);

	return  [le, bo, kx, t1, d1, ky](const Sample &s, TypeId meta_type_id) -> QPoint {
		const timemsec_t t = s.time;
		double x = le + static_cast<double>(t - t1) * kx;
		// too big or too small pixel sizes can make painting problems
		static constexpr int MIN_INT2 = std::numeric_limits<int>::min() / 2;
		static constexpr int MAX_INT2 = std::numeric_limits<int>::max() / 2;
		int int_x = (x > MAX_INT2)? MAX_INT2 : (x < MIN_INT2)? MIN_INT2 : static_cast<int>(x);
		int int_y;
		if(shv::coreqt::Utils::isValueNotAvailable(s.value)) {
			int_y = VALUE_NOT_AVILABLE_Y;
		}
		else {
			bool ok;
			double d = GraphModel::valueToDouble(s.value, meta_type_id, &ok);
			if(!ok) {
				shvWarning() << "Don't know how to convert qt type:" << s.value.typeName() << "to shv type:" << shv::core::utils::ShvTypeDescr::typeToString(meta_type_id);
				return QPoint();
			}
			double y = bo + (d - d1) * ky;
			int_y = (y > MAX_INT2)? MAX_INT2 : (y < MIN_INT2)? MIN_INT2 : static_cast<int>(y);
		}

		return QPoint{int_x, int_y};
	};
}

std::function<Sample (const QPoint &)> Graph::pointToDataFn(const QRect &src, const DataRect &dest)
{
	int le = src.left();
	int ri = src.right();
	int to = src.top();
	int bo = src.bottom();

	if(ri - le == 0)
		return nullptr;

	timemsec_t t1 = dest.xRange.min;
	timemsec_t t2 = dest.xRange.max;
	double d1 = dest.yRange.min;
	double d2 = dest.yRange.max;

	double kx = static_cast<double>(t2 - t1) / (ri - le);
	if(to - bo == 0)
		return nullptr;
	double ky = (d2 - d1) / (to - bo);

	return  [t1, le, kx, d1, bo, ky](const QPoint &p) -> Sample {
		const int x = p.x();
		const int y = p.y();
		auto t = static_cast<timemsec_t>(static_cast<double>(t1) + (x - le) * kx);
		double d = d1 + (y - bo) * ky;
		return Sample{t, d};
	};
}

std::function<timemsec_t (int)> Graph::posToTimeFn(const QPoint &src, const XRange &dest)
{
	int le = src.x();
	int ri = src.y();
	if(ri - le == 0)
		return nullptr;
	timemsec_t t1 = dest.min;
	timemsec_t t2 = dest.max;
	return [t1, t2, le, ri](int x) {
		return static_cast<timemsec_t>(static_cast<double>(t1) + static_cast<double>(x - le) * static_cast<double>(t2 - t1) / (ri - le));
	};
}

std::function<int (timemsec_t)> Graph::timeToPosFn(const XRange &src, const timeline::Graph::WidgetRange &dest)
{
	timemsec_t t1 = src.min;
	timemsec_t t2 = src.max;
	if(t2 - t1 <= 0)
		return nullptr;
	int le = dest.min;
	int ri = dest.max;
	return [t1, t2, le, ri](timemsec_t t) {
		return static_cast<timemsec_t>(le + static_cast<double>(t - t1) * (ri - le) / static_cast<double>(t2 - t1));
	};
}

std::function<int (double)> Graph::valueToPosFn(const YRange &src, const Graph::WidgetRange &dest)
{
	double d1 = src.min;
	double d2 = src.max;
	if(std::abs(d2 - d1) < 1e-6)
		return nullptr;

	int y1 = dest.min;
	int y2 = dest.max;
	return [d1, d2, y1, y2](double val) {
		return static_cast<int>(y1 + (val - d1) * (y2 - y1) / (d2 - d1));
	};
}

std::function<double (int)> Graph::posToValueFn(const Graph::WidgetRange &src, const YRange &dest)
{
	int bo = src.min;
	int to = src.max;
	if(bo == to)
		return nullptr;

	double d1 = dest.min;
	double d2 = dest.max;
	return [d1, d2, to, bo](int y) {
		return d1 + (y - bo) * (d2 - d1) / (to - bo);
	};
}

void Graph::processEvent(QEvent *ev)
{
	if(ev->type() == QEvent::MouseMove
			|| ev->type() == QEvent::MouseButtonPress
			|| ev->type() == QEvent::MouseButtonRelease) {
		if(m_cornerCellButtonBox->processEvent(ev))
			return;
		for (int i = 0; i < channelCount(); ++i) {
			GraphChannel *ch = channelAt(i);
			GraphButtonBox *bbx = ch->buttonBox();
			if(bbx) {
				if(bbx->processEvent(ev))
					break;
			}
		}
	}
}

void Graph::emitPresentationDirty(const QRect &rect)
{
	emit presentationDirty(rect);
}

void Graph::emitChannelContextMenuRequest(int channel_index, const QPoint &mouse_pos)
{
	emit channelContextMenuRequest(channel_index, mouse_pos);
}

QString Graph::rectToString(const QRect &r)
{
	QString s = "%1,%2 %3x%4";
	return s.arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

void Graph::drawSamples(QPainter *painter, int channel_ix, const DataRect &src_rect, const QRect &dest_rect, const GraphChannel::Style &channel_style)
{
	const GraphChannel *ch = channelAt(channel_ix);
	shvLogFuncFrame() << "channel:" << channel_ix << ch->shvPath();
	auto model_ix = ch->modelIndex();
	QRect effective_dest_rect = dest_rect.isEmpty()? ch->graphDataGridRect(): dest_rect;
	GraphChannel::Style ch_style = channel_style.isEmpty()? ch->m_effectiveStyle: channel_style;

	XRange xrange;
	YRange yrange;
	if(src_rect.isValid()) {
		xrange = src_rect.xRange;
		yrange = src_rect.yRange;
	}
	else {
		xrange = xRangeZoom();
		yrange = ch->yRangeZoom();
	}
	if(xrange.isEmpty()) {
		// if we want to show snapshot only, add one second to make graph drawable
		xrange.max = xrange.min + 1000;
	}
	shvDebug() << "x-range min:" << xrange.min << "max:" << xrange.max << "interval:" << xrange.interval();
	auto sample2point = dataToPointFn(DataRect{xrange, yrange}, effective_dest_rect);

	if(!sample2point) {
		shvDebug() << "cannot construct sample2point() function";
		return;
	}

	int interpolation = ch_style.interpolation();

	QPen line_pen;
	QColor line_color = ch_style.color();
	line_pen.setColor(line_color);
	{
		double d = ch_style.lineWidth();
		line_pen.setWidthF(u2pxf(d));
	}
	line_pen.setCapStyle(Qt::FlatCap);
	QPen steps_join_pen = line_pen;
	{
		steps_join_pen.setWidthF(line_pen.widthF() / 2);
		auto c = line_pen.color();
		c.setAlphaF(0.6F);
		steps_join_pen.setColor(c);
	}
	painter->save();
	QRect clip_rect = effective_dest_rect.adjusted(0, -line_pen.width(), 0, line_pen.width());
	painter->setClipRect(clip_rect);
	painter->setPen(line_pen);
	QColor line_area_color;
	if(ch_style.lineAreaStyle() == GraphChannel::Style::LineAreaStyle::Filled) {
		line_area_color = line_color;
		line_area_color.setAlphaF(0.2F);
	}

	int sample_point_size = u2px(0.3);
	if(sample_point_size % 2 == 0)
		sample_point_size++; // make sample point size odd to have it center-able
	Graph::TypeId channel_meta_type_id = channelTypeId(channel_ix);
	GraphModel *graph_model = model();
	auto ix1 = graph_model->lessTimeIndex(model_ix, xrange.min);
	auto ix2 = graph_model->greaterTimeIndex(model_ix, xrange.max);
	auto samples_cnt = graph_model->count(model_ix);
	shvDebug() << "ix1:" << ix1 << "ix2:" << ix2 << "samples cnt:" << samples_cnt;
	static constexpr int NO_X = std::numeric_limits<int>::min();
	struct SamePixelValue {
		int x = NO_X;
		int y1 = 0;
		int y2 = 0;
		int minY = 0;
		int maxY = 0;

		SamePixelValue() = default;
		SamePixelValue(int x_, int y_) : x(x_), y1(y_), y2(y_), minY(y_), maxY(y_) {}
		SamePixelValue(const QPoint &p) : x(p.x()), y1(p.y()), y2(p.y()), minY(p.y()), maxY(p.y()) {}
		bool isValid() const { return x != NO_X; }
		bool isValueNotAvailable() const { return y1 == VALUE_NOT_AVILABLE_Y; }
	};
	SamePixelValue prev_point;
	if(ix1 < 0) {
		ix1 = 0;
	}
	else {
		prev_point = sample2point(graph_model->sampleAt(model_ix, ix1), channel_meta_type_id);
		ix1++;
	}
	shvDebug() << "iterating samples from:" << ix1 << "to:" << ix2 << "cnt:" << (ix2 - ix1 + 1);
	int x_axis_y = sample2point(Sample{xrange.min, 0}, channel_meta_type_id).y();
	// we need one more point, because the previous one is painted
	// when current is going to be active
	// we cannot paint current point unless we know y-range for more same-pixel values
	// this is why the sample paint is one step delayed
	for (auto i = ix1; i <= ix2; ++i) {
		shvDebug() << "processing sample on index:" << i;
		Q_ASSERT(i <= samples_cnt);
		QPoint current_point;
		if(i == samples_cnt) {
			// create fake point, move it one pixel right do be sure
			// that fake point x is greater then prev_point.x
			// in other case prev point will not be painted but just updated
			current_point = QPoint{effective_dest_rect.right() + 1, prev_point.y2};
		}
		else {
			Sample s = graph_model->sampleAt(model_ix, i);
			current_point = sample2point(s, channel_meta_type_id);
		}
		if(current_point.x() == prev_point.x) {
			prev_point.y2 = current_point.y();
			prev_point.minY = std::min(prev_point.minY, current_point.y());
			prev_point.maxY = std::max(prev_point.maxY, current_point.y());
		}
		else {
			if(prev_point.isValid()) {
				if(prev_point.isValueNotAvailable()) {
					// draw hashed area from prev to current x
					QRect rect = effective_dest_rect;
					rect.setLeft(prev_point.x);
					rect.setRight(current_point.x());
					QBrush brush(line_pen.color().lighter(), Qt::BDiagPattern);
					painter->fillRect(rect, brush);
				}
				else {
					// paint prev sample ymin-ymax area
					if(prev_point.isValid() && prev_point.maxY != prev_point.minY) {
						painter->drawLine(prev_point.x, prev_point.minY, prev_point.x, prev_point.maxY);
					}
					if (model()->channelInfo(ch->modelIndex()).typeDescr.sampleType() == shv::core::utils::ShvTypeDescr::SampleType::Discrete) {
						// draw arrow for discrete value
						QPoint arrow_heel{prev_point.x, 0};
						int arrow_width = u2px(1);
						painter->drawLine(arrow_heel.x(), clip_rect.y() + clip_rect.height() / 2, arrow_heel.x(), clip_rect.y() + clip_rect.height());
						QPainterPath path;
						path.moveTo(arrow_heel.x() - arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
						path.lineTo(arrow_heel.x() + arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
						path.lineTo(arrow_heel.x(), clip_rect.y() + clip_rect.height());
						path.lineTo(arrow_heel.x() - arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
						path.closeSubpath();
						painter->fillPath(path, painter->pen().color());
					}
					else if(interpolation == GraphChannel::Style::Interpolation::None) {
						if(line_area_color.isValid()) {
							// draw line from x axis to point
							QPoint p0{prev_point.x, x_axis_y};
							QPoint p1{prev_point.x, prev_point.maxY};
							painter->setPen(steps_join_pen);
							painter->drawLine(p0, p1);
						}
						painter->setBrush(line_pen.color());
						QRect r0{QPoint(), QSize{sample_point_size, sample_point_size}};
						r0.moveCenter(QPoint{prev_point.x, prev_point.y1});
						painter->drawEllipse(r0);
						if(prev_point.y1 != prev_point.y2) {
							r0.moveCenter(QPoint{prev_point.x, prev_point.y2});
							painter->drawEllipse(r0);
						}
					}
					else if(interpolation == GraphChannel::Style::Interpolation::Stepped) {
						// paint prev point and connection line to current one
						if(line_area_color.isValid()) {
							QPoint top_left{prev_point.x + 1, prev_point.y2};
							QPoint bottom_right{current_point.x(), x_axis_y};
							painter->fillRect(QRect{top_left, bottom_right}, line_area_color);
						}
						// draw vertical line lighter
						painter->setPen(steps_join_pen);
						painter->drawLine(QPoint{current_point.x(), prev_point.y2}, current_point);
						// draw horizontal line from prev point
						painter->setPen(line_pen);
						painter->drawLine(QPoint{prev_point.x, prev_point.y2}, QPoint{current_point.x(), prev_point.y2});
					}
					else if(interpolation == GraphChannel::Style::Interpolation::Line) {
						if(i < samples_cnt) {
							// connect real points only, not the fake one
							if(line_area_color.isValid()) {
								QPainterPath pp;
								pp.moveTo(QPoint{prev_point.x, prev_point.y2});
								pp.lineTo(current_point);
								pp.lineTo(QPoint{current_point.x(), x_axis_y});
								pp.lineTo(QPoint{prev_point.x, x_axis_y});
								pp.closeSubpath();
								painter->fillPath(pp, line_area_color);
							}
							painter->drawLine(QPoint{prev_point.x, prev_point.y2}, current_point);
						}
					}
				}
			}
			prev_point = current_point;
		}
	}
	painter->restore();
}

void Graph::drawCrossHairTimeMarker(QPainter *painter)
{
	if(!crossHairPos().isValid())
		return;
	auto crossbar_pos = crossHairPos().possition;
	if(m_layout.xAxisRect.left() >= crossbar_pos.x() || m_layout.xAxisRect.right() <= crossbar_pos.x()) {
		return;
	}
	QColor color = m_style.colorCrossHair();
	timemsec_t time = posToTime(crossHairPos().possition.x());
	int x = timeToPos(time);
	QPoint p1{x, m_layout.xAxisRect.top()};
	int tick_len = u2px(m_state.xAxis.tickLen)*2;
	{
		QRect r{0, 0, 2 * tick_len, tick_len};
		r.moveCenter(p1);
		r.moveBottom(p1.y());
		QPainterPath pp;
		pp.moveTo(r.topLeft());
		pp.lineTo(r.topRight());
		pp.lineTo(r.center().x(), r.bottom());
		pp.lineTo(r.topLeft());
		painter->fillPath(pp, color);
	}
	QString text = timeToStringTZ(time);
	p1.setY(p1.y() - tick_len);
	auto c_text = color;
	auto c_background = effectiveStyle().colorBackground();
	drawCenterBottomText(painter, p1 - QPoint{0, tick_len / 2}, text, m_style.font(), c_text, c_background);
}

void Graph::drawCrossHair(QPainter *painter, int channel_ix)
{
	if(!crossHairPos().isValid())
		return;
	auto crossbar_pos = crossHairPos().possition;
	const GraphChannel *ch = channelAt(channel_ix);
	if(ch->graphDataGridRect().left() >= crossbar_pos.x() || ch->graphDataGridRect().right() <= crossbar_pos.x()) {
		return;
	}
	painter->save();
	QColor color = m_style.colorCrossHair();
	QPen pen_solid;
	pen_solid.setColor(color);
	QPen pen_dash = pen_solid;
	QColor c50 = pen_solid.color();
	c50.setAlphaF(0.5);
	pen_dash.setColor(c50);
	pen_dash.setStyle(Qt::DashLine);
	painter->setPen(pen_dash);
	{
		/// draw vertical line
		QPoint p1{crossbar_pos.x(), ch->graphDataGridRect().top()};
		QPoint p2{crossbar_pos.x(), ch->graphDataGridRect().bottom()};
		painter->drawLine(p1, p2);
	}
	if(channel_ix == crossHairPos().channelIndex) {
		int d = u2px(0.4);
		QRect bulls_eye_rect(0, 0, d, d);
		bulls_eye_rect.moveCenter(crossbar_pos);
		/// draw horizontal line
		QPoint p1{ch->graphDataGridRect().left(), crossbar_pos.y()};
		QPoint p2{ch->graphDataGridRect().right(), crossbar_pos.y()};
		painter->drawLine(p1, p2);

		/// draw bulls-eye
		painter->setPen(pen_solid);
		painter->drawRect(bulls_eye_rect);
		{
			/// draw info
			QString info_text;
			if(selectionRect().isValid()) {
				/// show selection info
				auto sel_rect = selectionRect();
				auto ch1_ix = posToChannel(sel_rect.bottomLeft());
				auto ch2_ix = posToChannel(sel_rect.topLeft());
				auto *ch1 = channelAt(ch1_ix);
				auto *ch2 = channelAt(ch2_ix);
				auto t1 = posToTime(sel_rect.left());
				auto t2 = posToTime(sel_rect.right());
				auto y1 = ch1->posToValue(sel_rect.bottom());
				auto y2 = ch2->posToValue(sel_rect.top());
				info_text = tr("t1: %1").arg(timeToStringTZ(t1));
				info_text += '\n' + tr("duration: %1").arg(durationToString(t2 - t1));
				if(ch1 == ch2) {
					info_text += '\n' + tr("y1: %1").arg(y1);
					info_text += '\n' + tr("diff: %1").arg(y2 - y1);
				}
			}
			else {
				/// show sample value
				QVariant qv = toolTipValues(crossbar_pos).value("samplePrettyValue");
				QStringList lines;
#if QT_VERSION_MAJOR >= 6
				if(qv.typeId() == QMetaType::QVariantMap) {
#else
				if(qv.type() == QVariant::Map) {
#endif
					auto m = qv.toMap();
					for(auto it = m.begin(); it != m.end(); ++it) {
						lines << it.key() + ": " + it.value().toString();
					}
				}
				else {
					lines << qv.toString();
				}
				info_text = lines.join('\n');
			}
			if(!info_text.isEmpty()) {
				QFontMetrics fm(m_style.font());
				QRect info_rect = fm.boundingRect(QRect(), Qt::AlignLeft, info_text);
				info_rect.moveTopLeft(bulls_eye_rect.topLeft() + QPoint{20, 20});
				int offset = 5;
				auto r2 = info_rect.adjusted(-offset, 0, offset, 0);
				auto c = effectiveStyle().colorBackground();
				c.setAlphaF(0.5);
				painter->fillRect(r2, c);
				painter->drawText(info_rect, info_text);
				painter->drawRect(r2);
			}
		}
		{
			/// draw Y-marker
			int tick_len = u2px(m_state.xAxis.tickLen)*2;
			{
				QRect r{0, 0, tick_len, 2 * tick_len};
				r.moveCenter(p1);
				r.moveLeft(p1.x());
				QPainterPath pp;
				pp.moveTo(r.bottomRight());
				pp.lineTo(r.topRight());
				pp.lineTo(r.left(), r.center().y());
				pp.lineTo(r.bottomRight());
				painter->fillPath(pp, color);
			}
			/// draw Y value
			auto val = ch->posToValue(p1.y());
			auto c_text = color;
			auto c_background = effectiveStyle().colorBackground();
			c_background.setAlphaF(0.5);
			drawLeftCenterText(painter, p1 + QPoint{tick_len, 0}, QString::number(val), m_style.font(), c_text, c_background);
		}
		{
			/// draw point on current value graph
			timemsec_t time = posToTime(crossbar_pos.x());
			Sample s = timeToSample(channel_ix, time);
			if(s.value.isValid()) {
				QPoint p = dataToPos(channel_ix, s);
				int dd = u2px(0.3);
				QRect rect(0, 0, dd, dd);
				rect.moveCenter(p);
				painter->fillRect(rect, color);
				rect.adjust(2, 2, -2, -2);
				painter->fillRect(rect, ch->m_effectiveStyle.colorBackground());
			}
		}
	}
	painter->restore();
}

void Graph::drawProbes(QPainter *painter, int channel_ix)
{
	const GraphChannel *ch = channelAt(channel_ix);

	for (ChannelProbe *p: m_channelProbes) {
		auto time = p->currentTime();
		int x = timeToPos(time);

		if ((time >= 0) && (x >= ch->graphAreaRect().left() && x <= ch->graphAreaRect().right())) {
			painter->save();
			QPen pen;
			auto d = u2pxf(0.1);
			pen.setWidthF(d);
			pen.setColor(p->color());
			painter->setPen(pen);
			QPoint p1{x, ch->graphAreaRect().top()};
			QPoint p2{x, ch->graphAreaRect().bottom()};
			painter->drawLine(p1, p2);
			painter->restore();
		}
	}
}

void Graph::drawSelection(QPainter *painter)
{
	if(m_state.selectionRect.isNull())
		return;
	QColor c = m_style.colorSelection();
	c.setAlphaF(0.3F);
	painter->fillRect(m_state.selectionRect, c);
}

void Graph::drawCurrentTime(QPainter *painter, int channel_ix)
{
	auto time = m_state.currentTime;
	if(time <= 0)
		return;
	int x = timeToPos(time);
	const GraphChannel *ch = channelAt(channel_ix);
	if(ch->graphAreaRect().left() >= x || ch->graphAreaRect().right() <= x)
		return;

	painter->save();
	QPen pen;
	auto color = m_style.colorCurrentTime();
	auto d = u2pxf(0.2);
	pen.setWidthF(d);
	pen.setColor(color);
	painter->setPen(pen);
	QPoint p1{x, ch->graphAreaRect().top()};
	QPoint p2{x, ch->graphAreaRect().bottom()};
	painter->drawLine(p1, p2);
	painter->restore();
}

void Graph::drawCurrentTimeMarker(QPainter *painter, time_t time)
{
	if(time <= 0)
		return;
	QColor color = m_style.colorCurrentTime();
	int x = timeToPos(time);
	QPoint p1{x, m_layout.xAxisRect.top()};
	int tick_len = u2px(m_state.xAxis.tickLen)*2;
	{
		QRect r{0, 0, 2 * tick_len, tick_len};
		r.moveCenter(p1);
		r.moveTop(p1.y());
		QPainterPath pp;
		pp.moveTo(r.bottomLeft());
		pp.lineTo(r.bottomRight());
		pp.lineTo(r.center().x(), r.top());
		pp.lineTo(r.bottomLeft());
		painter->fillPath(pp, color);
	}
	QString text = timeToStringTZ(time);
	p1.setY(p1.y() + tick_len);
	drawCenterTopText(painter, p1, text, m_style.font(), color.darker(400), color);
}

QString Graph::timeToStringTZ(timemsec_t time) const
{
	QDateTime dt = QDateTime::fromMSecsSinceEpoch(time);
#if SHVVISU_HAS_TIMEZONE
	if(m_timeZone.isValid())
		dt = dt.toTimeZone(m_timeZone);
#endif
	QString text = dt.toString(Qt::ISODate);
	return text;
}

void Graph::onButtonBoxClicked(int button_id)
{
	shvLogFuncFrame();
	if(button_id == static_cast<int>(GraphButtonBox::ButtonId::Menu)) {
		QPoint pos = m_cornerCellButtonBox->buttonRect(static_cast<GraphButtonBox::ButtonId>(button_id)).center();
		emit graphContextMenuRequest(pos);
	}
}

void Graph::saveVisualSettings(const QString &settings_id, const QString &name) const
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(settings_id);
	settings.beginGroup(VIEWS_KEY);
	settings.setValue(name, visualSettings().toJson());
}

void Graph::loadVisualSettings(const QString &settings_id, const QString &name)
{
	VisualSettings graph_view;
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(settings_id);
	settings.beginGroup(VIEWS_KEY);
	setVisualSettings(VisualSettings::fromJson(settings.value(name).toString()));
}

void Graph::deleteVisualSettings(const QString &settings_id, const QString &name) const
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(settings_id);
	settings.beginGroup(VIEWS_KEY);
	settings.remove(name);
}

QStringList Graph::savedVisualSettingsNames(const QString &settings_id) const
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(settings_id);
	settings.beginGroup(VIEWS_KEY);
	return settings.childKeys();
}

QString Graph::VisualSettings::toJson() const
{
	if (isValid()) {
		QJsonArray settings;

		for (int i = 0; i < channels.count(); ++i) {
			settings << QJsonObject {
							{ "shvPath", channels[i].shvPath },
							{ "style", QJsonObject::fromVariantMap(channels[i].style) }
						};
		}
		return QJsonDocument(QJsonObject{{ "channels", settings }}).toJson(QJsonDocument::Compact);
	}

	return QString();
}

Graph::VisualSettings Graph::VisualSettings::fromJson(const QString &json)
{
	VisualSettings settings;

	if (!json.isEmpty()) {
		QJsonParseError parse_error;
		QJsonArray array = QJsonDocument::fromJson(json.toUtf8(), &parse_error).object()["channels"].toArray();
		if (parse_error.error == QJsonParseError::NoError) {
			for (int i = 0; i < array.count(); ++i) {
				QJsonObject item = array[i].toObject();
				settings.channels << VisualSettings::Channel({
											 item["shvPath"].toString(),
											 item["style"].toVariant().toMap()
										 });
			}
		}
		else {
			shvWarning() << "Error on parsing user settings" << parse_error.errorString();
		}
	}
	return settings;
}

bool Graph::VisualSettings::isValid() const
{
	return channels.count();
}

Graph::XAxis::XAxis() = default;
Graph::XAxis::XAxis(timemsec_t t, int se, LabelScale f)
	: tickInterval(t)
	, subtickEvery(se)
	  , labelScale(f)
{
}

bool Graph::XAxis::isValid() const
{
	return tickInterval > 0;
}
}
