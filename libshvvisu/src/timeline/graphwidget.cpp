
#include <shv/visu/timeline/graphview.h>
#include <shv/visu/timeline/graphwidget.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/channelprobewidget.h>


#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QApplication>
#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QDateTime>
#include <QMenu>
#include <QScreen>
#include <QScrollBar>
#include <QWindow>
#include <QDrag>
#include <QMimeData>
#include <QMessageBox>
#include <QToolTip>

#if QT_VERSION_MAJOR < 6
#include <QDesktopWidget>
#endif

#define logMouseSelection() nCDebug("MouseSelection")

namespace shv::visu::timeline {

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
}

void GraphWidget::setGraph(Graph *g)
{
	if(m_graph)
		m_graph->disconnect(this);
	m_graph = g;
	Graph::Style style = graph()->style();
	style.init(this);
	graph()->setStyle(style);
	update();
	connect(m_graph, &Graph::presentationDirty, this, [this](const QRect &rect) {
		if(rect.isEmpty())
			update();
		else
			update(rect);
	});
	connect(m_graph, &Graph::layoutChanged, this, [this]() {
		makeLayout();
	});

	connect(m_graph, &Graph::styleChanged, this, [this]() {
		update();
	});

	connect(m_graph, &Graph::graphContextMenuRequest, this, &GraphWidget::showGraphContextMenu);
	connect(m_graph, &Graph::channelContextMenuRequest, this, &GraphWidget::showChannelContextMenu);
}

Graph *GraphWidget::graph()
{
	return m_graph;
}

const Graph *GraphWidget::graph() const
{
	return m_graph;
}

#if SHVVISU_HAS_TIMEZONE
void GraphWidget::setTimeZone(const QTimeZone &tz)
{
	graph()->setTimeZone(tz);
}
#endif

void GraphWidget::makeLayout(const QSize &preferred_size)
{
	shvLogFuncFrame();
	m_graphPreferredSize = preferred_size;
	graph()->makeLayout(QRect(QPoint(), preferred_size));
	QSize sz = graph()->rect().size();
	shvDebug() << "preferred size:" << preferred_size.width() << 'x' << preferred_size.height();
	shvDebug() << "new size:" << sz.width() << 'x' << sz.height();
	if(sz.width() > 0 && sz.height() > 0) {
		setMinimumSize(sz);
		setMaximumSize(sz);
	}
	update();
}

void GraphWidget::makeLayout()
{
	makeLayout(m_graphPreferredSize);
}

bool GraphWidget::event(QEvent *ev)
{
	if(Graph *gr = graph()) {
		if (ev->type() == QEvent::ToolTip) {
			auto *help_event = static_cast<QHelpEvent *>(ev);
			auto channel_ix = gr->posToChannelHeader(help_event->pos());

			if (channel_ix > -1) {
				GraphModel::ChannelInfo channel_info = gr->channelInfo(channel_ix);
				QString tooltip;

				if (channel_info.name.isEmpty()) {
					tooltip = QString("<p style='white-space:pre'><b>%1</b> %2</p>").arg(tr("Path:"), channel_info.shvPath);
				}
				else {
					tooltip = QString("<p style='white-space:pre'><b>%1</b> %2 <br><b>%3</b> %4</p>")
							.arg(tr("Name:"), channel_info.name, tr("Path:"), channel_info.shvPath);
				}

				QToolTip::showText(help_event->globalPos(), tooltip, this, {}, 2000);
				help_event->accept();
				return true;
			}
			QToolTip::hideText();
			help_event->ignore();
		}
	}
	return Super::event(ev);
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	auto *view_port = parentWidget();
	auto *scroll_area = qobject_cast<GraphView*>(view_port->parentWidget());
	if(!scroll_area) {
		shvError() << "Cannot find GraphView";
		return;
	}
	QPainter painter(this);
	const QRect dirty_rect = event->rect();
	graph()->draw(&painter, dirty_rect,  scroll_area->widgetViewRect());
}

