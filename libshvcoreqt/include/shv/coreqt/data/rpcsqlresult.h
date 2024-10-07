#pragma once

#include <shv/coreqt/shvcoreqtglobal.h>

#include <shv/chainpack/rpcvalue.h>

#include <QVector>
#include <QVariantList>

#include <optional>

namespace shv::chainpack { class RpcResponse; }

//class QSqlQuery;
//class QSqlRecord;

namespace shv::coreqt::data {

class SHVCOREQT_DECL_EXPORT RpcSqlField
{
public:
	QString name;
	int type = 0;
public:
	//explicit RpcSqlField(const QJsonObject &jo = QJsonObject()) : Super(jo) {}
	shv::chainpack::RpcValue toRpcValue() const;
	QVariant toVariant() const;
	static RpcSqlField fromRpcValue(const shv::chainpack::RpcValue &rv);
	static RpcSqlField fromVariant(const QVariant &v);
};

class SHVCOREQT_DECL_EXPORT RpcSqlResult
{
public:
	int numRowsAffected = 0;
	int lastInsertId = 0;
	QString lastError;
	QVector<RpcSqlField> fields;
	using Row = QVariantList;
	QVariantList rows;
public:
	explicit RpcSqlResult() = default;
	explicit RpcSqlResult(const shv::chainpack::RpcResponse &resp);

	std::optional<qsizetype> columnIndex(const QString &name) const;
	QVariant value(qsizetype row, qsizetype col) const;
	QVariant value(qsizetype row, const QString &name) const;
	void setValue(qsizetype row, qsizetype col, const QVariant &val);
	void setValue(qsizetype row, const QString &name, const QVariant &val);

	bool isSelect() const {return !fields.isEmpty();}
	shv::chainpack::RpcValue toRpcValue() const;
	QVariant toVariant() const;
	static RpcSqlResult fromVariant(const QVariant &v);
	static RpcSqlResult fromRpcValue(const shv::chainpack::RpcValue &rv);
};

}
