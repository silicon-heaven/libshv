#include <shv/visu/widgets/timezonecombobox.h>

#include <shv/core/log.h>

#include <QKeyEvent>
#include <QLineEdit>
#if QT_CONFIG(timezone)
#include <QTimeZone>
#endif

namespace shv::visu::widgets {

TimeZoneComboBox::TimeZoneComboBox(QWidget *parent)
	: Super(parent)
{
#if QT_CONFIG(timezone)
	setEditable(true);
#endif
}

#if QT_CONFIG(timezone)
static constexpr int INVALID_COMBOBOX_INDEX = -1;

void TimeZoneComboBox::setTimeZones(const QList<QByteArray> &available_timezone_ids)
{
	clear();
	const QList<QByteArray> all_tz_ids = QTimeZone::availableTimeZoneIds();

	for (const QByteArray &tzn : available_timezone_ids) {
		if (all_tz_ids.contains(tzn)) {
			addItem(QString::fromUtf8(tzn));
		}
		else {
			shvWarning() << "Cannot add timezone id to combobox:" << QString::fromUtf8(tzn).toStdString();
		}
	}

	setCurrentIndex(findText(QString::fromUtf8(QTimeZone::systemTimeZoneId())));
}

QTimeZone TimeZoneComboBox::currentTimeZone() const
{
	if(currentIndex() <= INVALID_COMBOBOX_INDEX) {
		return QTimeZone();
	}

	return QTimeZone(currentText().toUtf8());
}

void TimeZoneComboBox::setCurrentTimeZone(const QTimeZone &time_zone)
{
	auto idx = findText(QString::fromUtf8(time_zone.id()));
	if (idx <= INVALID_COMBOBOX_INDEX) {
		shvWarning() << "Failed to selecet timezone:" << time_zone.id().toStdString() << "in combobox";
	}
	setCurrentIndex(idx);
}

void TimeZoneComboBox::keyPressEvent(QKeyEvent *event)
{
	QLineEdit *ed = lineEdit();
	if(event->key() == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
		ed->selectAll();
		event->accept();
		return;
	}
	if(ed->text().length() > 0 && ed->selectionLength() == ed->text().length()) {
		m_searchText = QString();
		ed->setText(m_searchText);
	}
	QString c = event->text();
	if (c.isEmpty()) {
		m_searchText.clear();
		Super::keyPressEvent(event);
		return;
	}

	c = m_searchText + c;
	int ix = findText(c, Qt::MatchContains);
	if(ix >= 0) {
		event->accept();
		m_searchText = c;
		setCurrentIndex(ix);
		auto curr_text = currentText();
		ed->setText(curr_text);
		auto sel_start = curr_text.indexOf(m_searchText, 0, Qt::CaseInsensitive);
#if QT_VERSION_MAJOR >= 6
		ed->setSelection(static_cast<int>(sel_start), static_cast<int>(m_searchText.length()));
#else
		ed->setSelection(sel_start, m_searchText.length());
#endif
		ed->setFocus();
	}
}
#endif

}
