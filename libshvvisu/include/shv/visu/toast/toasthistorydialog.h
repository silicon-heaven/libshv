#pragma once

#include "toastmessage.h"

#include <shv/visu/shvvisu_export.h>

#include <QDialog>

namespace shv::visu::toast {

namespace Ui {
class ToastHistoryDialog;
}

class LIBSHVVISU_EXPORT ToastHistoryDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ToastHistoryDialog(QWidget *parent, const QList<ToastMessage> &messages);
	~ToastHistoryDialog() override;

private:
	Ui::ToastHistoryDialog *ui;
};
}
