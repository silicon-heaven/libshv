#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QComboBox>

class QTimeZone;

namespace shv::visu::widgets {

class SHVVISU_DECL_EXPORT TimeZoneComboBox : public QComboBox
{
	using Super = QComboBox;
public:
	TimeZoneComboBox(QWidget *parent = nullptr);

#if QT_CONFIG(timezone)
	void createTimeZones(const QSet<QByteArray> &available_timezones_id);
	QTimeZone currentTimeZone() const;
	void setCurrentTimeZone(const QTimeZone &time_zone);
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	QString m_searchText;
#endif
};

}
