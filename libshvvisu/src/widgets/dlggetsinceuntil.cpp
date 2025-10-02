#include <shv/visu/widgets/dlggetsinceuntil.h>
#include <shv/visu/widgets/sinceuntildatetimeedit.h>
#include "ui_dlggetsinceuntil.h"

#include <QMenu>
#include <QTimeZone>
#include <shv/core/log.h>

namespace shv::visu::widgets {

static constexpr int64_t SEC = 1000;
static constexpr int64_t MIN = 60 * SEC;
static constexpr int64_t HOUR = 60 * MIN;
static constexpr int64_t DAY = 24 * HOUR;
static constexpr int64_t WEEK = 7 * DAY;

DlgGetSinceUntil::DlgGetSinceUntil(QWidget *parent)
	: Super(parent)
	, ui(new Ui::DlgGetSinceUntil)
{
	ui->setupUi(this);

	ui->cbxRecentValuesDuration->addItem(tr("last 10 minutes"), QVariant::fromValue(-10 * MIN));
	ui->cbxRecentValuesDuration->addItem(tr("last 30 minutes"), QVariant::fromValue(-30 * MIN));
	ui->cbxRecentValuesDuration->addItem(tr("last 1 hour"), QVariant::fromValue(-HOUR));
	ui->cbxRecentValuesDuration->addItem(tr("last 1 day"), QVariant::fromValue(-DAY));
	ui->cbxRecentValuesDuration->addItem(tr("last 2 days"), QVariant::fromValue(-2 * DAY));
	ui->cbxRecentValuesDuration->addItem(tr("last 7 days"), QVariant::fromValue(-WEEK));
	ui->cbxRecentValuesDuration->setCurrentIndex(2);

	ui->dteSince->setDateTime(QDateTime::currentDateTime());
	ui->dteUntil->setDateTime(ui->dteUntil->minimumDateTime());

	createSinceUntilMenu();

	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DlgGetSinceUntil::enableOk);
	connect(ui->dteUntil, &QDateTimeEdit::dateTimeChanged, this, &DlgGetSinceUntil::enableOk);
}

DlgGetSinceUntil::~DlgGetSinceUntil()
{
	delete ui;
}

#if QT_CONFIG(timezone)
QTimeZone DlgGetSinceUntil::timeZone() const
{
	return ui->cbxTimeZone->currentTimeZone();
}
#endif

void DlgGetSinceUntil::enableOk()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
				ui->tabWidget->currentWidget() == ui->tRecentValues ||
				ui->dteUntil->dateTime() > ui->dteSince->dateTime()
	);
}

qlonglong DlgGetSinceUntil::recentValuesDuration() const
{
	QVariant d = ui->cbxRecentValuesDuration->currentData();
	return d.toLongLong();
}

void DlgGetSinceUntil::adjustDateTime(QDateTimeEdit *from_dte, QDateTimeEdit *to_dte, int64_t msecs)
{
	if (from_dte->dateTime() == from_dte->minimumDateTime()) {
		from_dte->setDateTime(QDateTime::currentDateTime());
	}

	to_dte->setDateTime(from_dte->dateTime().addMSecs(msecs));
}

std::tuple<QDateTime, QDateTime> DlgGetSinceUntil::getSinceUntil() const
{
	QDateTime since;
	QDateTime until;

	if (ui->tabWidget->currentWidget() == ui->tRecentValues) {
		auto current_dt = QDateTime::currentDateTime();
#if QT_CONFIG(timezone)
		until = QDateTime(current_dt.date(), current_dt.time(), timeZone());
#else
		until = QDateTime(current_dt.date(), current_dt.time());
#endif
		since = until.addMSecs(recentValuesDuration());
	}

	if (ui->tabWidget->currentWidget() == ui->tTimeInterval) {
#if QT_CONFIG(timezone)
		since = QDateTime(ui->dteSince->date(),ui->dteSince->time(), timeZone());
		until = QDateTime(ui->dteUntil->date(),ui->dteUntil->time(), timeZone());
#else
		since = QDateTime(ui->dteSince->date(),ui->dteSince->time());
		until = QDateTime(ui->dteUntil->date(),ui->dteUntil->time());
#endif
	}

	return { since, until };
}

void DlgGetSinceUntil::createSinceUntilMenu()
{
	auto *since_menu = new QMenu(this);
	connect(since_menu->addAction(tr("since now")), &QAction::triggered, this, [this]() { ui->dteSince->setDateTime(QDateTime::currentDateTime()); });
	connect(since_menu->addAction(tr("10 minutes before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -10 * MIN); });
	connect(since_menu->addAction(tr("30 minutes before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -30 * MIN); });
	connect(since_menu->addAction(tr("1 hour before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -HOUR); });
	connect(since_menu->addAction(tr("2 hours before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -2 * HOUR); });
	connect(since_menu->addAction(tr("4 hours before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -4 * HOUR); });
	connect(since_menu->addAction(tr("8 hour before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -8 * HOUR); });
	connect(since_menu->addAction(tr("12 hours before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -12 * HOUR); });
	connect(since_menu->addAction(tr("1 day before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -DAY); });
	connect(since_menu->addAction(tr("2 days before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -2 * DAY); });
	connect(since_menu->addAction(tr("7 days before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -WEEK); });
	connect(since_menu->addAction(tr("30 days before until")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteUntil, ui->dteSince, -30 * DAY); });
	ui->pbSinceAction->setMenu(since_menu);

	auto *until_menu = new QMenu(this);
	connect(until_menu->addAction(tr("until now")), &QAction::triggered, this, [this]() { ui->dteUntil->setDateTime(QDateTime::currentDateTime()); });
	connect(until_menu->addAction(tr("10 minutes after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 10 * MIN); });
	connect(until_menu->addAction(tr("30 minutes after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 30 * MIN); });
	connect(until_menu->addAction(tr("1 hour after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, HOUR); });
	connect(until_menu->addAction(tr("2 hours after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 2 * HOUR); });
	connect(until_menu->addAction(tr("4 hours after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 4 * HOUR); });
	connect(until_menu->addAction(tr("8 hours after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 8 * HOUR); });
	connect(until_menu->addAction(tr("12 hours after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 12 * HOUR); });
	connect(until_menu->addAction(tr("1 day after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, DAY); });
	connect(until_menu->addAction(tr("2 days after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 2 * DAY); });
	connect(until_menu->addAction(tr("7 days after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, WEEK); });
	connect(until_menu->addAction(tr("30 days after since")), &QAction::triggered, this, [this]() { adjustDateTime(ui->dteSince, ui->dteUntil, 30 * DAY); });
	ui->pbUntilAction->setMenu(until_menu);
}

}
