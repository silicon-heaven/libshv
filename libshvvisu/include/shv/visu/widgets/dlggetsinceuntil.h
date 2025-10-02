#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QDialog>
#include <QTimeZone>

class QDateTimeEdit;

namespace shv::visu::widgets {

namespace Ui {
class DlgGetSinceUntil;
}

class SHVVISU_DECL_EXPORT DlgGetSinceUntil : public QDialog
{
	Q_OBJECT
	using Super = QDialog;

public:
	explicit DlgGetSinceUntil(QWidget *parent = nullptr);
	~DlgGetSinceUntil() override;

	std::tuple<QDateTime, QDateTime> getSinceUntil() const;
#if QT_CONFIG(timezone)
	QTimeZone timeZone() const;
#endif

private:
	void enableOk();
	void createSinceUntilMenu();
	qlonglong recentValuesDuration() const;
	void adjustDateTime(QDateTimeEdit *from_dte, QDateTimeEdit *to_dte, int64_t msecs);

	Ui::DlgGetSinceUntil *ui;
};
}
