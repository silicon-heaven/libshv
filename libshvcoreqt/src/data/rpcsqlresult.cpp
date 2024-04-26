#include <shv/coreqt/data/rpcsqlresult.h>

#include <shv/coreqt/log.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/coreqt/rpc.h>

#include <QDateTime>
//#include <QSqlQuery>
//#include <QSqlRecord>
//#include <QSqlField>

using namespace shv::chainpack;

namespace shv::coreqt::data {

RpcValue RpcSqlField::toRpcValue() const
{
	RpcValue::Map ret;
	ret["name"] = name.toStdString();
	ret["type"] = type;
	ret["typeName"] = QMetaType(type).name();
	return RpcValue(std::move(ret));
}

QVariant RpcSqlField::toVariant() const
{
	QVariantMap ret;
	ret["name"] = name;
	ret["type"] = type;
	ret["typeName"] = QMetaType(type).name();
	return ret;
}

RpcSqlField RpcSqlField::fromRpcValue(const shv::chainpack::RpcValue &rv)
{
	RpcSqlField ret;
	const RpcValue::Map &map = rv.asMap();
	ret.name = QString::fromStdString(map.value("name").asString());
	ret.type = map.value("type").toInt();
	return ret;
}

RpcSqlField RpcSqlField::fromVariant(const QVariant &v)
{
	RpcSqlField ret;
	const QVariantMap map = v.toMap();
	ret.name = map.value("name").toString();
	ret.type = map.value("type").toInt();
	return ret;
}

RpcSqlResult::RpcSqlResult(const shv::chainpack::RpcResponse &resp)
{
	if(resp.isSuccess()) {
		const RpcValue::Map result = resp.result().asMap();
		for(const RpcValue &rv : result.valref("fields").asList()) {
			RpcSqlField fld;
			fld.name = QString::fromStdString(rv.asMap().value("name").toString());
			fld.type = rv.asMap().value("type").toInt();
			fields.append(fld);
		}
		for(const RpcValue &rowv : result.valref("rows").asList()) {
			Row row;
			for(const RpcValue &rv : rowv.asList()) {
				bool ok;
				QVariant v = shv::coreqt::rpc::rpcValueToQVariant(rv, &ok);
				if (!ok) {
					shvError() << "Cannot convert:" << rv.toCpon() << "to QVariant";
				}
				row.append(v);
			}
			rows.insert(rows.count(), row);
		}
		numRowsAffected = result.value("numRowsAffected").toInt();
		lastInsertId = result.value("lastInserId").toInt();
	}
	else {
		lastError = QString::fromStdString(resp.errorString());
	}
}

QVariant RpcSqlResult::value(int row, int col) const
{
	return rows.value(row).toList().value(col);
}

RpcValue RpcSqlResult::toRpcValue() const
{
	return shv::coreqt::rpc::qVariantToRpcValue(toVariant());
}

QVariant RpcSqlResult::toVariant() const
{
	QVariantMap ret;
	if(isSelect()) {
		QVariantList flds;
		for(const auto &fld : this->fields)
			flds.push_back(fld.toVariant());
		ret["fields"] = flds;
		ret["rows"] = rows;
	}
	else {
		ret["numRowsAffected"] = numRowsAffected;
		ret["lastInsertId"] = lastInsertId;
	}
	return ret;
}

RpcSqlResult RpcSqlResult::fromVariant(const QVariant &v)
{
	RpcSqlResult ret;
	const QVariantMap map = v.toMap();
	const QVariantList flds = map.value("fields").toList();
	if(flds.isEmpty()) {
		ret.numRowsAffected = map.value("numRowsAffected").toInt();
		ret.lastInsertId = map.value("lastInsertId").toInt();
	}
	else {
		for(const QVariant &fv : flds)
			ret.fields.append(RpcSqlField::fromVariant(fv));
		ret.rows = map.value("rows").toList();
	}
	return ret;
}

RpcSqlResult RpcSqlResult::fromRpcValue(const shv::chainpack::RpcValue &rv)
{
	auto v = shv::coreqt::rpc::rpcValueToQVariant(rv);
	return fromVariant(v);
}

}
