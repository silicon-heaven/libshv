#include "channelprobe.h"
#include "graph.h"
#include "graphmodel.h"

namespace shv {
namespace visu {
namespace timeline {

ChannelProbe::ChannelProbe(Graph *graph, int channel_ix, timemsec_t time)
	: QObject(graph)
{
	m_graph = qobject_cast<Graph*>(parent());
	m_channelIndex = channel_ix;
	m_currentTime = time;
}

QColor ChannelProbe::color()
{
	return m_graph->channelAt(m_channelIndex)->style().color();
}

timemsec_t ChannelProbe::currentTime()
{
	return m_currentTime;
}

void ChannelProbe::nextValue()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	int model_ix = ch->modelIndex();
	int ix = m->lessOrEqualIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix + 1);

		if (s.isValid()) {
			m_currentTime = s.time;
			emit currentTimeChanged();
		}
	}
}

void ChannelProbe::prevValue()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	int model_ix = ch->modelIndex();
	int ix = m->lessOrEqualIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix - 1);

		if (s.isValid()) {
			m_currentTime = s.time;
			emit currentTimeChanged();
		}
	}
}

QString ChannelProbe::yValues()
{
	Sample s = m_graph->nearestSample(m_channelIndex, m_currentTime);
	return m_graph->sampleToString(m_channelIndex, s);
}

QString ChannelProbe::shvPath()
{
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	return m_graph->model()->channelInfo(ch->modelIndex()).shvPath;
}

}
}
}
