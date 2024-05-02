#include <shv/visu/logtablemodelbase.h>

#include <QColor>
#include <QDateTime>

namespace shv::visu {

LogTableModelRow::LogTableModelRow(int size)
{
	resize(size);
}

LogTableModelBase::LogTableModelBase(QObject *parent)
	: Super(parent)
{
}

int LogTableModelBase::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
#if QT_VERSION_MAJOR >= 6
	return static_cast<int>(m_rows.count());
#else
	return m_rows.count();
#endif
}

QVariant LogTableModelBase::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) {
		return QVariant();
	}
	switch (role) {
	case Qt::DisplayRole: {
		QVariant ret = data(index, Qt::EditRole);
		return ret;
	}
	case Qt::EditRole:
		return m_rows[index.row()].value(index.column());
	case Qt::ForegroundRole:
	case Qt::BackgroundRole: {
		break;
	}
	case Qt::ToolTipRole: {
		QVariant ret = data(index, Qt::DisplayRole);
		return ret;
	}
	default:
		break;
	}
	return QVariant();
}

void LogTableModelBase::clear()
{
	beginResetModel();
	m_rows.clear();
	endResetModel();
}

LogTableModelRow LogTableModelBase::rowAt(int row) const
{
	return m_rows.value(row);
}

void LogTableModelBase::addLogRow(const LogTableModelRow &row)
{
	static constexpr int ROWS_OVERLAP = 100;
	if(rowCount() >= maximumRowCount() + ROWS_OVERLAP) {
		if(direction() == Direction::AppendToBottom) {
			beginRemoveRows(QModelIndex(), 0, ROWS_OVERLAP - 1);
			m_rows = m_rows.mid(ROWS_OVERLAP);
			endRemoveRows();
		}
		else {
			beginRemoveRows(QModelIndex(), rowCount() - ROWS_OVERLAP, rowCount() - 1);
			m_rows = m_rows.mid(0, rowCount() - ROWS_OVERLAP);
			endRemoveRows();
		}
	}
	if(direction() == Direction::AppendToBottom) {
		beginInsertRows(QModelIndex(), rowCount(), rowCount());
		m_rows.append(row);
		endInsertRows();
		emit logRowInserted(rowCount() - 1);
	}
	else {
		beginInsertRows(QModelIndex(), 0, 0);
		m_rows.insert(0, row);
		endInsertRows();
		emit logRowInserted(0);
	}
}

}
