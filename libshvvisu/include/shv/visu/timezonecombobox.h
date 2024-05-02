#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QComboBox>

class QTimeZone;

namespace shv::visu {

class SHVVISU_DECL_EXPORT TimeZoneComboBox : public QComboBox
{
	using Super = QComboBox;
public:
	TimeZoneComboBox(QWidget *parent = nullptr);

#if SHVVISU_HAS_TIMEZONE
	QTimeZone currentTimeZone() const;
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	QString m_searchText;
#endif
};

}