bool GraphWidget::isMouseAboveMiniMap(const QPoint &mouse_pos) const
{
	const Graph *gr = graph();
	return gr->miniMapRect().contains(mouse_pos);
}

bool GraphWidget::isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const
{
	const Graph *gr = graph();
	if(mouse_pos.y() < gr->miniMapRect().top() || mouse_pos.y() > gr->miniMapRect().bottom())
		return false;
	int x = left
			? gr->miniMapTimeToPos(gr->xRangeZoom().min)
			: gr->miniMapTimeToPos(gr->xRangeZoom().max);
	int w = gr->u2px(0.5);
	int x1 = left ? x - w : x - w/2;
	int x2 = left ? x + w/2 : x + w;
	return mouse_pos.x() > x1 && mouse_pos.x() < x2;
}

bool GraphWidget::isMouseAboveLeftMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, true);
}

bool GraphWidget::isMouseAboveRightMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, false);
}

bool GraphWidget::isMouseAboveMiniMapSlider(const QPoint &pos) const
{
	const Graph *gr = graph();
	if(pos.y() < gr->miniMapRect().top() || pos.y() > gr->miniMapRect().bottom())
		return false;
	int x1 = gr->miniMapTimeToPos(gr->xRangeZoom().min);
	int x2 = gr->miniMapTimeToPos(gr->xRangeZoom().max);
	return (x1 < pos.x()) && (pos.x() < x2);
}

int GraphWidget::posToChannelVerticalHeader(const QPoint &pos) const
{
	const Graph *gr = graph();
	for (int i = 0; i < gr->channelCount(); ++i) {
		const GraphChannel *ch = gr->channelAt(i);
		if(ch->verticalHeaderRect().contains(pos)) {
			return i;
		}
	}
	return -1;
}

qsizetype GraphWidget::posToChannel(const QPoint &pos) const
{
	const Graph *gr = graph();
	auto ch_ix = gr->posToChannel(pos);
	return ch_ix;
}

void GraphWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(isMouseAboveMiniMap(pos)) {
		Graph *gr = graph();
		gr->resetXZoom();
		update();
		event->accept();
		return;
	}
	if(posToChannel(pos) >= 0) {
		if(event->modifiers() == Qt::NoModifier) {
			emit graphChannelDoubleClicked(pos);
			event->accept();
			return;
		}
	}
	Super::mouseDoubleClickEvent(event);
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(event->button() == Qt::LeftButton) {
		if(isMouseAboveLeftMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapLeftResize;
			shvDebug() << "LEFT resize";
			event->accept();
			return;
		}
		if(isMouseAboveRightMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapRightResize;
			event->accept();
			return;
		}
		if(isMouseAboveMiniMapSlider(pos)) {
			m_mouseOperation = MouseOperation::MiniMapScrollZoom;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		if (isMouseAboveChannelResizeHandle(pos, Qt::Edge::BottomEdge)) {
			m_mouseOperation = MouseOperation::ChannelHeaderResizeHeight;
			m_resizeChannelIx = graph()->posToChannelHeader(pos);
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		if (isMouseAboveChannelResizeHandle(pos, Qt::Edge::RightEdge)) {
			m_mouseOperation = MouseOperation::GraphVerticalHeaderResizeWidth;
			m_resizeChannelIx = graph()->posToChannelHeader(pos);
			m_recentMousePos = pos;
			event->accept();
			return;
		}

		if(posToChannel(pos) >= 0) {
			if(event->modifiers() == Qt::ControlModifier) {
				m_mouseOperation = MouseOperation::GraphDataAreaLeftCtrlPress;
			}
			else if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
				m_mouseOperation = MouseOperation::GraphDataAreaLeftCtrlShiftPress;
			}
			else {
				logMouseSelection() << "GraphAreaPress";
				m_mouseOperation = MouseOperation::GraphDataAreaLeftPress;
			}
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		if (posToChannelVerticalHeader(pos) > -1) {
			if (event->modifiers() == Qt::NoModifier) {
				m_mouseOperation = MouseOperation::ChannelHeaderMove;
			}
			m_recentMousePos = pos;
			event->accept();
			return;
		}
	}
	else if(event->button() == Qt::RightButton) {
		if(posToChannel(pos) >= 0) {
			if(event->modifiers() == Qt::NoModifier) {
				logMouseSelection() << "GraphAreaRightPress";
				m_mouseOperation = MouseOperation::GraphDataAreaRightPress;
				m_recentMousePos = pos;
				event->accept();
				return;
			}
		}
	}
	Super::mousePressEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
	logMouseSelection() << "mouseReleaseEvent, button:" << event->button() << "op:" << static_cast<int>(m_mouseOperation);
	auto old_mouse_op = m_mouseOperation;
	m_mouseOperation = MouseOperation::None;
	if(event->button() == Qt::LeftButton) {
		if(old_mouse_op == MouseOperation::GraphDataAreaLeftPress) {
			graph()->setSelectionRect(QRect());
			update();
			event->accept();
			return;
		}
		if(old_mouse_op == MouseOperation::GraphDataAreaLeftCtrlPress) {
			if(event->modifiers() == Qt::ControlModifier) {
				auto channel_ix = posToChannel(event->pos());
				if(channel_ix >= 0) {
					removeProbes(channel_ix);
					timemsec_t time = m_graph->posToTime(event->pos().x());
					createProbe(channel_ix, time);
					event->accept();
					return;
				}
			}
		}
		else if(old_mouse_op == MouseOperation::GraphDataAreaLeftCtrlShiftPress) {
			auto channel_ix = posToChannel(event->pos());
			if(channel_ix >= 0) {
				timemsec_t time = m_graph->posToTime(event->pos().x());
				createProbe(channel_ix, time);
				event->accept();
				return;
			}
		}
		else if(old_mouse_op == MouseOperation::GraphAreaSelection) {
			if(event->modifiers() == Qt::NoModifier) {
				showGraphSelectionContextMenu(event->pos());
			}
			else if(event->modifiers() == Qt::ShiftModifier) {
				m_graph->zoomToSelection(false);
			}
			event->accept();
			graph()->setSelectionRect(QRect());
			update();
			return;
		}
		else if((old_mouse_op == MouseOperation::ChannelHeaderResizeHeight) || (old_mouse_op == MouseOperation::GraphVerticalHeaderResizeWidth)) {
			m_resizeChannelIx = -1;
			event->accept();
			update();
			return;
		}
	}
	else if(event->button() == Qt::RightButton) {
		if(old_mouse_op == MouseOperation::GraphDataAreaRightPress) {
			if(event->modifiers() == Qt::NoModifier) {
				auto ch_ix = posToChannel(event->pos());
				if(ch_ix >= 0) {
					showChannelContextMenu(ch_ix, event->pos());
					event->accept();
					update();
					return;
				}
			}
		}
	}
	Super::mouseReleaseEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	Super::mouseMoveEvent(event);
	QPoint pos = event->pos();
	Graph *gr = graph();
	if(isMouseAboveLeftMiniMapHandle(pos) || isMouseAboveRightMiniMapHandle(pos)) {
		setCursor(QCursor(Qt::SplitHCursor));
	}
	else if(isMouseAboveChannelResizeHandle(pos, Qt::Edge::BottomEdge) || (m_mouseOperation == MouseOperation::ChannelHeaderResizeHeight)) {
		setCursor(QCursor(Qt::SizeVerCursor));
	}
	else if(isMouseAboveMiniMapSlider(pos) || isMouseAboveChannelResizeHandle(pos, Qt::Edge::RightEdge) ||
			(m_mouseOperation == MouseOperation::GraphVerticalHeaderResizeWidth)) {
		setCursor(QCursor(Qt::SizeHorCursor));
	}
	else {
		auto channel_ix = graph()->posToChannelHeader(pos);
		if (channel_ix > -1) {
			setCursor(QCursor(Qt::OpenHandCursor));
		}
		else {
			setCursor(QCursor(Qt::ArrowCursor));
		}
	}

	switch (m_mouseOperation) {
	case MouseOperation::None:
		break;
	case MouseOperation::MiniMapLeftResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.min = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapRightResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.max = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapScrollZoom: {
		timemsec_t t2 = gr->miniMapPosToTime(pos.x());
		timemsec_t t1 = gr->miniMapPosToTime(m_recentMousePos.x());
		m_recentMousePos = pos;
		XRange r = gr->xRangeZoom();
		shvDebug() << "r.min:" << r.min << "r.max:" << r.max;
		timemsec_t dt = t2 - t1;
		shvDebug() << dt << "r.min:" << r.min << "-->" << (r.min + dt);
		r.min += dt;
		r.max += dt;
		if(r.min >= gr->xRange().min && r.max <= gr->xRange().max) {
			gr->setXRangeZoom(r);
			r = gr->xRangeZoom();
			shvDebug() << "new r.min:" << r.min << "r.max:" << r.max;
			update();
		}
		return;
	}
	case MouseOperation::GraphDataAreaLeftPress: {
		QPoint point = pos - m_recentMousePos;
		if (point.manhattanLength() > 3) {
			m_mouseOperation = MouseOperation::GraphAreaSelection;
		}
		break;
	}
	case MouseOperation::GraphAreaSelection: {
		gr->setSelectionRect(QRect(m_recentMousePos, pos));
		update();
		break;
	}
	case MouseOperation::GraphAreaMove:
	case MouseOperation::GraphDataAreaLeftCtrlPress: {
		m_mouseOperation = MouseOperation::GraphAreaMove;
		timemsec_t t0 = gr->posToTime(m_recentMousePos.x());
		timemsec_t t1 = gr->posToTime(pos.x());
		timemsec_t dt = t0 - t1;
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max += dt;
		gr->setXRangeZoom(r);
		m_recentMousePos = pos;
		update();
		return;
	}
	case MouseOperation::GraphDataAreaRightPress:
	case MouseOperation::GraphDataAreaLeftCtrlShiftPress:
		return;
	case MouseOperation::ChannelHeaderResizeHeight: {
		gr->resizeChannelHeight(m_resizeChannelIx, pos.y() - m_recentMousePos.y());
		m_recentMousePos = pos;
		return;
	}
	case MouseOperation::GraphVerticalHeaderResizeWidth: {
		gr->resizeVerticalHeaderWidth(pos.x() - m_recentMousePos.x());
		m_recentMousePos = pos;
		return;
	}
	case MouseOperation::ChannelHeaderMove:
		m_mouseOperation = MouseOperation::None;
		if (gr->channelCount() == 0) {
			return;
		}

		QRect header_rect;
		int dragged_channel = -1;
		for (int i = 0; i < gr->channelCount(); ++i) {
			const GraphChannel *ch = gr->channelAt(i);
			if (ch->verticalHeaderRect().contains(pos)) {
				header_rect = ch->verticalHeaderRect();
				dragged_channel = i;
				break;
			}
		}
		if (dragged_channel != -1) {
			auto *drag = new QDrag(this);
			auto *mime = new QMimeData;
			mime->setText(QString());
			drag->setMimeData(mime);
			QPoint p = mapToGlobal(header_rect.topLeft());
#if QT_VERSION_MAJOR < 6
			drag->setPixmap(screen()->grabWindow(QDesktopWidget().winId(), p.x(), p.y(), header_rect.width(), header_rect.height()));
#else
			drag->setPixmap(screen()->grabWindow(0, p.x(), p.y(), header_rect.width(), header_rect.height()));
#endif
			drag->setHotSpot(mapToGlobal(pos) - p);
			setAcceptDrops(true);

			m_channelHeaderMoveContext = new ChannelHeaderMoveContext(this);
			connect(m_channelHeaderMoveContext->mouseMoveScrollTimer, &QTimer::timeout, this, &GraphWidget::scrollToCurrentMousePosOnDrag);
			m_channelHeaderMoveContext->draggedChannel = dragged_channel;
			m_channelHeaderMoveContext->channelDropMarker->resize(header_rect.width(), QFontMetrics(font()).height() / 2);
			m_channelHeaderMoveContext->channelDropMarker->show();
			drag->exec();
			event->accept();
			setAcceptDrops(false);
			delete m_channelHeaderMoveContext;
			m_channelHeaderMoveContext = nullptr;
		}
		return;
	}

	auto ch_ix = posToChannel(pos);
	if(ch_ix >= 0 && !isMouseAboveMiniMap(pos)) {
		setCursor(Qt::BlankCursor);
		gr->setCrossHairPos({ch_ix, pos});
	}
	else {
		hideCrossHair();
	}
}

void GraphWidget::moveDropMarker(const QPoint &mouse_pos)
{
	Q_ASSERT(m_channelHeaderMoveContext);
	Graph *gr = graph();
	int ix = moveChannelTragetIndex(mouse_pos);
	if (ix < gr->channelCount()) {
		QRect ch_rect = gr->channelAt(ix)->verticalHeaderRect();
		m_channelHeaderMoveContext->channelDropMarker->move(ch_rect.left(), ch_rect.top() - m_channelHeaderMoveContext->channelDropMarker->height() / 2);
	}
	else {
		QRect ch_rect = gr->channelAt(ix - 1)->verticalHeaderRect();
		m_channelHeaderMoveContext->channelDropMarker->move(ch_rect.left(), ch_rect.bottom() - m_channelHeaderMoveContext->channelDropMarker->height() / 2);
	}
}

int GraphWidget::moveChannelTragetIndex(const QPoint &mouse_pos) const
{
	const Graph *gr = graph();
	for (int i = 0; i < gr->channelCount(); ++i) {
		const GraphChannel *ch = gr->channelAt(i);
		QRect ch_rect = ch->verticalHeaderRect();
		if (ch_rect.contains(mouse_pos)) {
			if (mouse_pos.y() - ch_rect.top() <= ch_rect.bottom() - mouse_pos.y()) {
				return i;
			}
			int j = i;

			while (true) {
				++j;
				if (j >= gr->channelCount()) {
					return i + 1;
				}
				if (gr->channelAt(j)->verticalHeaderRect().height()) {
					return j;
				}
			}
		}
	}
	if (gr->channelCount() && mouse_pos.y() > gr->channelAt(gr->channelCount() - 1)->verticalHeaderRect().bottom()) {
		return static_cast<int>(gr->channelCount());
	}
	return 0;
}

void GraphWidget::scrollToCurrentMousePosOnDrag()
{
	QPoint mouse_pos = QCursor::pos();
	scrollByMouseOuterOverlap(mouse_pos);
}

bool GraphWidget::scrollByMouseOuterOverlap(const QPoint &mouse_pos)
{
	auto *view_port = parentWidget();
	auto *scroll_area = qobject_cast<GraphView*>(view_port->parentWidget());
	QRect g = view_port->geometry();
	int view_port_top = view_port->mapToGlobal(g.topLeft()).y();
	int view_port_bottom = view_port->mapToGlobal(QPoint(g.left(), g.bottom() - graph()->southFloatingBarRect().height())).y();
	auto *vs = scroll_area->verticalScrollBar();
	if (mouse_pos.y() - view_port_top < 5) {
		int diff = view_port_top - mouse_pos.y();
		if (diff < 5) {
			diff = 5;
		}
		vs->setValue(vs->value() - diff);
		moveDropMarker(mapFromGlobal(mouse_pos));
		return true;
	}
	if (mouse_pos.y() + 5 > view_port_bottom) {
		int diff = mouse_pos.y() - view_port_bottom;
		if (diff < 5) {
			diff = 5;
		}
		vs->setValue(vs->value() + diff);
		moveDropMarker(mapFromGlobal(mouse_pos));
		return true;
	}
	return false;
}

void GraphWidget::leaveEvent(QEvent *)
{
	hideCrossHair();
}

void GraphWidget::hideCrossHair()
{
	Graph *gr = graph();
	if(gr->crossHairPos().isValid()) {
		gr->setCrossHairPos({});
		setCursor(Qt::ArrowCursor);
		update();
	}
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	QPoint pos = event->pos();
#else
	QPoint pos = event->position().toPoint();
#endif
	bool is_zoom_on_slider = isMouseAboveMiniMapSlider(pos);
	bool is_zoom_on_graph = (event->modifiers() == Qt::ControlModifier) && posToChannel(pos) >= 0;
	static constexpr int ZOOM_STEP = 10;
	if(is_zoom_on_slider) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * static_cast<double>(gr->xRangeZoom().interval()) / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max -= dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			update();
		}
		event->accept();
		return;
	}
	if(is_zoom_on_graph) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * static_cast<double>(gr->xRangeZoom().interval()) / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		double ratio = 0.5;
		// shift new zoom to center it horizontally on the mouse position
		timemsec_t t_mouse = gr->posToTime(pos.x());
		ratio = static_cast<double>(t_mouse - r.min) / static_cast<double>(r.interval());
		r.min += static_cast<timemsec_t>(ratio * static_cast<double>(dt));
		r.max -= static_cast<timemsec_t>((1 - ratio) * static_cast<double>(dt));
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			update();
		}
		event->accept();
		return;
	}

	Super::wheelEvent(event);
	// hide cross-hair, because its position is invalid after graph scroll
	// it will be updated on next moude move event
	hideCrossHair();
}

