#pragma once

#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/channelprobewidget.h>

#include <shv/coreqt/utils.h>

#include <QLabel>
#include <QTimer>
#include <QWidget>

class QMenu;
namespace shv::visu::timeline {

class Graph;

class LIBSHVVISU_EXPORT GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
public:
	GraphWidget(QWidget *parent = nullptr);

	void setGraph(Graph *g);
	Graph *graph();
	const Graph *graph() const;

#if QT_CONFIG(timezone)
	void setTimeZone(const QTimeZone &tz);
#endif

	void makeLayout(const QSize &preferred_size);
	void makeLayout();
	void setProbesEnabled(bool b);

	Q_SIGNAL void graphChannelDoubleClicked(const QPoint &mouse_pos);
protected:
	bool event(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

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

	void showChannelContextMenu(qsizetype channel_ix, const QPoint &mouse_pos);
	virtual QMenu* createChannelContextMenu(qsizetype channel_ix, const QPoint &mouse_pos);
protected:
	void createProbe(qsizetype channel_ix, timemsec_t time);
	void removeProbes(qsizetype channel_ix);
	bool isMouseAboveChannelResizeHandle(const QPoint &mouse_pos, Qt::Edge header_edge) const;
	bool isMouseAboveMiniMap(const QPoint &mouse_pos) const;
	bool isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const;
	bool isMouseAboveLeftMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveRightMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveMiniMapSlider(const QPoint &pos) const;
	int posToChannelVerticalHeader(const QPoint &pos) const;
	std::optional<qsizetype> posToChannel(const QPoint &pos) const;
	void scrollToCurrentMousePosOnDrag();
	bool scrollByMouseOuterOverlap(const QPoint &mouse_pos);
	void moveDropMarker(const QPoint &mouse_pos);
	int moveChannelTargetIndex(const QPoint &mouse_pos) const;

	static Graph::ZoomType zoomTypeFromRect(const QRect &rect);
	void updateXZoomOnPinch(std::pair<int, int> new_pos);

protected:
	Graph *m_graph = nullptr;
	QSize m_graphPreferredSize;
	enum class MouseOperation {
		None = 0,
		MiniMapLeftResize,
		MiniMapRightResize,
		ChannelHeaderResizeHeight,
		GraphVerticalHeaderResizeWidth,
		MiniMapScrollZoom,
		ChannelHeaderMove,

		GraphDataAreaLeftPress,
		GraphDataAreaLeftCtrlPress,
		GraphDataAreaLeftCtrlShiftPress,
		GraphDataAreaMiddlePress,
		GraphDataAreaRightPress,

		GraphAreaMove,
		GraphAreaSelection,
	};
	MouseOperation m_mouseOperation = MouseOperation::None;
	QPoint m_mouseOperationStartPos;
	std::optional<qsizetype> m_resizeChannelIx;

	XRange m_pinchZoom;
	std::pair<int, int> m_pinchOperationStart;

	struct ChannelHeaderMoveContext
	{
		ChannelHeaderMoveContext(QWidget *parent);
		~ChannelHeaderMoveContext();
		QTimer *mouseMoveScrollTimer;
		QWidget *channelDropMarker;
		int draggedChannel;
	};
	ChannelHeaderMoveContext *m_channelHeaderMoveContext = nullptr;
	bool m_probesEnabled = true;
private:
	Graph::ZoomType m_zoomType = Graph::ZoomType::ZoomToRect;
};
}
