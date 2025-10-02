#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QDateTimeEdit>

namespace shv::visu::widgets {

class SHVVISU_DECL_EXPORT SinceUntilDateTimeEdit : public QDateTimeEdit
{
	Q_OBJECT
	using Super = QDateTimeEdit;

public:
	explicit SinceUntilDateTimeEdit(QWidget *parent);
	~SinceUntilDateTimeEdit() override = default;

protected:
	void mousePressEvent(QMouseEvent *mouse_event) override;
	void focusInEvent(QFocusEvent *event) override;
};

}