void GraphWidget::contextMenuEvent(QContextMenuEvent *event)
{
	shvLogFuncFrame();
	if(!m_graph)
		return;
	const QPoint pos = event->pos();
	if(m_graph->cornerCellRect().contains(pos)) {
		showGraphContextMenu(pos);
		return;
	}
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		const GraphChannel *ch = m_graph->channelAt(i);
		if(ch->verticalHeaderRect().contains(pos)) {
			showChannelContextMenu(i, pos);
		}
	}
}

void GraphWidget::dragEnterEvent(QDragEnterEvent *event)
{
	event->accept();
}

void GraphWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	Q_UNUSED(event);
}

void GraphWidget::dragMoveEvent(QDragMoveEvent *event)
{
	Q_ASSERT(m_channelHeaderMoveContext);
#if QT_VERSION_MAJOR >= 6
	QPoint pos = event->position().toPoint();
#else
	QPoint pos = event->pos();
#endif

	if (scrollByMouseOuterOverlap(mapToGlobal(pos))) {
		m_channelHeaderMoveContext->mouseMoveScrollTimer->start();
	}
	else {
		m_channelHeaderMoveContext->mouseMoveScrollTimer->stop();
	}

	moveDropMarker(pos);
}

void GraphWidget::dropEvent(QDropEvent *event)
{
	Q_ASSERT(m_channelHeaderMoveContext);
#if QT_VERSION_MAJOR >= 6
	int target_channel = moveChannelTragetIndex(event->position().toPoint());
#else
	int target_channel = moveChannelTragetIndex(event->pos());
#endif
	if (target_channel != m_channelHeaderMoveContext->draggedChannel) {
		graph()->moveChannel(m_channelHeaderMoveContext->draggedChannel, target_channel);
	}
	event->accept();
}

