#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <shv/visu/logtablemodelbase.h>

#include <QTableView>

class QAbstractButton;

namespace shv {
namespace visu {

class LogFilterProxyModel;
class LogTableModelRow;
class LogTableModelBase;

namespace Ui {
class LogWidget;
}

class LogWidgetTableView : public QTableView
{
	Q_OBJECT
private:
	typedef QTableView Super;
public:
	LogWidgetTableView(QWidget *parent);

	void copy();
protected:
	void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
};

class SHVVISU_DECL_EXPORT LogWidget : public QWidget
{
	Q_OBJECT
private:
	typedef QWidget Super;
public:
	explicit LogWidget(QWidget *parent = nullptr);
	~LogWidget();

	Q_SLOT void addLogRow(const LogTableModelRow &row);
	Q_SLOT void scrollToLastRow();

	void clear();
	virtual void setLogTableModel(LogTableModelBase *m);
	LogTableModelBase* logTableModel();
	QTableView* tableView() const;

protected:
	QAbstractButton* tableMenuButton();

	void onVerticalScrollBarValueChanged();
private:
	Q_SLOT void filterStringChanged(const QString &filter_string);
	Q_SLOT void btClearLog_clicked();
	Q_SLOT void btResizeColumns_clicked();
protected:
	LogTableModelBase* m_logTableModel = nullptr;
	LogFilterProxyModel* m_filterModel = nullptr;
private:
	Ui::LogWidget *ui;
	bool m_columnsResized = false;
	bool m_isAutoScroll = true;
};

}}

