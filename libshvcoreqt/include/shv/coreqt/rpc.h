#pragma once

#include <shv/coreqt/shvcoreqtglobal.h>

#include <shv/chainpack/rpcvalue.h>

#include <QDateTime>
#include <QMetaType>

namespace shv {
namespace coreqt {
namespace rpc {

SHVCOREQT_DECL_EXPORT void registerQtMetaTypes();
SHVCOREQT_DECL_EXPORT QVariant rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok = nullptr);
SHVCOREQT_DECL_EXPORT chainpack::RpcValue qVariantToRpcValue(const QVariant &v, bool *ok = nullptr);
SHVCOREQT_DECL_EXPORT QStringList rpcValueToStringList(const shv::chainpack::RpcValue &rpcval);
SHVCOREQT_DECL_EXPORT shv::chainpack::RpcValue stringListToRpcValue(const QStringList &sl);

}}}

template<> SHVCOREQT_DECL_EXPORT QString shv::chainpack::RpcValue::to<QString>() const;
template<> SHVCOREQT_DECL_EXPORT QDateTime shv::chainpack::RpcValue::to<QDateTime>() const;

namespace shv {
namespace chainpack {

template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QString>(const QString &s) { return s.toStdString(); }
template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QDateTime>(const QDateTime &d)
{
	if (d.isValid()) {
		shv::chainpack::RpcValue::DateTime dt = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(d.toUTC().toMSecsSinceEpoch());
		int offset = d.offsetFromUtc();
		dt.setTimeZone(offset / 60);
		return dt;
	}
	else {
		return shv::chainpack::RpcValue();
	}
}

} // namespace chainack
} // namespace shv

Q_DECLARE_METATYPE(shv::chainpack::RpcValue)
