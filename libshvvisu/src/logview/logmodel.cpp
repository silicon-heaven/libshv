#include "logmodel.h"

#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

namespace shv {
namespace visu {
namespace logview {

//============================================================
// MemoryJournalLogModel
//============================================================
LogModel::LogModel(QObject *parent)
	: Super(parent)
{

}

void LogModel::setTimeZone(const QTimeZone &tz)
{
	m_timeZone = tz;
	auto ix1 = index(0, ColDateTime);
	auto ix2 = index(rowCount() - 1, ColDateTime);
	emit dataChanged(ix1, ix2);
}

void LogModel::setLog(const shv::chainpack::RpcValue &log)
{
	beginResetModel();
	m_log = log;
	endResetModel();
}

int LogModel::rowCount(const QModelIndex &) const
{
	const shv::chainpack::RpcValue::List &lst = m_log.toList();
	return (int)lst.size();
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
			}
		}
	}
	return Super::headerData(section, orientation, role);
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid() && index.row() < rowCount()) {
		if(role == Qt::DisplayRole) {
			const shv::chainpack::RpcValue::List &lst = m_log.toList();
			shv::chainpack::RpcValue row = lst.value((unsigned)index.row());
			shv::chainpack::RpcValue val = row.toList().value((unsigned)index.column());
			if(index.column() == ColDateTime) {
				int64_t msec = val.toDateTime().msecsSinceEpoch();
				if(msec == 0)
					return QVariant();
				QDateTime dt = QDateTime::fromMSecsSinceEpoch(msec);
				dt = dt.toTimeZone(m_timeZone);
				return dt.toString(Qt::ISODateWithMs);
			}
			else if(index.column() == ColPath) {
				if ((val.type() == cp::RpcValue::Type::UInt) || (val.type() == cp::RpcValue::Type::Int)) {
					static std::string KEY_PATHS_DICT = shv::core::utils::ShvFileJournal::KEY_PATHS_DICT;
					const chainpack::RpcValue::IMap &dict = m_log.metaData().valref(KEY_PATHS_DICT).asIMap();
					auto it = dict.find(val.toInt());
					if(it != dict.end())
						val = it->second;
					return QString::fromStdString(val.asString());
				}
				return QString::fromStdString(val.asString());
			}
			else if(index.column() == ColValueFlags) {
				int t = val.toUInt();
				return QString::fromStdString(cp::DataChange::valueFlagsToString(t));
			}
			return QString::fromStdString(val.toCpon());
		}
	}
	return QVariant();
}

}}}
