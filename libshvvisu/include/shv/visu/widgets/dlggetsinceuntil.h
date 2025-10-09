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
	explicit DlgGetSinceUntil(QWidget *parent, const QSet<QByteArray> &available_timezone_ids = {});
	~DlgGetSinceUntil() override;

#if QT_CONFIG(timezone)
	std::tuple<QDateTime, QDateTime> getSinceUntil() const;
	QTimeZone timeZone() const;
	void setTimeZone(const QTimeZone &time_zone);
#endif

private:
	void enableOk();
	void createSinceUntilMenu();
	qlonglong recentValuesDuration() const;
	void adjustDateTime(QDateTimeEdit *from_dte, QDateTimeEdit *to_dte, int64_t msecs);

	Ui::DlgGetSinceUntil *ui;
};
}
