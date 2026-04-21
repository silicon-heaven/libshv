#include <shv/visu/toast/toastswidget.h>

#include "ui_toastswidget.h"
#include <shv/visu/toast/toastwidget.h>

namespace shv::visu::toast {

static constexpr int MAX_VISIBLE_TOASTS = 5;

ToastsWidget::ToastsWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ToastsWidget)
{
	ui->setupUi(this);
	resizeAndUpdatePosition();
}

ToastsWidget::~ToastsWidget()
{
	delete ui;
}

void ToastsWidget::addToast(const ToastMessage &toast_message)
{
	static constexpr int FIRST_TOAST_POSITION = 0;

	if (ui->mainLayout->count() >= MAX_VISIBLE_TOASTS) {
		auto *oldest_toast = qobject_cast<ToastWidget*>(ui->mainLayout->itemAt(ui->mainLayout->count() -1)->widget());
		deleteToastWidget(oldest_toast);
	}

	auto *toast = new ToastWidget(this);
	ui->mainLayout->insertWidget(FIRST_TOAST_POSITION, toast);

	connect(toast, &ToastWidget::toastClosed, this, [this, toast] {
		deleteToastWidget(toast);
	});

	toast->showToast(toast_message);
	resizeAndUpdatePosition();
}

void ToastsWidget::resizeAndUpdatePosition()
{
	int total_height = 0;

	for (int i = 0; i < ui->mainLayout->count(); i++) {
		auto *toast = qobject_cast<ToastWidget*>(ui->mainLayout->itemAt(i)->widget());

		if (total_height > 0) {
			total_height += ui->mainLayout->spacing();
		}

		total_height += toast->height();
	}

	setVisible(total_height > 0);
	setFixedHeight(total_height);
	move(parentWidget()->width() - width(), parentWidget()->height() - height() - 30);
}

void ToastsWidget::deleteToastWidget(ToastWidget *widget)
{
	ui->mainLayout->removeWidget(widget);
	widget->deleteLater();

	resizeAndUpdatePosition();
}
}