void GraphWidget::showGraphSelectionContextMenu(const QPoint &mouse_pos)
{
	QMenu menu(this);
	menu.addAction(tr("Zoom X axis to selection"), this, [this]() {
		m_graph->zoomToSelection(false);
		update();
	});
	auto *act_zoom_channel = menu.addAction(tr("Zoom channel to selection"), this, [this]() {
		m_graph->zoomToSelection(true);
		update();
	});
	qsizetype sel_ch1 = m_graph->posToChannel(m_graph->selectionRect().topLeft());
	qsizetype sel_ch2 = m_graph->posToChannel(m_graph->selectionRect().bottomRight());
	if (sel_ch1 != sel_ch2 || sel_ch1 < 0) {
		act_zoom_channel->setEnabled(false);
	}

	menu.addAction(tr("Show selection info"), this, [this]() {
		auto sel_rect = m_graph->selectionRect();
		auto ch1_ix = m_graph->posToChannel(sel_rect.bottomLeft());
		auto ch2_ix = m_graph->posToChannel(sel_rect.topLeft());
		auto *ch1 = m_graph->channelAt(ch1_ix);
		auto *ch2 = m_graph->channelAt(ch2_ix);
		auto t1 = m_graph->posToTime(sel_rect.left());
		auto t2 = m_graph->posToTime(sel_rect.right());
		auto y1 = ch1->posToValue(sel_rect.bottom());
		auto y2 = ch2->posToValue(sel_rect.top());
		QString s;

		if (m_graph->model()->xAxisType() == GraphModel::XAxisType::Timeline) {
			s = tr("t1: %1").arg(m_graph->timeToStringTZ(t1));
			s += '\n' + tr("t2: %1").arg(m_graph->timeToStringTZ(t2));
			s += '\n' + tr("duration: %1").arg(m_graph->durationToString(t2 - t1));
		}
		else {
			s = tr("x1: %1").arg(m_graph->timeToStringTZ(t1));
			s += '\n' + tr("x2: %1").arg(m_graph->timeToStringTZ(t2));
			s += '\n' + tr("width: %1").arg(m_graph->durationToString(t2 - t1));
		}

		s += '\n' + tr("y1: %1").arg(y1);
		s += '\n' + tr("y2: %1").arg(y2);
		if(ch1 == ch2)
			s += '\n' + tr("diff: %1").arg(y2 - y1);
		QMessageBox::information(this, tr("Selection info"), s);
	});

	menu.exec(mapToGlobal(mouse_pos));
}

