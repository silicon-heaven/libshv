#include <shv/visu/timeline/channelprobe.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>

#include <QDateTime>

namespace shv::visu::timeline {

ChannelProbe::ChannelProbe(Graph *graph, qsizetype channel_ix, timemsec_t time)
	: QObject(graph)
{
	m_graph = graph;
	m_channelIndex = channel_ix;
	m_currentTime = time;
}

QColor ChannelProbe::color() const
{
	return m_graph->channelAt(m_channelIndex)->style().color();
}

void ChannelProbe::setCurrentTime(timemsec_t time)
{
	m_currentTime = time;
	emit currentTimeChanged(time);
}

timemsec_t ChannelProbe::currentTime() const
{
	return m_currentTime;
}

QString ChannelProbe::currentTimeIsoFormat() const
{
	QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_currentTime);
#if SHVVISU_HAS_TIMEZONE
	dt = dt.toTimeZone(m_graph->timeZone());
#endif
	return dt.toString(Qt::ISODateWithMs);
}

void ChannelProbe::nextSample()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	auto model_ix = ch->modelIndex();
	auto ix = m->greaterTimeIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix);

		if (s.isValid()) {
			setCurrentTime(s.time);
		}
	}
}

void ChannelProbe::prevSample()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	auto model_ix = ch->modelIndex();
	auto ix = m->lessTimeIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix);

		if (s.isValid()) {
			setCurrentTime(s.time);
		}
	}
}

QVariantMap ChannelProbe::sampleValues() const
{
	Sample s = m_graph->timeToSample(m_channelIndex, m_currentTime);
	return m_graph->sampleValues(m_channelIndex, s);
}

QString ChannelProbe::shvPath() const
{
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	return m_graph->model()->channelInfo(ch->modelIndex()).shvPath;
}

qsizetype ChannelProbe::channelIndex() const
{
	return m_channelIndex;
}

}
