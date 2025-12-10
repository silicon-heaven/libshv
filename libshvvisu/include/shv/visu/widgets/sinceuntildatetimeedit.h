#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QDateTimeEdit>

namespace shv::visu::widgets {

class LIBSHVVISU_EXPORT SinceUntilDateTimeEdit : public QDateTimeEdit
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
