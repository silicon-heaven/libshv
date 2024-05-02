#include <shv/visu/logview/logmodel.h>

#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/log.h>

#include <QDateTime>

namespace cp = shv::chainpack;

namespace shv::visu::logview {

//============================================================
// MemoryJournalLogModel
//============================================================
LogModel::LogModel(QObject *parent)
	: Super(parent)
{

}

#if SHVVISU_HAS_TIMEZONE
void LogModel::setTimeZone(const QTimeZone &tz)
{
	m_timeZone = tz;
	auto ix1 = index(0, ColDateTime);
	auto ix2 = index(rowCount() - 1, ColDateTime);
	emit dataChanged(ix1, ix2);
}
#endif

void LogModel::setLog(const shv::chainpack::RpcValue &log)
{
	beginResetModel();
	m_log = log;
	endResetModel();
}

shv::chainpack::RpcValue LogModel::log() const
{
	return m_log;
}

int LogModel::rowCount(const QModelIndex &) const
{
	const shv::chainpack::RpcValue::List &lst = m_log.asList();
	return static_cast<int>(lst.size());
}

int LogModel::columnCount(const QModelIndex&) const
{
	return ColCnt;
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section) {
			case ColDateTime: return tr("DateTime");
			case ColPath: return tr("Path");
			case ColValue: return tr("Value");
			case ColShortTime: return tr("ShortTime");
			case ColDomain: return tr("Domain");
			case ColValueFlags: return tr("ValueFlags");
			case ColUserId: return tr("UserId");
			default: break;
			}
		}
	}
	return Super::headerData(section, orientation, role);
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid() && index.row() < rowCount()) {
		if(role == Qt::DisplayRole) {
			const shv::chainpack::RpcValue::List &lst = m_log.asList();
			shv::chainpack::RpcValue row = lst.value(static_cast<unsigned>(index.row()));
			shv::chainpack::RpcValue val = row.asList().value(static_cast<unsigned>(index.column()));
			if(index.column() == ColDateTime) {
				int64_t msec = val.toDateTime().msecsSinceEpoch();
				if(msec == 0)
					return QVariant();
				QDateTime dt = QDateTime::fromMSecsSinceEpoch(msec);
#if SHVVISU_HAS_TIMEZONE
				dt = dt.toTimeZone(m_timeZone);
#endif
				return dt.toString(Qt::ISODateWithMs);
			}
			if(index.column() == ColPath) {
				if ((val.type() == cp::RpcValue::Type::UInt) || (val.type() == cp::RpcValue::Type::Int)) {
					const chainpack::RpcValue::IMap &dict = m_log.metaData().valref(core::utils::ShvJournalCommon::KEY_PATHS_DICT).asIMap();
					auto it = dict.find(val.toInt());
					if(it != dict.end())
						val = it->second;
					return QString::fromStdString(val.asString());
				}
				return QString::fromStdString(val.asString());
			}
			if(index.column() == ColValueFlags) {
				return QString::fromStdString(cp::DataChange::valueFlagsToString(val.toUInt()));
			}
			return QString::fromStdString(val.toCpon());
		}
	}
	return QVariant();
}

}
