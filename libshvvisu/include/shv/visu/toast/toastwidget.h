#pragma once

#include "toastmessage.h"

#include <shv/visu/shvvisu_export.h>

#include <QWidget>
#include <QPropertyAnimation>

class QLabel;
class QTimer;
class QGraphicsOpacityEffect;

namespace shv::visu::toast {

namespace Ui {
class ToastWidget;
}

class LIBSHVVISU_EXPORT ToastWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
public:
	explicit ToastWidget(QWidget *parent = nullptr);

	void showToast(const ToastMessage &msg);

	Q_SIGNAL void toastClosed();

private:
	void dismissToast();
	void onAnimationFinished();

	QPropertyAnimation *m_animation;
	QTimer *m_timer;

	Ui::ToastWidget *ui;
};
}
