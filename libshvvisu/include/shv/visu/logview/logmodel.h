#pragma once

#include "../shvvisuglobal.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QAbstractTableModel>
#if SHVVISU_HAS_TIMEZONE
#include <QTimeZone>
#endif

namespace shv::visu::logview {

class SHVVISU_DECL_EXPORT LogModel : public QAbstractTableModel
{
	Q_OBJECT

	using Super = QAbstractTableModel;
public:
	enum {ColDateTime = 0, ColPath, ColValue, ColShortTime, ColDomain, ColValueFlags, ColUserId,  ColCnt};
public:
	LogModel(QObject *parent = nullptr);
#if SHVVISU_HAS_TIMEZONE
	void setTimeZone(const QTimeZone &tz);
#endif

	void setLog(const shv::chainpack::RpcValue &log);
	shv::chainpack::RpcValue log() const;

	int rowCount(const QModelIndex & = QModelIndex()) const override;
	int columnCount(const QModelIndex & = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QModelIndex &index, int role) const override;
protected:
	shv::chainpack::RpcValue m_log;
#if SHVVISU_HAS_TIMEZONE
	QTimeZone m_timeZone;
#endif
};
}
