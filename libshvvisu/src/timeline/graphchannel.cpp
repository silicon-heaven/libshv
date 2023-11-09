#include <shv/visu/timeline/graphchannel.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>

#include <shv/coreqt/log.h>

namespace shv::visu::timeline {

//==========================================
// Graph::Channel
//==========================================
GraphChannel::Style::Style() = default;

GraphChannel::Style::Style(const QVariantMap &o)
	: QVariantMap(o)
{
}

GraphChannel::GraphChannel(Graph *graph)
	: QObject(graph)
	, m_buttonBox(new GraphButtonBox({GraphButtonBox::ButtonId::Hide, GraphButtonBox::ButtonId::Menu}, this))
{
	static int n = 0;
	m_buttonBox->setObjectName(QString("channelButtonBox_%1").arg(++n));
	connect(m_buttonBox, &GraphButtonBox::buttonClicked, this, &GraphChannel::onButtonBoxClicked);
}

qsizetype GraphChannel::modelIndex() const
{
	return m_modelIndex;
}

void GraphChannel::setModelIndex(qsizetype ix)
{
	m_modelIndex = ix;
}

QString GraphChannel::shvPath() const
{
	if(graph() && graph()->model()) {
		return graph()->model()->channelShvPath(m_modelIndex);
	}
	return QString();
}

YRange GraphChannel::yRange() const
{
	return m_state.yRange;
}

YRange GraphChannel::yRangeZoom() const
{
	return m_state.yRangeZoom;
}

const GraphChannel::Style& GraphChannel::style() const
{
	return m_style;
}

void GraphChannel::setStyle(const Style& st)
{
	m_style = st;
}

GraphChannel::Style GraphChannel::effectiveStyle() const
{
	return m_effectiveStyle;
}

const QRect& GraphChannel::graphAreaRect() const
{
	return  m_layout.graphAreaRect;
}

const QRect& GraphChannel::graphDataGridRect() const
{
	return  m_layout.graphDataGridRect;
}

const QRect& GraphChannel::verticalHeaderRect() const
{
	return  m_layout.verticalHeaderRect;
}

const QRect& GraphChannel::yAxisRect() const
{
	return  m_layout.yAxisRect;
}

int GraphChannel::valueToPos(double val) const
{
	auto val2pos = Graph::valueToPosFn(yRangeZoom(), Graph::WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()});
	return val2pos? val2pos(val): 0;
}

double GraphChannel::posToValue(int y) const
{
	auto pos2val = Graph::posToValueFn(Graph::WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()}, yRangeZoom());
	return pos2val? pos2val(y): 0;
}

const GraphButtonBox *GraphChannel::buttonBox() const
{
	return m_buttonBox;
}

GraphButtonBox *GraphChannel::buttonBox()
{
	return m_buttonBox;
}

bool GraphChannel::isMaximized() const
{
	return m_state.isMaximized;
}

void GraphChannel::setMaximized(bool b)
{
	m_state.isMaximized = b;
}

Graph *GraphChannel::graph() const
{
	return qobject_cast<Graph*>(parent());
}

void GraphChannel::onButtonBoxClicked(int button_id)
{
	shvLogFuncFrame();
	if(button_id == static_cast<int>(GraphButtonBox::ButtonId::Menu)) {
		QPoint pos = buttonBox()->buttonRect(static_cast<GraphButtonBox::ButtonId>(button_id)).center();
		graph()->emitChannelContextMenuRequest(graphChannelIndex(), pos);
	}
	else if(button_id == static_cast<int>(GraphButtonBox::ButtonId::Hide)) {
		graph()->setChannelVisible(graphChannelIndex(), false);
	}
}

int GraphChannel::graphChannelIndex() const
{
	auto *g = graph();
	for (int i = 0; i < g->channelCount(); ++i) {
		if(g->channelAt(i) == this) {
			return i;
		}
	}
	return -1;
}

} // namespace shv
