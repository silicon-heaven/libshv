#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QObject>

namespace shv::visu::timeline {

class Graph;

using timemsec_t = int64_t;

class SHVVISU_DECL_EXPORT ChannelProbe: public QObject
{
	Q_OBJECT

public:
	ChannelProbe(Graph *graph, qsizetype channel_ix, timemsec_t time);

	QColor color() const;
	QString shvPath() const;
	QString localizedShvPath() const;
	qsizetype channelIndex() const;
	bool isLocalizeShvPath() const;

	void setCurrentTime(timemsec_t time);
	timemsec_t currentTime() const;
	QString currentTimeIsoFormat() const;

	void nextSample();
	void prevSample();
	QVariantMap sampleValues() const;

	Q_SIGNAL void currentTimeChanged(timemsec_t time);

protected:
	const Graph *m_graph = nullptr;
private:
	timemsec_t m_currentTime;
	qsizetype m_channelIndex;
};

}