void GraphWidget::showGraphContextMenu(const QPoint &mouse_pos)
{
	Q_UNUSED(mouse_pos);
}

void GraphWidget::showChannelContextMenu(qsizetype channel_ix, const QPoint &mouse_pos)
{
	shvLogFuncFrame();
	const GraphChannel *ch = m_graph->channelAt(channel_ix, !shv::core::Exception::Throw);
	if(!ch)
		return;

	QMenu menu(this);

	if(ch->isMaximized()) {
		menu.addAction(tr("Normal size"), this, [this, channel_ix]() {
			m_graph->setChannelMaximized(channel_ix, false);
		});
	}
	else {
		menu.addAction(tr("Maximize"), this, [this, channel_ix]() {
			m_graph->setChannelMaximized(channel_ix, true);
		});
	}

	menu.addAction(tr("Set probe (Ctrl + Left mouse)"), this, [this, channel_ix, mouse_pos]() {
		removeProbes(channel_ix);
		timemsec_t time = m_graph->posToTime(mouse_pos.x());
		createProbe(channel_ix, time);
	});
	menu.addAction(tr("Add probe (Ctrl + Shift + Left mouse)"), this, [this, channel_ix, mouse_pos]() {
			timemsec_t time = m_graph->posToTime(mouse_pos.x());
			createProbe(channel_ix, time);
		});
	menu.addAction(tr("Reset X-zoom"), this, [this]() {
		m_graph->resetXZoom();
		this->update();
	});
	menu.addAction(tr("Reset Y-zoom"), this, [this, channel_ix]() {
		m_graph->resetYZoom(channel_ix);
		this->update();
	});

	if (m_graph->isYAxisVisible()) {
		menu.addAction(tr("Hide Y axis"), this, [this]() {
			m_graph->setYAxisVisible(false);
		});
	}
	else {
		menu.addAction(tr("Show Y axis"), this, [this]() {
			m_graph->setYAxisVisible(true);
		});
	}

	if(menu.actions().count())
		menu.exec(mapToGlobal(mouse_pos));
}

