#pragma once

#include "shvcoreqtglobal.h"

#include <shv/core/utils.h>
#include <shv/core/exception.h>

#include <QMetaType>
#include <QObject>

#include <string>

#ifdef LIBC_NEWLIB
#include <sstream>
#endif

#define SHV_QUOTE_QSTRINGLITERAL(x) QStringLiteral(#x)

#define SHV_PROPERTY_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	private: ptype m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		} \
		return false; \
	}

#define SHV_PROPERTY_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	private: ptype m_##lower_letter##name_rest; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		} \
		return false; \
	}

#define SHV_PROPERTY(ptype, lower_letter, upper_letter, name_rest) \
	Q_PROPERTY(ptype lower_letter##name_rest READ lower_letter##name_rest WRITE set##upper_letter##name_rest NOTIFY lower_letter##name_rest##Changed) \
	SHV_PROPERTY_IMPL(ptype, lower_letter, upper_letter, name_rest)

#define SHV_PROPERTY2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	Q_PROPERTY(ptype lower_letter##name_rest READ lower_letter##name_rest WRITE set##upper_letter##name_rest NOTIFY lower_letter##name_rest##Changed) \
	SHV_PROPERTY_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value)
/*
#define SHV_PROPERTY_BOOL2(name, default_value) \
	private: bool m_is##name = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const bool &new_val); \
	public: bool is##name() const {return m_is##name;} \
	public: Q_SLOT bool set##name(bool val) { \
		if(m_is##name != val) { \
			m_is##name = val; \
			emit is##name##Changed(m_is##name); \
			return true; \
		}\
		return false; \
	}

#define SHV_PROPERTY_BOOL(name) \
	SHV_PROPERTY_BOOL2(name, false)
*/
#define SHV_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	private: bool m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const bool &new_val); \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT bool set##upper_letter##name_rest(bool val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
			return true; \
		}\
		return false; \
	}

#define SHV_PROPERTY_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)

#define SHV_PROPERTY_BOOL(lower_letter, upper_letter, name_rest) \
	Q_PROPERTY(bool lower_letter##name_rest READ is##upper_letter##name_rest WRITE set##upper_letter##name_rest NOTIFY lower_letter##name_rest##Changed) \
	SHV_PROPERTY_BOOL_IMPL(lower_letter, upper_letter, name_rest)

#define SHV_PROPERTY_BOOL2(lower_letter, upper_letter, name_rest, default_value) \
	Q_PROPERTY(bool lower_letter##name_rest READ is##upper_letter##name_rest WRITE set##upper_letter##name_rest NOTIFY lower_letter##name_rest##Changed) \
	SHV_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value)

#define SHV_VARIANTMAP_FIELD(ptype, getter_prefix, setter_prefix, name_rest) \
	public: bool getter_prefix##name_rest##_isset() const {return contains(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest));} \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)] = QVariant::fromValue(val);}
/// for default values other than QVariant()
#define SHV_VARIANTMAP_FIELD2(ptype, getter_prefix, setter_prefix, name_rest, default_value) \
	public: bool getter_prefix##name_rest##_isset() const {return contains(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest));} \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest), default_value));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTE_QSTRINGLITERAL(getter_prefix##name_rest)] = QVariant::fromValue(val);}
	//since c++14 public: auto& setter_prefix##name_rest(const ptype &val) {(*this)[SHV_QUOTEME(getter_prefix##name_rest)] = val; return *this;}

/// for implicitly shared classes properties
#define SHV_SHARED_CLASS_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: const ptype& getter_prefix##name_rest() const {return d->getter_prefix##name_rest;} \
	public: void setter_prefix##name_rest(const ptype &val) {d->getter_prefix##name_rest = val;}

/// for implicitly shared classes properties
#define SHV_SHARED_CLASS_BIT_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const {return d->getter_prefix##name_rest;} \
	public: void setter_prefix##name_rest(const ptype &val) {d->getter_prefix##name_rest = val;}

class QVariant;
#if QT_VERSION_MAJOR < 6
class QStringList;
#endif

namespace shv {
namespace chainpack { class RpcValue; }
namespace coreqt {

class SHVCOREQT_DECL_EXPORT Utils
{
public:
	static bool isDefaultQVariantValue(const QVariant &val);
	static bool isValueNotAvailable(const QVariant &val);
	static QVariant rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok = nullptr);
	static chainpack::RpcValue qVariantToRpcValue(const QVariant &v, bool *ok = nullptr);
	static QStringList rpcValueToStringList(const shv::chainpack::RpcValue &rpcval);
	static shv::chainpack::RpcValue stringListToRpcValue(const QStringList &sl);
	static QString joinPath(const QString &p1, const QString &p2);
	static QString joinPath(const QString &p1, const QString &p2, const QString &p3);

	static std::vector<uint8_t> compressGZip(const std::vector<uint8_t> &data);

	template <class T>
	static T findParent(const QObject *_o, bool throw_exc = shv::core::Exception::Throw)
	{
		T t = nullptr;
		QObject *o = const_cast<QObject*>(_o);
		while(o) {
			o = o->parent();
			if(!o)
				break;
			t = qobject_cast<T>(o);
			if(t)
				break;
		}
		if(!t && throw_exc) {
			SHV_EXCEPTION("Object has not any parent of requested type.");
		}
		return t;
	}
};

} // namespace coreqt
} // namespace shv

