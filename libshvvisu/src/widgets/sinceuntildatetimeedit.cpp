#include <shv/visu/widgets/sinceuntildatetimeedit.h>

#include <QMouseEvent>
#include <QStyleOptionSpinBox>

static constexpr int64_t INVALID_DT = 0;

namespace shv::visu::widgets {

SinceUntilDateTimeEdit::SinceUntilDateTimeEdit(QWidget *parent)
	: Super(parent)
{
#if QT_VERSION_MAJOR >=6 && QT_VERSION_MINOR >= 8
	setTimeZone(QTimeZone::LocalTime);
#else
	setTimeSpec(Qt::TimeSpec::LocalTime);
#endif

	setMinimumDateTime(QDateTime::fromSecsSinceEpoch(INVALID_DT));
	setSpecialValueText(tr("Select a date"));
}

void SinceUntilDateTimeEdit::mousePressEvent(QMouseEvent *mouse_event)
{
	if (mouse_event->type() == QEvent::MouseButtonPress) {
		QStyleOptionSpinBox opt;
		initStyleOption(&opt);
		QRect button_rect = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_ComboBoxArrow, this);
		if (button_rect.contains(mouse_event->pos()) && dateTime() == minimumDateTime()) {
			setDateTime(QDateTime::currentDateTime());
		}
	}
	Super::mousePressEvent(mouse_event);
}

void SinceUntilDateTimeEdit::focusInEvent(QFocusEvent *event)
{
	if (dateTime() == minimumDateTime()) {
		setDateTime(QDateTime::currentDateTime());
		setCurrentSectionIndex(0);
	}
	Super::focusInEvent(event);
}
}
