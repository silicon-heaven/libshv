#pragma once

#include "../shvvisuglobal.h"

#include <QObject>

namespace shv {
namespace visu {
namespace timeline {

class Graph;

using timemsec_t = int64_t;

class SHVVISU_DECL_EXPORT ChannelProbe: public QObject
{
	Q_OBJECT

public:
	ChannelProbe(Graph *graph, int channel_ix, timemsec_t time);

	QColor color() const;
	QString shvPath() const;
	int channelIndex() const;

	void setCurrentTime(timemsec_t time);
	timemsec_t currentTime() const;
	QString currentTimeIsoFormat() const;

	void nextSample();
	void prevSample();
	QMap<QString, QString> yValues() const;

	Q_SIGNAL void currentTimeChanged(timemsec_t time);

protected:
	const Graph *m_graph = nullptr;
private:
	timemsec_t m_currentTime;
	int m_channelIndex;
};

}
}
}
