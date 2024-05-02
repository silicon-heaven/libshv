#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QSet>
#include <QWidget>

namespace shv::visu::timeline {

namespace Ui {
class ChannelProbeWidget;
}

class ChannelProbe;

class SHVVISU_DECL_EXPORT ChannelProbeWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;

public:
	explicit ChannelProbeWidget(ChannelProbe *probe, QWidget *parent);
	~ChannelProbeWidget();

	const ChannelProbe* probe();

protected:
	enum class MouseOperation { None = 0, LeftMouseClick, ResizeWidget, MoveWidget };
	enum class FrameSection { NoSection, Left, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft };

	enum DataTableColumn { ColProperty = 0, ColValue, ColCount };

	bool eventFilter(QObject *o, QEvent *e) override;

	void loadValues();
	ChannelProbeWidget::FrameSection getFrameSection();
	QCursor frameSectionCursor(FrameSection fs);


	ChannelProbe *m_probe = nullptr;
	QPoint m_recentMousePos;

	MouseOperation m_mouseOperation = MouseOperation::None;
	ChannelProbeWidget::FrameSection m_frameSection = FrameSection::NoSection;

	Ui::ChannelProbeWidget *ui;
};

}
