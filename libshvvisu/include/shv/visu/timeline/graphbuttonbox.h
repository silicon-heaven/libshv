#pragma once

#include <shv/core/utils.h>

#include <QObject>
#include <QRect>

class QPainter;

namespace shv::visu::timeline {

class Graph;

class GraphButtonBox : public QObject
{
	Q_OBJECT

	SHV_FIELD_BOOL_IMPL2(a, A, utoRaise, true)
public:
	enum class ButtonId { Invalid = 0, Menu, User };
public:
	GraphButtonBox(const QVector<ButtonId> &button_ids, QObject *parent);
	virtual ~GraphButtonBox() = default;

	Q_SIGNAL void buttonClicked(int button_id);

	void moveTopRight(const QPoint &p);
	void hide();

	bool processEvent(QEvent *ev);

	virtual void draw(QPainter *painter);

	QSize size() const;
	int buttonSize() const;
	int buttonSpace() const;
	QRect buttonRect(ButtonId id) const;
	bool isMouseOverButton() const { return m_mouseOverButtonIndex >= 0; }

protected:
	qsizetype buttonCount() const;
	QRect buttonRect(int ix) const;

	void drawButton(QPainter *painter, const QRect &rect, int button_index);

	Graph *graph() const;
private:
	QVector<ButtonId> m_buttonIds;
	QRect m_rect;
	bool m_mouseOver = false;
	int m_mouseOverButtonIndex = -1;
	int m_mousePressButtonIndex = -1;
};

} // namespace shv::visu::timeline
