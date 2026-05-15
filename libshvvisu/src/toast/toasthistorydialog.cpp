#include <shv/visu/toast/toasthistorydialog.h>

#include "ui_toasthistorydialog.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QHeaderView>

namespace shv::visu::toast {

ToastHistoryDialog::ToastHistoryDialog(QWidget *parent, const QList<ToastMessage> &messages) :
	QDialog(parent),
	ui(new Ui::ToastHistoryDialog)
{
	ui->setupUi(this);

	for (const auto &msg : messages) {
		int row = ui->twHistory->rowCount();
		ui->twHistory->insertRow(row);

		auto timestamp_item = new QTableWidgetItem(msg.creationTimestamp().toString("dd.MM.yyyy HH:mm:ss"));
		auto type_item = new QTableWidgetItem(ToastMessage::toastMessageTypeToString(msg.messageType()));
		auto message_item = new QTableWidgetItem(msg.message());

		auto bg_color = ToastMessage::toastMessageTypeToColor(msg.messageType());
		timestamp_item->setBackground(QBrush(bg_color));
		type_item->setBackground(QBrush(bg_color));
		message_item->setBackground(QBrush(bg_color));

		ui->twHistory->setItem(row, 0, timestamp_item);
		ui->twHistory->setItem(row, 1, type_item);
		ui->twHistory->setItem(row, 2, message_item);
	}

	ui->twHistory->resizeColumnsToContents();
}

ToastHistoryDialog::~ToastHistoryDialog()
{
	delete ui;
}

}
