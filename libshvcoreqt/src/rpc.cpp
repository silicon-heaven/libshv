#include "rpc.h"

#include <shv/coreqt/log.h>

#include <QVariant>

namespace shv::coreqt::rpc {

void registerQtMetaTypes()
{
	if(!QMetaType::fromType<shv::chainpack::RpcValue>().isRegistered())
		qRegisterMetaType<shv::chainpack::RpcValue>();
}

QVariant rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok)
{
	if(ok)
		*ok = true;
	switch (v.type()) {
	case chainpack::RpcValue::Type::Invalid: return QVariant();
	case chainpack::RpcValue::Type::Null: return QVariant::fromValue(nullptr);
	case chainpack::RpcValue::Type::UInt: return QVariant(v.toUInt());
	case chainpack::RpcValue::Type::Int: return QVariant(v.toInt());
	case chainpack::RpcValue::Type::Double: return QVariant(v.toDouble());
	case chainpack::RpcValue::Type::Bool: return QVariant(v.toBool());
	case chainpack::RpcValue::Type::String: {
		const std::string &s = v.asString();
		return QVariant(QString::fromStdString(s));
	}
	case chainpack::RpcValue::Type::Blob: {
		const auto &blob = v.asBlob();
		return QVariant(QByteArray(reinterpret_cast<const char*>(blob.data()), static_cast<int>(blob.size())));
	}
	case chainpack::RpcValue::Type::DateTime: {
		chainpack::RpcValue::DateTime cdt = v.toDateTime();
		QDateTime dt = QDateTime::fromMSecsSinceEpoch(cdt.msecsSinceEpoch(), Qt::OffsetFromUTC, cdt.minutesFromUtc() * 60);
		return dt;
	}
	case chainpack::RpcValue::Type::List: {
		QVariantList lst;
		for(const auto &rv : v.asList())
			lst.insert(lst.size(), rpcValueToQVariant(rv));
		return lst;
	}
	case chainpack::RpcValue::Type::Map: {
		QVariantMap map;
		for(const auto &kv : v.asMap())
			map[QString::fromStdString(kv.first)] = rpcValueToQVariant(kv.second);
		return map;
	}
	case chainpack::RpcValue::Type::IMap: {
		std::map<int, shv::chainpack::RpcValue> map;
		for(const auto &[k, val] : v.asIMap())
			map[k] = val;
		auto ret = QVariant::fromValue(map);
		return ret;
	}
	case chainpack::RpcValue::Type::Decimal: return QVariant(v.toDouble());
	}
	if(ok) {
		*ok = false;
		return QVariant();
	}
	return QString::fromStdString(v.asString());
}

chainpack::RpcValue qVariantToRpcValue(const QVariant &v, bool *ok)
{
	if(ok)
		*ok = true;
	if(!v.isValid())
		return chainpack::RpcValue();
	if(v.isNull())
		return chainpack::RpcValue(nullptr);
	if(v.userType() == qMetaTypeId<std::map<int, shv::chainpack::RpcValue>>()) {
		chainpack::RpcValue::IMap ret;
		auto map = qvariant_cast<std::map<int, shv::chainpack::RpcValue>>(v);
		for(const auto &[k, val] : map)
			ret[k] = val;
		return ret;
	}
	switch (v.userType()) {
	case QMetaType::Nullptr: return chainpack::RpcValue(nullptr);
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt: return chainpack::RpcValue(v.toUInt());
	case QMetaType::ULongLong: return chainpack::RpcValue(static_cast<uint64_t>(v.toULongLong()));
	case QMetaType::Char:
	case QMetaType::SChar:
	case QMetaType::Short:
	case QMetaType::Int: return chainpack::RpcValue(v.toInt());
	case QMetaType::LongLong: return chainpack::RpcValue(static_cast<int64_t>(v.toLongLong()));
	case QMetaType::Float:
	case QMetaType::Double: return chainpack::RpcValue(v.toDouble());
	case QMetaType::Bool: return chainpack::RpcValue(v.toBool());
	case QMetaType::QString: return v.toString().toStdString();
	case QMetaType::QByteArray: {
		auto ba = v.toByteArray();
		auto *array = reinterpret_cast<const uint8_t*>(ba.constData());
		return chainpack::RpcValue::Blob(array, array + ba.size());
	}
	case QMetaType::QDateTime: {
		QDateTime qdt = v.toDateTime();
		chainpack::RpcValue dt;
		if (!qdt.isNull()) {
			chainpack::RpcValue::DateTime cdt = chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(qdt.toMSecsSinceEpoch());
			int offset = qdt.offsetFromUtc();
			cdt.setUtcOffsetMin(offset / 60);
			dt = cdt;
		}
		return dt;
	}
	case QMetaType::QStringList:
	case QMetaType::QVariantList: {
		chainpack::RpcValue::List lst;
		for(const QVariant &qv : v.toList()) {
			lst.push_back(qVariantToRpcValue(qv));
		}
		return chainpack::RpcValue{std::move(lst)};
	}
	case QMetaType::QVariantMap: {
		chainpack::RpcValue::Map map;
		const auto m = v.toMap();
		QMapIterator<QString, QVariant> it(m);
		while (it.hasNext()) {
			it.next();
			map[it.key().toStdString()] = qVariantToRpcValue(it.value());
		}
		return chainpack::RpcValue{std::move(map)};
	}
	default:
		if(ok) {
			*ok = false;
			shvWarning() << "Cannot convert QVariant type:" << v.userType() << v.typeName() << "to RpcValue";
			return chainpack::RpcValue();
		}
		return v.toString().toStdString();
	}
}

QStringList rpcValueToStringList(const shv::chainpack::RpcValue &rpcval)
{
	QStringList ret;
	for(const chainpack::RpcValue &v : rpcval.asList())
		ret << QString::fromStdString(v.asString());
	return ret;
}

shv::chainpack::RpcValue stringListToRpcValue(const QStringList &sl)
{
	shv::chainpack::RpcValue::List ret;
	for(const QString &s : sl)
		ret.push_back(s.toStdString());
	return shv::chainpack::RpcValue(ret);
}

} // namespace shv

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109503
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
template<> QString shv::chainpack::RpcValue::to<QString>() const
{
	return QString::fromStdString(asString());
}

template<> QDateTime shv::chainpack::RpcValue::to<QDateTime>() const
{
	if (!isValid() || !isDateTime()) {
		return QDateTime();
	}
	return QDateTime::fromMSecsSinceEpoch(toDateTime().msecsSinceEpoch(), Qt::TimeSpec::UTC);
}
#pragma GCC diagnostic pop