void GraphWidget::createProbe(qsizetype channel_ix, timemsec_t time)
{
	const GraphChannel *ch = m_graph->channelAt(channel_ix);
	const GraphModel::ChannelInfo &channel_info = m_graph->model()->channelInfo(ch->modelIndex());

	if (channel_info.typeDescr.sampleType() == shv::core::utils::ShvTypeDescr::SampleType::Discrete) {
		Sample s = m_graph->timeToPreviousSample(channel_ix, time);

		if (s.isValid())
			time = s.time;
	}

	ChannelProbe *probe = m_graph->addChannelProbe(channel_ix, time);
	Q_ASSERT(probe);

	connect(probe, &ChannelProbe::currentTimeChanged, probe, [this](int64_t new_time) {
		bool is_time_visible = m_graph->xRangeZoom().contains(new_time);

		if (!is_time_visible) {
			XRange x_range;
			timemsec_t half_interval = m_graph->xRangeZoom().interval() / 2;
			x_range.min = new_time - half_interval;
			x_range.max = new_time + half_interval;

			m_graph->setXRangeZoom(x_range);
		}

		update();
	});

	auto *w = new ChannelProbeWidget(probe, this);

	connect(w, &ChannelProbeWidget::destroyed, this, [this, probe]() {
		m_graph->removeChannelProbe(probe);
		update();
	});

	w->show();
	QPoint pos(m_graph->timeToPos(time) - (w->width() / 2), -geometry().top() - w->height() - m_graph->u2px(0.2));
	w->move(mapToGlobal(pos));
	update();
}

