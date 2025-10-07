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
	QTimeZone currentTimeZone() const;
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	QString m_searchText;
#endif
};

}
