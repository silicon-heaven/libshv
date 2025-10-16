#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QComboBox>
#include <QSet>
#include <QByteArray>

#if QT_CONFIG(timezone)
#include <QTimeZone>
#endif
namespace shv::visu::widgets {

class SHVVISU_DECL_EXPORT TimeZoneComboBox : public QComboBox
{
	using Super = QComboBox;
public:
	TimeZoneComboBox(QWidget *parent = nullptr);

#if QT_CONFIG(timezone)
	void setTimeZones(const QList<QByteArray> &available_timezone_ids = QTimeZone::availableTimeZoneIds());
	QTimeZone currentTimeZone() const;
	void setCurrentTimeZone(const QTimeZone &time_zone);
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	QString m_searchText;
#endif
};

}
