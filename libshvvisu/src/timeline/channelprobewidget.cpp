#include "ui_channelprobewidget.h"

#include <shv/visu/timeline/channelprobewidget.h>
#include <shv/visu/timeline/channelprobe.h>
#include <shv/visu/timeline/graph.h>

#include <QMouseEvent>

namespace shv::visu::timeline {

ChannelProbeWidget::ChannelProbeWidget(ChannelProbe *probe, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ChannelProbeWidget)
{
	ui->setupUi(this);
	m_probe = probe;

	if (m_probe->isLocalizeShvPath()) {
		ui->lblTitle->setText(m_probe->localizedShvPath());
	}
	else {
		ui->lblTitle->setText(m_probe->shvPath());
	}

	ui->edCurentTime->setStyleSheet("background-color: white");
	ui->fHeader->setStyleSheet("background-color:" + m_probe->color().name() + ";");
	ui->tbClose->setStyleSheet("background-color:white;");
	if (m_probe->color().lightness() < 127) {
		ui->lblTitle->setStyleSheet("color:white;");
	}

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);

	ui->twData->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().lineSpacing() * 1.3));

	installEventFilter(this);
	ui->fHeader->installEventFilter(this);
	ui->twData->viewport()->installEventFilter(this);

	loadValues();

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
		if (o == ui->fHeader) {
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

		if (m_mouseOperation == MouseOperation::MoveWidget) {
			QPoint pos = static_cast<QMouseEvent*>(e)->pos();
			QPoint dist = pos - m_recentMousePos;
			move(geometry().topLeft() + dist);
			m_recentMousePos = pos - dist;
			e->accept();
			return true;
		}
		if (m_mouseOperation == MouseOperation::ResizeWidget) {
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

void ChannelProbeWidget::loadValues()
{
	ui->edCurentTime->setText(m_probe->currentTimeIsoFormat());
	ui->twData->clearContents();
	ui->twData->setRowCount(0);

	QVariantMap values = m_probe->sampleValues();
	auto pv = values.value(Graph::KEY_SAMPLE_PRETTY_VALUE);
#if QT_VERSION_MAJOR >= 6
	if(pv.typeId() == QMetaType::QVariantMap) {
#else
	if(pv.type() == QVariant::Map) {
#endif
		ui->twData->setColumnCount(MapColCount);
		ui->twData->setHorizontalHeaderLabels(QStringList{ tr("Property"), tr("Value") });
		auto m = pv.toMap();
		QMapIterator<QString, QVariant> i(m);
		while (i.hasNext()) {
			i.next();
			int ix = ui->twData->rowCount();
			ui->twData->insertRow(ix);

			auto *item = new QTableWidgetItem(i.key());
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			ui->twData->setItem(ix, MapColProperty, item);

			item = new QTableWidgetItem(i.value().toString());
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			ui->twData->setItem(ix, MapColValue, item);
		}
	}
#if QT_VERSION_MAJOR >= 6
	else if(pv.typeId() == QMetaType::QVariantList) {
#else
	else if(pv.type() == QVariant::List) {
#endif
		auto l = pv.toList();
		if (l.count()) {
			auto header = l[0].toList();
			ui->twData->setColumnCount(static_cast<int>(header.count()));
			QStringList labels;
			std::for_each(header.begin(), header.end(), [&labels](const auto &label) { labels << label.toString(); });
			ui->twData->setHorizontalHeaderLabels(labels);

			ui->twData->setRowCount(static_cast<int>(l.count() - 1));
			for (int i = 1; i < l.count(); ++i) {
				auto row = l[i].toList();
				for (int j = 0; j < row.count(); ++j) {
					auto *item = new QTableWidgetItem(row[j].toString());
					item->setFlags(item->flags() & ~Qt::ItemIsEditable);
					ui->twData->setItem(i - 1, j, item);
				}
			}
		}
	}
	else {
		ui->twData->setColumnCount(DataColCount);
		ui->twData->setHorizontalHeaderLabels(QStringList{ tr("Value") });
		int ix = ui->twData->rowCount();
		ui->twData->insertRow(ix);

		QString s = pv.toString();
		if (s.endsWith('\n')) {
			s.chop(1);
		}
		auto *item = new QTableWidgetItem(s);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twData->setItem(ix, DataColValue, item);
	}
	ui->twData->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
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
	if (top_margin && right_margin)
		return FrameSection::TopRight;
	if (bottom_margin && left_margin)
		return FrameSection::BottomLeft;
	if (bottom_margin && right_margin)
		return FrameSection::BottomRight;
	if (left_margin)
		return FrameSection::Left;
	if (right_margin)
		return FrameSection::Right;
	if (top_margin)
		return FrameSection::Top;
	if (bottom_margin)
		return FrameSection::Bottom;

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

}
