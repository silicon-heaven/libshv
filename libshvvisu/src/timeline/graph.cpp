#include "channelprobe.h"
#include "graphwidget.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QPainterPath>

#include <cmath>

namespace cp = shv::chainpack;

namespace shv {
namespace visu {
namespace timeline {

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

void Graph::createChannelsFromModel()
{
	static QVector<QColor> colors {
		Qt::magenta,
		Qt::cyan,
		Qt::blue,
		QColor("#e63c33"), //red
		QColor("orange"),
		QColor("#6da13a"), // green
	};
	clearChannels();
	if(!m_model)
		return;
	// sort channels alphabetically
	QMap<QString, int> path_to_model_index;
	for (int i = 0; i < m_model->channelCount(); ++i) {
		QString shv_path = m_model->channelShvPath(i);
		path_to_model_index[shv_path] = i;
	}
	for(const auto &shv_path : path_to_model_index.keys()) {
		int model_ix = path_to_model_index[shv_path];
		shvDebug() << "adding channel:" << shv_path;
		//shvInfo() << "new channel:" << model_ix;
		GraphChannel *ch = appendChannel(model_ix);
		//ch->buttonBox()->setObjectName(QString::fromStdString(shv_path));
		int channel_ix = channelCount() - 1;
		GraphChannel::Style style = ch->style();
		style.setColor(colors.value(channel_ix % colors.count()));
		ch->setStyle(style);
		//ch->setMetaTypeId(m_model->guessMetaType(model_ix));
	}
	resetChannelsRanges();
}

void Graph::resetChannelsRanges()
{
	if(!m_model)
		return;
	XRange x_range;
	for (int channel_ix = 0; channel_ix < channelCount(); ++channel_ix) {
		GraphChannel *ch = channelAt(channel_ix);
		int model_ix = ch->modelIndex();
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

void Graph::clearChannels()
{
	qDeleteAll(m_channels);
	m_channels.clear();
}

shv::visu::timeline::GraphChannel *Graph::appendChannel(int model_index)
{
	if(model_index >= 0) {
		if(model_index >= m_model->channelCount())
			SHV_EXCEPTION("Invalid model index: " + std::to_string(model_index));
	}
	m_channels.append(new GraphChannel(this));
	GraphChannel *ch = m_channels.last();
	ch->setModelIndex(model_index < 0? m_channels.count() - 1: model_index);
	if (m_model->channelInfo(model_index).typeDescr.sampleType == shv::chainpack::DataChange::SampleType::Discrete) {
		auto style = ch->style();
		style.setInterpolation(GraphChannel::Style::Interpolation::None);
		ch->setStyle(style);
	}
	return ch;
}

shv::visu::timeline::GraphChannel *Graph::channelAt(int ix, bool throw_exc)
{
	if(ix < 0 || ix >= m_channels.count()) {
		if(throw_exc)
			SHV_EXCEPTION("Index out of range.");
		return nullptr;
	}
	return m_channels[ix];
}

const GraphChannel *Graph::channelAt(int ix, bool throw_exc) const
{
	if(ix < 0 || ix >= m_channels.count()) {
		if(throw_exc)
			SHV_EXCEPTION("Index out of range.");
		return nullptr;
	}
	return m_channels[ix];
}

int Graph::channelMetaTypeId(int ix) const
{
	if(!m_model)
		SHV_EXCEPTION("Graph model is NULL");
	const GraphChannel *ch = channelAt(ix);
	return m_model->channelInfo(ch->modelIndex()).metaTypeId;
}

void Graph::moveChannel(int channel, int new_pos)
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

QSet<QString> Graph::channelPaths()
{
	QSet<QString> shv_paths;

	for (int i = 0; i < m_channels.count(); ++i) {
		shv_paths.insert(m_channels[i]->shvPath());
	}

	return shv_paths;
}

void Graph::hideFlatChannels()
{
	QSet<QString> matching_paths = m_channelFilter.matchingPaths();

	for (int i = 0; i < m_channels.count(); ++i) {
		GraphChannel *ch = m_channels[i];
		if(isChannelFlat(ch)) {
			matching_paths.remove(ch->shvPath());
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

void Graph::setChannelVisible(int channel_ix, bool is_visible)
{
	GraphChannel *ch = channelAt(channel_ix);

	if (is_visible) {
		m_channelFilter.addMatchingPath(ch->shvPath());
	}
	else {
		m_channelFilter.removeMatchingPath(ch->shvPath());
	}

	emit layoutChanged();
	emit channelFilterChanged();
}

void Graph::setChannelMaximized(int channel_ix, bool is_maximized)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->setMaximized(is_maximized);
	emit layoutChanged();
}

ChannelProbe *Graph::channelProbe(int channel_ix, timemsec_t time)
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

ChannelProbe *Graph::addChannelProbe(int channel_ix, timemsec_t time)
{
	ChannelProbe *probe = new ChannelProbe(this, channel_ix, time);
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

Sample Graph::timeToSample(int channel_ix, timemsec_t time) const
{
	GraphModel *m = model();
	const GraphChannel *ch = channelAt(channel_ix);
	int model_ix = ch->modelIndex();
	int ix1 = m->lessOrEqualIndex(model_ix, time);
	if(ix1 < 0)
		return Sample();
	int interpolation = ch->m_effectiveStyle.interpolation();
	//shvInfo() << channel_ix << "interpolation:" << interpolation;
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
		int ix2 = ix1 + 1;
		if(ix2 >= m->count(model_ix))
			return Sample();
		Sample s1 = m->sampleAt(model_ix, ix1);
		Sample s2 = m->sampleAt(model_ix, ix2);
		if(s1.time == s2.time)
			return Sample();
		double d = s1.value.toDouble() + (time - s1.time) * (s2.value.toDouble() - s1.value.toDouble()) / (s2.time - s1.time);
		return Sample(time, d);
	}
	return Sample();
}

Sample Graph::nearestSample(int channel_ix, timemsec_t time) const
{
	GraphModel *m = model();
	const GraphChannel *ch = channelAt(channel_ix);
	int model_ix = ch->modelIndex();
	int ix1 = m->lessOrEqualIndex(model_ix, time);

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
	else {
		return s2;
	}
}

Sample Graph::posToData(const QPoint &pos) const
{
	int ch_ix = posToChannel(pos);
	if(ch_ix < 0)
		return Sample();
	const GraphChannel *ch = channelAt(ch_ix);
	auto point2data = pointToDataFn(ch->graphDataGridRect(), DataRect{xRangeZoom(), ch->yRangeZoom()});
	return point2data? point2data(pos): Sample();
}

QPoint Graph::dataToPos(int ch_ix, const Sample &s) const
{
	const GraphChannel *ch = channelAt(ch_ix);
	auto data2point = dataToPointFn(DataRect{xRangeZoom(), ch->yRangeZoom()}, ch->graphDataGridRect());
	return data2point? data2point(s, channelMetaTypeId(ch_ix)): QPoint();
}

QMap<QString, QString> Graph::yValuesToMap(int channel_ix, const shv::visu::timeline::Sample &s) const
{
	QMap<QString, QString> ret;

	if (s.isValid()) {
		GraphModel::ChannelInfo &channel_info = model()->channelInfo(m_channels[channel_ix]->modelIndex());

		if (channel_info.typeDescr.sampleType != shv::chainpack::DataChange::SampleType::Invalid) {
			if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Map) {
				const QVariantMap &map = s.value.toMap();
				for (auto it = map.cbegin(); it != map.cend(); ++it) {
					QString value = it.value().toString();
					for (auto &field : channel_info.typeDescr.fields) {
						if (QString::fromStdString(field.name) == it.key() && field.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Enum) {
							value = model()->typeDescrFieldName(field.typeDescr, it.value().toInt());
							break;
						}
					}
					ret[it.key()] = value;
				}
			}
			else if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::IMap) {
				const QVariantMap &map = s.value.toMap();
				for (auto it = map.cbegin(); it != map.cend(); ++it) {
					for (auto &field : channel_info.typeDescr.fields) {
						if (it.key().toInt() == field.value.toInt()) {
							QString value;
							if (field.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Enum) {
								value = model()->typeDescrFieldName(field.typeDescr, it.value().toInt());
							}
							else {
								value = it.value().toString();
							}
							ret[QString::fromStdString(field.name)] = value;
							break;
						}
					}
				}
			}
			else if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Enum) {
				QStringList p = channel_info.shvPath.split("/");
				if (p.isEmpty()) {
					shvError() << "Failed to split shvPath:" << channel_info.shvPath;
				}

				QString key = (p.isEmpty()) ? "y" : p.last();
				ret[key] = model()->typeDescrFieldName(channel_info.typeDescr, s.value.toInt());
			}
			else {
				QStringList p = channel_info.shvPath.split("/");
				if (p.isEmpty()) {
					shvError() << "Failed to split shvPath:" << channel_info.shvPath;
				}

				QString key = (p.isEmpty()) ? "y" : p.last();
				ret[key] = s.value.toString();
			}
		}
	}

	return ret;
}

void Graph::setCrossHairPos(const Graph::CrossHairPos &pos)
{
	//{
	//	const GraphChannel *ch = channelAt(crossBarPos().channelIndex, !shv::core::Exception::Throw);
	//	if(ch)
	//		emit emitPresentationDirty(ch->graphDataGridRect());
	//}
	m_state.crossHairPos = pos;
	//{
	//	const GraphChannel *ch = channelAt(crossBarPos().channelIndex, !shv::core::Exception::Throw);
	//	if(ch)
	//		emit emitPresentationDirty(ch->graphDataGridRect());
	//}
	QRect dirty_rect;
	for (int i = 0; i < channelCount(); ++i) {
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

void Graph::setCurrentTime(timemsec_t time)
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
	m_state.currentTime = time;
	emit presentationDirty(dirty_rect(m_state.currentTime));
	emit presentationDirty(m_layout.xAxisRect);
}

void Graph::setSelectionRect(const QRect &rect)
{
	m_state.selectionRect = rect;
}

int Graph::posToChannel(const QPoint &pos) const
{
	for (int i = 0; i < channelCount(); ++i) {
		const GraphChannel *ch = channelAt(i);
		const QRect r = ch->graphAreaRect();
		//shvInfo() << ch->shvPath()
		//		  << QString("[%1, %2]").arg(pos.x()).arg(pos.y())
		//		  << "in"
		//		  << QString("[%1, %2](%3 x %4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
		if(r.contains(pos)) {
			return i;
		}
	}
	return -1;
}

int Graph::posToChannelHeader(const QPoint &pos) const
{
	for (int i = 0; i < channelCount(); ++i) {
		const GraphChannel *ch = channelAt(i);

		if(ch->verticalHeaderRect().contains(pos)) {
			return i;
		}
	}

	return -1;
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
	if(new_r.min < m_state.xRange.min)
		new_r.min = m_state.xRange.min;
	if(new_r.max > m_state.xRange.max)
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

void Graph::setYRange(int channel_ix, const YRange &r)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->m_state.yRange = r;
	resetZoom(channel_ix);
}

void Graph::enlargeYRange(int channel_ix, double step)
{
	GraphChannel *ch = channelAt(channel_ix);
	YRange r = ch->m_state.yRange;
	r.min -= step;
	r.max += step;
	setYRange(channel_ix, r);
}

void Graph::setYRangeZoom(int channel_ix, const YRange &r)
{
	GraphChannel *ch = channelAt(channel_ix);
	ch->m_state.yRangeZoom = r;
	if(ch->m_state.yRangeZoom.min < ch->m_state.yRange.min)
		ch->m_state.yRangeZoom.min = ch->m_state.yRange.min;
	if(ch->m_state.yRangeZoom.max > ch->m_state.yRange.max)
		ch->m_state.yRangeZoom.max = ch->m_state.yRange.max;
	makeYAxis(channel_ix);
}

void Graph::resetZoom(int channel_ix)
{
	GraphChannel *ch = channelAt(channel_ix);
	setYRangeZoom(channel_ix, ch->yRange());
}

void Graph::zoomToSelection(bool zoom_vertically)
{
	shvLogFuncFrame();
	XRange xrange;
	xrange.min = posToTime(m_state.selectionRect.left());
	xrange.max = posToTime(m_state.selectionRect.right());
	xrange.normalize();
	if(zoom_vertically) {
		int ch1 = posToChannel(m_state.selectionRect.topLeft());
		int ch2 = posToChannel(m_state.selectionRect.bottomRight());
		if(ch1 == ch2 && ch1 >= 0) {
			const GraphChannel *ch = channelAt(ch1);
			if(ch) {
				YRange yrange;
				yrange.min = ch->posToValue(m_state.selectionRect.top());
				yrange.max = ch->posToValue(m_state.selectionRect.bottom());
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

void Graph::setStyle(const Graph::Style &st)
{
	m_style = st;
}

void Graph::setDefaultChannelStyle(const GraphChannel::Style &st)
{
	m_defaultChannelStyle = st;
}

QVariantMap Graph::mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const
{
	QVariantMap ret = base;
	QMapIterator<QString, QVariant> it(overlay);
	//shvDebug() << "====================== merge ====================";
	while(it.hasNext()) {
		it.next();
		//shvDebug() << it.key() << "<--" << it.value();
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
		{1 * MSec, {0, 1, XAxis::LabelFormat::MSec}},
		{2 * MSec, {0, 2, XAxis::LabelFormat::MSec}},
		{5 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{10 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{20 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{50 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{100 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{200 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{500 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{1 * Sec, {0, 1, XAxis::LabelFormat::Sec}},
		{2 * Sec, {0, 2, XAxis::LabelFormat::Sec}},
		{5 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{10 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{20 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{30 * Sec, {0, 3, XAxis::LabelFormat::Sec}},
		{1 * Min, {0, 1, XAxis::LabelFormat::Min}},
		{2 * Min, {0, 2, XAxis::LabelFormat::Min}},
		{5 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{10 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{20 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{30 * Min, {0, 3, XAxis::LabelFormat::Min}},
		{1 * Hour, {0, 1, XAxis::LabelFormat::Hour}},
		{2 * Hour, {0, 2, XAxis::LabelFormat::Hour}},
		{3 * Hour, {0, 3, XAxis::LabelFormat::Hour}},
		{6 * Hour, {0, 6, XAxis::LabelFormat::Hour}},
		{12 * Hour, {0, 6, XAxis::LabelFormat::Hour}},
		{1 * Day, {0, 1, XAxis::LabelFormat::Day}},
		{2 * Day, {0, 2, XAxis::LabelFormat::Day}},
		{5 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{10 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{20 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{1 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{3 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{6 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{1 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{2 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{5 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{10 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{20 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{50 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{100 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{200 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{500 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{1000 * Year, {0, 1, XAxis::LabelFormat::Year}},
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

void Graph::makeYAxis(int channel)
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
	double interval = d1 - d2;
	if(qFuzzyIsNull(interval)) {
		shvError() << "channel:" << channel << "Y axis interval == 0";
		return;
	}
	shvDebug() << channel << "tick interval:" << interval << "range interval:" << range.interval();
	double pow = 1;
	if( interval >= 1 ) {
		while(interval >= 7) {
			interval /= 10;
			pow *= 10;
		}
	}
	else {
		while(interval > 0 && interval < 1) {
			interval *= 10;
			pow /= 10;
		}
	}

	GraphChannel::YAxis &axis = ch->m_state.axis;
	// snap to closest 1, 2, 5
	if(interval < 1.5) {
		axis.tickInterval = 1 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 3) {
		axis.tickInterval = 2 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 7) {
		axis.tickInterval = 5 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 10) {
		axis.tickInterval = 5 * pow * 10;
		axis.subtickEvery = tick_units;
	}
	else
		shvWarning() << "snapping interval error, interval:" << interval;
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

QRect Graph::southFloatingBarRect() const
{
	QRect ret = m_layout.miniMapRect.united(m_layout.xAxisRect);
	ret.setX(0);
	return ret;
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

void Graph::makeLayout(const QRect &pref_rect)
{
	shvDebug() << __PRETTY_FUNCTION__;
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
	int h_rest = pref_rect.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(m_style.topMargin());
	h_rest -= u2px(m_style.bottomMargin());
	if(h_rest > 0) {
		// distribute free widget height space to channel's rects heights
		std::sort(rests.begin(), rests.end(), [](const Rest &a, const Rest &b) {
			return a.rest < b.rest;
		});
		for (int i = 0; i < rests.count(); ++i) {
			int fair_rest = h_rest / (rests.count() - i);
			const Rest &r = rests[i];
			GraphChannel *ch = channelAt(r.index);
			int h = u2px(ch->m_effectiveStyle.heightRange());
			if(h > fair_rest)
				h = fair_rest;
			ch->m_layout.graphAreaRect.setHeight(ch->m_layout.graphAreaRect.height() + h);
			h_rest -= h;
		}
	}
	// shift channel rects
	int widget_height = 0;
	widget_height += u2px(m_style.topMargin());
	for (int i = visible_channels.count() - 1; i >= 0; --i) {
		GraphChannel *ch = channelAt(visible_channels[i]);

		ch->m_layout.graphAreaRect.moveTop(widget_height);
		//ch->layout.graphRect.setWidth(layout.navigationBarRect.width());

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

	makeXAxis();
	for (int i = visible_channels.count() - 1; i >= 0; --i)
		makeYAxis(i);
}

void Graph::drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	painter->save();
	if(background.isValid())
		painter->fillRect(rect, background);
	QPen pen;
	pen.setColor(color);
	painter->setPen(pen);
	painter->drawRect(rect);
	painter->setFont(font);
	QFontMetrics fm(font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	painter->drawText(rect.left() + fm.horizontalAdvance('i'), rect.top() + fm.leading() + fm.ascent(), text);
#else
	painter->drawText(rect.left() + fm.width('i'), rect.top() + fm.leading() + fm.ascent(), text);
#endif
	painter->restore();
}

void Graph::drawCenteredRectText(QPainter *painter, const QPoint &top_center, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	painter->save();
	QFontMetrics fm(font);
	QRect br = fm.boundingRect(text);
	int inset = u2px(0.2)*2;
	br.adjust(-inset, 0, inset, 0);
	br.moveCenter(top_center);
	br.moveTop(top_center.y());
	if(background.isValid())
		painter->fillRect(br, background);
	QPen pen;
	pen.setColor(color);
	painter->setPen(pen);
	painter->drawRect(br);
	painter->setFont(font);
	br.adjust(inset/2, 0, 0, 0);
	painter->drawText(br, text);
	painter->restore();
}

QVector<int> Graph::visibleChannels()
{
	QVector<int> visible_channels;
	int maximized_channel = maximizedChannelIndex();

	if (maximized_channel >= 0) {
		visible_channels << maximized_channel;
	}
	else {
		for (int i = 0; i < m_channels.count(); ++i) {
			QString shv_path = model()->channelInfo(m_channels[i]->modelIndex()).shvPath;
			if(m_channelFilter.isPathMatch(shv_path)) {
				visible_channels << i;
			}
		}
	}

	return visible_channels;
}

int Graph::maximizedChannelIndex()
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
	for (int i : visibleChannels()) {
		const GraphChannel *ch = channelAt(i);
		if(dirty_rect.intersects(ch->graphAreaRect())) {
			drawBackground(painter, i);
			drawGrid(painter, i);
			drawSamples(painter, i);
			drawProbes(painter, i);
			drawCrossHair(painter, i);
			drawCurrentTime(painter, i);
		}
		if(dirty_rect.intersects(ch->verticalHeaderRect()))
			drawVerticalHeader(painter, i);
		if(dirty_rect.intersects(ch->yAxisRect()))
			drawYAxis(painter, i);
	}
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
			//ch_st.setLineAreaStyle(ChannelStyle::LineAreaStyle::Filled);
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
	bc.setAlphaF(0.8);
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

void Graph::drawVerticalHeader(QPainter *painter, int channel)
{
	int header_inset = u2px(m_style.headerInset());
	GraphChannel *ch = channelAt(channel);
	QColor c = m_style.color();
	QColor bc = m_style.colorPanel();
	//bc.setAlphaF(0.05);
	QPen pen;
	pen.setColor(c);
	painter->setPen(pen);

	const GraphModel::ChannelInfo& chi = model()->channelInfo(ch->modelIndex());
	QString name = chi.name;
	QString shv_path = chi.shvPath;
	if(name.isEmpty())
		name = shv_path;
	if(name.isEmpty())
		name = tr("<no name>");
	//shvInfo() << channel << shv_path << name;
	QFont font = m_style.font();
	//drawRectText(painter, ch->m_layout.verticalHeaderRect, name, font, c, bc);
	painter->save();
	painter->fillRect(ch->m_layout.verticalHeaderRect, bc);
	//painter->drawRect(ch->m_layout.verticalHeaderRect);

	painter->setClipRect(ch->m_layout.verticalHeaderRect);
	{
		QRect r = ch->m_layout.verticalHeaderRect;
		r.setWidth(header_inset);
		painter->fillRect(r, ch->m_effectiveStyle.color());
	}

	font.setBold(true);
	QFontMetrics fm(font);
	painter->setFont(font);
	QRect text_rect = ch->m_layout.verticalHeaderRect.adjusted(2*header_inset, header_inset, -header_inset, -header_inset);
	painter->drawText(text_rect, name);

	//font.setBold(false);
	//painter->setFont(font);
	//text_rect.moveTop(text_rect.top() + fm.lineSpacing());
	//painter->drawText(text_rect, shv_path);

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
	//pen.setWidth(u2px(0.1));
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
		//drawRectText(painter, m_layout.xAxisRect, "x-axis", m_effectiveStyle.font(), Qt::green);
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
			dt = dt.toTimeZone(m_timeZone);
			return dt;
		};
		QString text;
		switch (axis.labelFormat) {
		case XAxis::LabelFormat::MSec:
			text = QStringLiteral("%1.%2").arg((t / 1000) % 1000).arg(t % 1000, 3, 10, QChar('0'));
			break;
		case XAxis::LabelFormat::Sec:
			text = QString::number((t / 1000) % 1000);
			break;
		case XAxis::LabelFormat::Min:
		case XAxis::LabelFormat::Hour: {
			QTime tm = date_time_tz(t).time();
			text = QStringLiteral("%1:%2").arg(tm.hour()).arg(tm.minute(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Day: {
			QDate dt = date_time_tz(t).date();
			text = QStringLiteral("%1/%2").arg(dt.month()).arg(dt.day(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Month: {
			QDate dt = date_time_tz(t).date();
			text = QStringLiteral("%1-%2").arg(dt.year()).arg(dt.month(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Year: {
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
		//shvInfo() << text;
	}
	{
		QString text;
		switch (axis.labelFormat) {
		case XAxis::LabelFormat::MSec:
		case XAxis::LabelFormat::Sec:
			text = QStringLiteral("sec");
			break;
		case XAxis::LabelFormat::Min:
		case XAxis::LabelFormat::Hour: {
			text = QStringLiteral("hour:min");
			break;
		}
		case XAxis::LabelFormat::Day: {
			text = QStringLiteral("month/day");
			break;
		}
		case XAxis::LabelFormat::Month: {
			text = QStringLiteral("year-month");
			break;
		}
		case XAxis::LabelFormat::Year: {
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
			QColor color = m_style.colorCurrentTime();
			drawXAxisTimeMark(painter, current_time, color);
		}
	}
	if(crossHairPos().isValid()) {
		timemsec_t time = posToTime(crossHairPos().possition.x());
		QColor color = m_style.colorCrossBar();
		drawXAxisTimeMark(painter, time, color);
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

std::function<QPoint (const Sample &s, int meta_type_id)> Graph::dataToPointFn(const DataRect &src, const QRect &dest)
{
	int le = dest.left();
	int ri = dest.right();
	int to = dest.top();
	int bo = dest.bottom();

	timemsec_t t1 = src.xRange.min;
	timemsec_t t2 = src.xRange.max;
	double d1 = src.yRange.min;
	double d2 = src.yRange.max;

	if(t2 - t1 == 0)
		return nullptr;
	double kx = static_cast<double>(ri - le) / (t2 - t1);
	//shvInfo() << "t1:" << t1 << "t2:" << t2 << "le:" << le << "ri:" << ri << "kx:" << kx;
	if(std::abs(d2 - d1) < 1e-6)
		return nullptr;
	double ky = (to - bo) / (d2 - d1);

	return  [le, bo, kx, t1, d1, ky](const Sample &s, int meta_type_id) -> QPoint {
		if(!s.isValid())
			return QPoint();
		const timemsec_t t = s.time;
		bool ok;
		double d = GraphModel::valueToDouble(s.value, meta_type_id, &ok);
		if(!ok)
			return QPoint();
		double x = le + (t - t1) * kx;
		double y = bo + (d - d1) * ky;
		return QPoint{static_cast<int>(x), static_cast<int>(y)};
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
		timemsec_t t = static_cast<timemsec_t>(t1 + (x - le) * kx);
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
		return static_cast<timemsec_t>(t1 + static_cast<double>(x - le) * (t2 - t1) / (ri - le));
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
		return static_cast<timemsec_t>(le + static_cast<double>(t - t1) * (ri - le) / (t2 - t1));
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
		return static_cast<int>(y1 + static_cast<double>(val - d1) * (y2 - y1) / (d2 - d1));
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
				//if(ev->isAccepted()) cannot accept mouse-move, since it invalidates painting regions
				//	return true;
			}
		}
	}
}

QString Graph::rectToString(const QRect &r)
{
	QString s = "%1,%2 %3x%4";
	return s.arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

void Graph::drawSamples(QPainter *painter, int channel_ix, const DataRect &src_rect, const QRect &dest_rect, const GraphChannel::Style &channel_style)
{
	//shvLogFuncFrame() << "channel:" << channel_ix;
	const GraphChannel *ch = channelAt(channel_ix);
	int model_ix = ch->modelIndex();
	QRect rect = dest_rect.isEmpty()? ch->graphDataGridRect(): dest_rect;
	GraphChannel::Style ch_style = channel_style.isEmpty()? ch->m_effectiveStyle: channel_style;

	XRange xrange;
	YRange yrange;
	if(!src_rect.isValid()) {
		xrange = xRangeZoom();
		yrange = ch->yRangeZoom();
	}
	else {
		xrange = src_rect.xRange;
		yrange = src_rect.yRange;
	}
	auto sample2point = dataToPointFn(DataRect{xrange, yrange}, rect);

	if(!sample2point)
		return;

	int interpolation = ch_style.interpolation();

	QPen pen;
	QColor line_color = ch_style.color();
	pen.setColor(line_color);
	{
		double d = ch_style.lineWidth();
		pen.setWidthF(u2pxf(d));
	}
	pen.setCapStyle(Qt::FlatCap);
	QPen steps_join_pen = pen;
	steps_join_pen.setWidthF(pen.widthF() / 4);
	{
		auto c = pen.color();
		c.setAlphaF(0.3);
		steps_join_pen.setColor(c);
	}
	painter->save();
	QRect clip_rect = rect.adjusted(0, -pen.width(), 0, pen.width());
	painter->setClipRect(clip_rect);
	painter->setPen(pen);
	QColor line_area_color;
	if(ch_style.lineAreaStyle() == GraphChannel::Style::LineAreaStyle::Filled) {
		line_area_color = line_color;
		line_area_color.setAlphaF(0.4);
		//line_area_color.setHsv(line_area_color.hslHue(), line_area_color.hsvSaturation() / 2, line_area_color.lightness());
	}

	int channel_meta_type_id = channelMetaTypeId(channel_ix);
	GraphModel *graph_model = model();
	int ix1 = graph_model->lessOrEqualIndex(model_ix, xrange.min);
	//ix1--; // draw one more sample to correctly display connection line to the first one in the zoom window
	if(ix1 < 0)
		ix1 = 0;
	int ix2 = graph_model->lessOrEqualIndex(model_ix, xrange.max) + 1;
	ix2++; // draw one more sample to correctly display (n-1)th one
	//const GraphModel::ChannelInfo &chinfo = graph_model->channelInfo(channel_ix);
	int samples_cnt = graph_model->count(model_ix);
	shvDebug() << graph_model->channelShvPath(channel_ix) << "range:" << xrange.min << xrange.max;
	shvDebug() << "\t" << channel_ix
			   << "from:" << ix1 << "to:" << ix2 << "cnt:" << (ix2 - ix1) << "of:" << samples_cnt;
	constexpr int NO_X = std::numeric_limits<int>::min();
	struct OnePixelPoints {
		int x = NO_X;
		int lastY = 0;
		int minY = std::numeric_limits<int>::max();
		int maxY = std::numeric_limits<int>::min();
	};
	OnePixelPoints current_px, recent_px;
	for (int i = ix1; i <= ix2 && i <= samples_cnt; ++i) {
		// sample is drawn one step behind, so one more loop is needed
		const Sample s = (i < samples_cnt)? graph_model->sampleAt(model_ix, i): Sample();
		if (interpolation == GraphChannel::Style::Interpolation::None) {
			QPoint sample_point = sample2point(Sample{s.time, 0}, channel_meta_type_id); // <- this can be computed ahead
			int arrow_width = u2px(1);
			painter->drawLine(sample_point.x(), clip_rect.y() + clip_rect.height() / 2, sample_point.x(), clip_rect.y() + clip_rect.height());
			QPainterPath path;
			path.moveTo(sample_point.x() - arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
			path.lineTo(sample_point.x() + arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
			path.lineTo(sample_point.x(), clip_rect.y() + clip_rect.height());
			path.lineTo(sample_point.x() - arrow_width / 2, clip_rect.y() + clip_rect.height() - arrow_width / 2);
			path.closeSubpath();
			painter->fillPath(path, painter->pen().color());
			continue;
		}

		const QPoint sample_point = sample2point(s, channel_meta_type_id);
		//shvDebug() << i << "t:" << s.time << "x:" << sample_point.x() << "y:" << sample_point.y();
		//shvDebug() << "\t recent x:" << recent_px.x << " current x:" << current_px.x;
		if(sample_point.x() == current_px.x) {
			current_px.minY = qMin(current_px.minY, sample_point.y());
			current_px.maxY = qMax(current_px.maxY, sample_point.y());
		}
		else {
			if(current_px.x != NO_X) {
				// draw recent point
				QPoint drawn_point(current_px.x, current_px.lastY);
				if(current_px.maxY != current_px.minY) {
					drawn_point = QPoint{current_px.x, (current_px.minY + current_px.maxY) / 2};
					painter->drawLine(current_px.x, current_px.minY, current_px.x, current_px.maxY);
				}
				if(interpolation == GraphChannel::Style::Interpolation::Stepped) {
					if(recent_px.x != NO_X) {
						QPoint pa{recent_px.x, recent_px.lastY};
						if(line_area_color.isValid()) {
							QPoint p0 = sample2point(Sample{s.time, 0}, channel_meta_type_id); // <- this can be computed ahead
							p0.setX(drawn_point.x());
							painter->fillRect(QRect{pa + QPoint{1, 0}, p0}, line_area_color);
						}
						QPoint pb{drawn_point.x(), recent_px.lastY};
						// draw vertical line lighter
						painter->setPen(steps_join_pen);
						painter->drawLine(pb, drawn_point);
						// draw horizontal line
						painter->setPen(pen);
						painter->drawLine(pa, pb);
					}
				}
				else {
					if(recent_px.x != NO_X) {
						QPoint pa{recent_px.x, recent_px.lastY};
						if(line_area_color.isValid()) {
							QPoint p0 = sample2point(Sample{s.time, 0}, channel_meta_type_id);
							p0.setX(drawn_point.x());
							QPainterPath pp;
							pp.moveTo(pa);
							pp.lineTo(drawn_point);
							pp.lineTo(p0);
							pp.lineTo(pa.x(), p0.y());
							pp.closeSubpath();
							painter->fillPath(pp, line_area_color);
						}
						painter->drawLine(pa, drawn_point);
					}
				}
				recent_px = current_px;
			}
			current_px.x = sample_point.x();
			current_px.minY = current_px.maxY = sample_point.y();
		}
		current_px.lastY = sample_point.y();
	}
	painter->restore();
}

void Graph::drawCrossHair(QPainter *painter, int channel_ix)
{
	if(!crossHairPos().isValid())
		return;
	auto crossbar_pos = crossHairPos().possition;
	//if(channel_ix != crossBarPos().channelIndex)
	//	return;
	const GraphChannel *ch = channelAt(channel_ix);
	if(ch->graphDataGridRect().left() >= crossbar_pos.x() || ch->graphDataGridRect().right() <= crossbar_pos.x()) {
		return;
	}
	shvDebug() << "drawCrossHair:" << ch->shvPath();
	painter->save();
	QColor color = m_style.colorCrossBar();
	if(channel_ix == crossHairPos().channelIndex) {
		/// draw point on current value graph
		timemsec_t time = posToTime(crossbar_pos.x());
		Sample s = timeToSample(channel_ix, time);
		if(s.value.isValid()) {
			QPoint p = dataToPos(channel_ix, s);
			{
				QDateTime dt = QDateTime::fromMSecsSinceEpoch(time);
				dt = dt.toTimeZone(m_timeZone);
				shvDebug() << "sample point" << dt.toString(Qt::ISODateWithMs) << m_timeZone.id()
						   << s.value.toString() << "--->" << p.x() << p.y();
				GraphModel *m = model();
				const GraphChannel *ch = channelAt(channel_ix);
				int model_ix = ch->modelIndex();
				shvDebug() << "samples cnt:" << m->count(model_ix);
			}
			int d = u2px(0.3);
			QRect rect(0, 0, d, d);
			rect.moveCenter(p);
			//painter->fillRect(rect, c->effectiveStyle.color());
			painter->fillRect(rect, color);
			rect.adjust(2, 2, -2, -2);
			painter->fillRect(rect, ch->m_effectiveStyle.colorBackground());
		}
	}
	int d = u2px(0.4);
	QRect bull_eye_rect;
	if(ch->graphDataGridRect().top() < crossbar_pos.y() && ch->graphDataGridRect().bottom() > crossbar_pos.y()) {
		bull_eye_rect = QRect(0, 0, d, d);
		bull_eye_rect.moveCenter(crossbar_pos);
	}
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
	if(!bull_eye_rect.isNull()) {
		//painter->setClipRect(c->dataAreaRect());
		/// draw horizontal line
		QPoint p1{ch->graphDataGridRect().left(), crossbar_pos.y()};
		QPoint p2{ch->graphDataGridRect().right(), crossbar_pos.y()};
		//painter->drawLine(p1, p2);
		//painter->drawLine(p3, p4);
		painter->drawLine(p1, p2);

		/// draw point
		painter->setPen(pen_solid);
		painter->fillRect(bull_eye_rect, ch->m_effectiveStyle.colorBackground());
		painter->drawRect(bull_eye_rect);

		enum {DEBUG = 0};
		if(DEBUG) {
			//timemsec_t t = posToTime(crossbar_pos.x());
			Sample s = posToData(crossbar_pos);
			cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromMSecsSinceEpoch(s.time);
			QString str = QStringLiteral("%1, %2").arg(QString::fromStdString(dt.toIsoString())).arg(s.value.toDouble());
			shvDebug() << "bull eye:" << str;
			//painter->setClipRect(m_layout.rect);
			//painter->drawText(crossbar_pos + QPoint{focus_rect.width(), -focus_rect.height()}, str);
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
	c.setAlphaF(0.3);
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

void Graph::drawXAxisTimeMark(QPainter *painter, time_t time, const QColor &color)
{
	if(time <= 0)
		return;
	int x = timeToPos(time);
	QPoint p1{x, m_layout.xAxisRect.top()};
	int tick_len = u2px(m_state.xAxis.tickLen)*2;
	{
		QRect r{0, 0, 2 * tick_len, tick_len};
		r.moveCenter(p1);
		r.moveTop(p1.y());
		//painter->fillRect(r, Qt::green);
		QPainterPath pp;
		pp.moveTo(r.bottomLeft());
		pp.lineTo(r.bottomRight());
		pp.lineTo(r.center().x(), r.top());
		pp.lineTo(r.bottomLeft());
		painter->fillPath(pp, color);
	}
	QDateTime dt = QDateTime::fromMSecsSinceEpoch(time);
	if(m_timeZone.isValid())
		dt = dt.toTimeZone(m_timeZone);
	QString text = dt.toString(Qt::ISODate);
	//shvInfo() << text;
	p1.setY(p1.y() + tick_len);
	drawCenteredRectText(painter, p1, text, m_style.font(), color.darker(400), color);
}

void Graph::onButtonBoxClicked(int button_id)
{
	shvLogFuncFrame();
	if(button_id == (int)GraphButtonBox::ButtonId::Menu) {
		QPoint pos = m_cornerCellButtonBox->buttonRect((GraphButtonBox::ButtonId)button_id).center();
		emit graphContextMenuRequest(pos);
	}
}

}}}
