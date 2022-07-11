#include "channelprobewidget.h"
#include "channelprobe.h"

#include "ui_channelprobewidget.h"

#include <shv/core/log.h>

#include <QMouseEvent>

namespace shv {
namespace visu {
namespace timeline {

ChannelProbeWidget::ChannelProbeWidget(ChannelProbe *probe, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ChannelProbeWidget)
{
	ui->setupUi(this);
	m_probe = probe;

	ui->lblTitle->setText(m_probe->shvPath());

	ui->edCurentTime->setStyleSheet("background-color: white");
	ui->fHeader->setStyleSheet("background-color:" + m_probe->color().name() + ";");
	ui->tbClose->setStyleSheet("background-color:white;");
	ui->lblTitle->setStyleSheet("color:white;");

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowType::Window | Qt::WindowStaysOnTopHint);

	ui->twData->setColumnCount(DataTableColumn::ColCount);
	ui->twData->setHorizontalHeaderItem(DataTableColumn::ColProperty, new QTableWidgetItem("Property"));
	ui->twData->setHorizontalHeaderItem(DataTableColumn::ColValue, new QTableWidgetItem("Value"));
	ui->twData->verticalHeader()->setDefaultSectionSize((int)(fontMetrics().lineSpacing() * 1.3));

	installEventFilter(this);
	ui->fHeader->installEventFilter(this);
	ui->twData->viewport()->installEventFilter(this);

	loadValues(m_probe->currentTime());

	ui->twData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

	connect(ui->tbClose, &QToolButton::clicked, this, &ChannelProbeWidget::close);
	connect(m_probe, &ChannelProbe::currentTimeChanged, this, &ChannelProbeWidget::loadValues);
	connect(ui->tbPrevSample, &QToolButton::clicked, m_probe, &ChannelProbe::prevSample);
	connect(ui->tbNextSample, &QToolButton::clicked, m_probe, &ChannelProbe::nextSample);
}

ChannelProbeWidget::~ChannelProbeWidget()
{
	delete ui;
}

const ChannelProbe *ChannelProbeWidget::probe()
{
	return m_probe;
}

bool ChannelProbeWidget::eventFilter(QObject *o, QEvent *e)
{
	if (e->type() == QEvent::MouseButtonPress) {
		QPoint pos = static_cast<QMouseEvent*>(e)->pos();
		m_frameSection = getFrameSection();

		if (m_frameSection != FrameSection::NoSection) {
			m_mouseOperation = MouseOperation::ResizeWidget;
			e->accept();
			return true;
		}
		else if (o == ui->fHeader) {
			setCursor(QCursor(Qt::DragMoveCursor));
			m_recentMousePos = pos;
			m_mouseOperation = MouseOperation::MoveWidget;
			e->accept();
			return true;
		}
	}
	else if (e->type() == QEvent::MouseButtonRelease) {
		m_mouseOperation = MouseOperation::None;
		setCursor(QCursor(Qt::ArrowCursor));
	}
	else if (e->type() == QEvent::MouseMove) {
		QPoint pos = static_cast<QMouseEvent*>(e)->pos();

		if (m_mouseOperation == MouseOperation::MoveWidget) {
			QPoint dist = pos - m_recentMousePos;
			move(geometry().topLeft() + dist);
			m_recentMousePos = pos - dist;
			e->accept();
			return true;
		}
		else if (m_mouseOperation == MouseOperation::ResizeWidget) {
			QPoint pos =  QCursor::pos();
			QRect g = geometry();

			switch (m_frameSection) {
			case FrameSection::NoSection:
				break;
			case FrameSection::Left:
				g.setLeft(pos.x());
				break;
			case FrameSection::Right:
				g.setRight(pos.x());
				break;
			case FrameSection::Top:
				g.setTop(pos.y());
				break;
			case FrameSection::Bottom:
				g.setBottom(pos.y());
				break;
			case FrameSection::TopLeft:
				g.setTopLeft(pos);
				break;
			case FrameSection::BottomRight:
				g.setBottomRight(pos);
				break;
			case FrameSection::TopRight:
				g.setTopRight(pos);
				break;
			case FrameSection::BottomLeft:
				g.setBottomLeft(pos);
				break;
			}

			setGeometry(g);
			e->accept();
			return true;
		}
		FrameSection fs = getFrameSection();
		setCursor(frameSectionCursor(fs));
	}

	return Super::eventFilter(o, e);
}

void ChannelProbeWidget::loadValues(int64_t time)
{
	Q_UNUSED(time);
	ui->edCurentTime->setText(m_probe->currentTimeIsoFormat());
	ui->twData->clearContents();
	ui->twData->setRowCount(0);

	QMap<QString, QString> values = m_probe->yValues();
	QMapIterator<QString, QString> i(values);

	while (i.hasNext()) {
		i.next();
		int ix = ui->twData->rowCount();
		ui->twData->insertRow(ix);

		QTableWidgetItem *item = new QTableWidgetItem(i.key());
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twData->setItem(ix, DataTableColumn::ColProperty, item);

		item = new QTableWidgetItem(i.value());
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twData->setItem(ix, DataTableColumn::ColValue, item);
	}
}

ChannelProbeWidget::FrameSection ChannelProbeWidget::getFrameSection()
{
	static const int FRAME_MARGIN = 6;

	QPoint pos = QCursor::pos();
	bool left_margin = (pos.x() - geometry().left() < FRAME_MARGIN);
	bool right_margin = (geometry().right() - pos.x() < FRAME_MARGIN);
	bool top_margin = (pos.y() - geometry().top() < FRAME_MARGIN);
	bool bottom_margin = (geometry().bottom() - pos.y() < FRAME_MARGIN);

	if (top_margin && left_margin)
		return FrameSection::TopLeft;
	else if (top_margin && right_margin)
		return FrameSection::TopRight;
	else if (bottom_margin && left_margin)
		return FrameSection::BottomLeft;
	else if (bottom_margin && right_margin)
		return FrameSection::BottomRight;
	else if (left_margin)
		return FrameSection::Left;
	else if (right_margin)
		return FrameSection::Right;
	else if (top_margin)
		return FrameSection::Top;
	else if (bottom_margin)
		return FrameSection::Bottom;
	else
		return FrameSection::NoSection;
}

QCursor ChannelProbeWidget::frameSectionCursor(ChannelProbeWidget::FrameSection fs)
{
	switch (fs) {
	case FrameSection::NoSection:
		return QCursor(Qt::ArrowCursor);
	case FrameSection::Left:
	case FrameSection::Right:
		return QCursor(Qt::SizeHorCursor);
	case FrameSection::Top:
	case FrameSection::Bottom:
		return QCursor(Qt::SizeVerCursor);
	case FrameSection::TopLeft:
	case FrameSection::BottomRight:
		return QCursor(Qt::SizeFDiagCursor);
	case FrameSection::TopRight:
	case FrameSection::BottomLeft:
		return QCursor(Qt::SizeBDiagCursor);
	}
	return QCursor(Qt::ArrowCursor);
}

}}}
