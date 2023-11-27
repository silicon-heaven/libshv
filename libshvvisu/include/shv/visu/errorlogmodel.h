#pragma once

#include <shv/visu/logtablemodelbase.h>

#include <shv/visu/shvvisuglobal.h>

#include <necrologlevel.h>

namespace shv {
namespace visu {

class SHVVISU_DECL_EXPORT ErrorLogModel : public LogTableModelBase
{
	Q_OBJECT

	using Super = LogTableModelBase;
public:
	ErrorLogModel(QObject *parent = nullptr);

	enum Cols {Level, TimeStamp, Message, UserData, Count};

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	NecroLogLevel level(int row) const;

	void addLogRow(NecroLogLevel level, const std::string &msg, const QVariant &user_data = QVariant());

};

}}

