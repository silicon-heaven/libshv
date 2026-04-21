#include <shv/visu/toast/toastwidget.h>

#include "ui_toastwidget.h"

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QTimer>

namespace shv::visu::toast {

namespace Ui {
class ToastWidget;
}

static constexpr int ANIMATION_DURATION = 200;

ToastWidget::ToastWidget(QWidget *parent):
	Super(parent),
	ui(new Ui::ToastWidget)
{
	ui->setupUi(this);

	auto *m_effect = new QGraphicsOpacityEffect(this);

	setAttribute(Qt::WA_StyledBackground, true);
	setGraphicsEffect(m_effect);
	m_effect->setOpacity(0);
	auto p = palette();
	p.setColor(QPalette::WindowText, Qt::black);
	setPalette(p);

	m_animation = new QPropertyAnimation(m_effect, "opacity", this);
	m_timer = new QTimer(this);

	ui->lMessage->setAlignment(Qt::AlignCenter);
	ui->lMessage->setWordWrap(true);

	//common::applyCssStyleClass(ui->tbClose, style::MENU_WIDGET_HIDE_BUTTON, common::PolishCssStyle::No);

	connect(m_animation, &QPropertyAnimation::finished, this, &ToastWidget::onAnimationFinished);
	connect(m_timer, &QTimer::timeout, this, &ToastWidget::dismissToast);
	connect(ui->tbClose, &QToolButton::clicked, this, &ToastWidget::dismissToast);
}

void ToastWidget::showToast(const ToastMessage &msg)
{
	ui->lMessage->setText(msg.message());

	QColor color = ToastMessage::toastMessageTypeToColor(msg.messageType());
	setStyleSheet(QString("QWidget {background-color: %1; border-radius: 8px;}").arg(color.name()));

	QFontMetrics fm(ui->lMessage->font());
	int height = fm.boundingRect(0, 0, width(), 0, Qt::TextWordWrap, msg.message()).height();
	setFixedHeight(height + 20);

	m_timer->start(msg.durationSecs() * 1000);

	m_animation->setDuration(ANIMATION_DURATION);
	m_animation->setStartValue(0.0);
	m_animation->setEndValue(1.0);
	m_animation->start();
}

void ToastWidget::dismissToast()
{
	m_animation->setDuration(ANIMATION_DURATION);
	m_animation->setStartValue(1.0);
	m_animation->setEndValue(0.0);
	m_animation->start();
}

void ToastWidget::onAnimationFinished()
{
	if (m_animation->endValue() == 0.0) {
		emit toastClosed();
	}
}
}
