#include "timezonecombobox.h"

#include <QKeyEvent>
#include <QLineEdit>
#if SHVVISU_HAS_TIMEZONE
#include <QTimeZone>
#endif
#include <QDebug>

namespace shv {
namespace visu {

//const char *LOCAL_TZ_ID = "Local";

TimeZoneComboBox::TimeZoneComboBox(QWidget *parent)
	: Super(parent)
{
#if SHVVISU_HAS_TIMEZONE
	setEditable(true);
	//addItem(LOCAL_TZ_ID);
	for(auto tzn : QTimeZone::availableTimeZoneIds()) {
		addItem(tzn);
	}
	setCurrentIndex(findText(QTimeZone::systemTimeZoneId()));
	/*
	connect(this, &TimeZoneComboBox::editTextChanged, [this](const QString &text) {
		int ix = findText(text, Qt::MatchContains);
		if(ix >= 0) {
			setCurrentIndex(ix);
		}
	});
	*/
#endif
}

#if SHVVISU_HAS_TIMEZONE
QTimeZone TimeZoneComboBox::currentTimeZone() const
{
	if(currentIndex() < 0)
		return QTimeZone();
	if(currentIndex() == 0)
		return QTimeZone::systemTimeZone();
	return QTimeZone(currentText().toUtf8());
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
	//qDebug() << "pressed:" << event;
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
		int sel_start = curr_text.indexOf(m_searchText, 0, Qt::CaseInsensitive);
		//qDebug() << "curr:" << curr_text << "search:" << m_searchText << "sel_start:" << sel_start;
		ed->setSelection(sel_start, m_searchText.length());
		ed->setFocus();
	}
}
#endif

}}
