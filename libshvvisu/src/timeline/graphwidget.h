#pragma once

#include "graph.h"
#include "graphmodel.h"
#include "channelprobewidget.h"

#include <shv/coreqt/utils.h>

#include <QLabel>
#include <QTimer>
#include <QWidget>

namespace shv {
namespace visu {
namespace timeline {

class Graph;

class SHVVISU_DECL_EXPORT GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
	//friend class GraphWidget;
public:
	GraphWidget(QWidget *parent = nullptr);

	void setGraph(Graph *g);
	Graph *graph();
	const Graph *graph() const;

#if SHVVISU_HAS_TIMEZONE
	void setTimeZone(const QTimeZone &tz);
#endif

	void makeLayout(const QSize &preferred_size);
	void makeLayout();

	Q_SIGNAL void graphChannelDoubleClicked(const QPoint &mouse_pos);
protected:
	bool event(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	//void keyPressEvent(QKeyEvent *event) override;
	//void keyReleaseEvent(QKeyEvent *event) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	void hideCrossHair();

	virtual void showGraphSelectionContextMenu(const QPoint &mouse_pos);
	virtual void showGraphContextMenu(const QPoint &mouse_pos);
	virtual void showChannelContextMenu(int channel_ix, const QPoint &mouse_pos);
protected:
	void createProbe(int channel_ix, timemsec_t time);
	void removeProbes(int channel_ix);
	bool isMouseAboveChannelResizeHandle(const QPoint &mouse_pos) const;
	bool isMouseAboveMiniMap(const QPoint &mouse_pos) const;
	bool isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const;
	bool isMouseAboveLeftMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveRightMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveMiniMapSlider(const QPoint &pos) const;
	int posToChannelVerticalHeader(const QPoint &pos) const;
	int posToChannel(const QPoint &pos) const;
	void scrollToCurrentMousePosOnDrag();
	bool scrollByMouseOuterOverlap(const QPoint &mouse_pos);
	void moveDropMarker(const QPoint &mouse_pos);
	int moveChannelTragetIndex(const QPoint &mouse_pos) const;

protected:
	Graph *m_graph = nullptr;
	QSize m_graphPreferredSize;
	enum class MouseOperation {
		None = 0,
		MiniMapLeftResize,
		MiniMapRightResize,
		ChannelHeaderResize,
		MiniMapScrollZoom,
		ChannelHeaderMove,
		GraphDataAreaLeftPress,
		GraphDataAreaLeftCtrlPress,
		GraphDataAreaLeftCtrlShiftPress,
		GraphAreaMove,
		GraphAreaSelection,
		GraphDataAreaRightPress,
	};
	MouseOperation m_mouseOperation = MouseOperation::None;
	QPoint m_recentMousePos;
	int m_resizeChannelIx = -1;

	struct ChannelHeaderMoveContext
	{
		ChannelHeaderMoveContext(QWidget *parent);
		~ChannelHeaderMoveContext();
		QTimer *mouseMoveScrollTimer;
		QWidget *channelDropMarker;
		int draggedChannel;
	};
	ChannelHeaderMoveContext *m_channelHeaderMoveContext;
};

}}}