void GraphWidget::removeProbes(qsizetype channel_ix)
{
	QList<ChannelProbeWidget*> probe_widgets = findChildren<ChannelProbeWidget*>(QString(), Qt::FindDirectChildrenOnly);

	for (ChannelProbeWidget *pw: probe_widgets) {
		if (pw->probe()->channelIndex() == channel_ix)
			pw->close();
	}
}

bool GraphWidget::isMouseAboveChannelResizeHandle(const QPoint &mouse_pos, Qt::Edge header_edge) const
{
	const Graph *gr = graph();
	const int MARGIN = gr->u2px(0.5);

	auto channel_ix = gr->posToChannelHeader(mouse_pos);

	if (channel_ix > -1) {
		const GraphChannel *ch = gr->channelAt(channel_ix);
		QRect header_rect = ch->verticalHeaderRect();

		switch (header_edge) {
		case Qt::Edge::BottomEdge:
			return (header_rect.bottom() - mouse_pos.y() < MARGIN);
			break;
		case Qt::Edge::RightEdge:
			return (header_rect.right() - mouse_pos.x() < MARGIN);
			break;
		default:
			shvWarning() << "Unsupported resize header edge option";
			break;
		}
	}

	return false;
}

GraphWidget::ChannelHeaderMoveContext::ChannelHeaderMoveContext(QWidget *parent)
{
	mouseMoveScrollTimer = new QTimer(parent);
	mouseMoveScrollTimer->setInterval(100);
	channelDropMarker = new QWidget(parent);
	QPalette pal = channelDropMarker->palette();
	pal.setColor(QPalette::ColorRole::Window, Qt::yellow);
	pal.setColor(QPalette::ColorRole::Base, Qt::yellow);
	pal.setColor(QPalette::ColorRole::ToolTipBase, Qt::yellow);
	channelDropMarker->setAutoFillBackground(true);
	channelDropMarker->setPalette(pal);
	channelDropMarker->hide();
}

GraphWidget::ChannelHeaderMoveContext::~ChannelHeaderMoveContext()
{
	delete mouseMoveScrollTimer;
	delete channelDropMarker;
}

}

