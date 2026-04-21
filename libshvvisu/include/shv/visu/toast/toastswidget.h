#pragma once

#include "toastmessage.h"
#include "toastwidget.h"

#include <shv/visu/shvvisu_export.h>

#include <QWidget>

namespace shv::visu::toast {

namespace Ui {
class ToastsWidget;
}

class LIBSHVVISU_EXPORT ToastsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ToastsWidget(QWidget *parent = nullptr);
	~ToastsWidget() override;

	void addToast(const ToastMessage &toast_message);

private:
	void resizeAndUpdatePosition();
	void deleteToastWidget(ToastWidget *widget);

	Ui::ToastsWidget *ui;
};
}
