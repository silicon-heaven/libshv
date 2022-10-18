#include "utils.h"

#include <shv/chainpack/rpcvalue.h>

#include <shv/core/log.h>

#include <QStringView>
#include <QVariant>
#include <QDateTime>

namespace shv {
namespace coreqt {

QVariant Utils::rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok)
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
		return QVariant(QByteArray((char*)blob.data(), blob.size()));
	}
	case chainpack::RpcValue::Type::DateTime: {
		chainpack::RpcValue::DateTime cdt = v.toDateTime();
		QDateTime dt = QDateTime::fromMSecsSinceEpoch(cdt.msecsSinceEpoch(), Qt::OffsetFromUTC, cdt.minutesFromUtc() * 60);
		return dt;
	}
	case chainpack::RpcValue::Type::List: {
		QVariantList lst;
		for(const auto &rv : v.toList())
			lst.insert(lst.size(), rpcValueToQVariant(rv));
		return lst;
	}
	case chainpack::RpcValue::Type::Map: {
		QVariantMap map;
		for(const auto &kv : v.toMap())
			map[QString::fromStdString(kv.first)] = rpcValueToQVariant(kv.second);
		return map;
	}
	case chainpack::RpcValue::Type::IMap: {
		QVariantMap map;
		for(const auto &kv : v.toIMap())
			map[QString::number(kv.first)] = rpcValueToQVariant(kv.second);
		return map;
	}
	case chainpack::RpcValue::Type::Decimal: return QVariant(v.toDouble());
	}
	if(ok) {
		*ok = false;
		return QVariant();
	}
	return QString::fromStdString(v.asString());
}

chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v, bool *ok)
{
	if(ok)
		*ok = true;
	if(!v.isValid())
		return chainpack::RpcValue();
	if(v.isNull())
		return chainpack::RpcValue(nullptr);
	switch (v.userType()) {
	case QMetaType::Nullptr: return chainpack::RpcValue(nullptr);
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt: return chainpack::RpcValue(v.toUInt());
	case QMetaType::ULongLong: return chainpack::RpcValue(static_cast<uint64_t>(v.toULongLong()));
	case QMetaType::Char:
	case QMetaType::Short:
	case QMetaType::Int: return chainpack::RpcValue(v.toInt());
	case QMetaType::LongLong: return chainpack::RpcValue(static_cast<int64_t>(v.toLongLong()));
	case QMetaType::Float:
	case QMetaType::Double: return chainpack::RpcValue(v.toDouble());
	case QMetaType::Bool: return chainpack::RpcValue(v.toBool());
	case QMetaType::QString: return v.toString().toStdString();
	case QMetaType::QByteArray: {
		auto ba = v.toByteArray();
		auto *array = (const uint8_t*)ba.constData();
		return chainpack::RpcValue::Blob(array, array + ba.size());
	}
	case QMetaType::QDateTime: {
		QDateTime qdt = v.toDateTime();
		chainpack::RpcValue::DateTime cdt = chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(qdt.toMSecsSinceEpoch());
		int offset = qdt.offsetFromUtc();
		cdt.setUtcOffsetMin(offset / 60);
		return cdt;
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
		QMapIterator<QString, QVariant> it(v.toMap());
		while (it.hasNext()) {
			it.next();
			map[it.key().toStdString()] = qVariantToRpcValue(it.value());
		}
		return chainpack::RpcValue{std::move(map)};
	}
	default:
		if(ok) {
			*ok = false;
			shvWarning() << "Cannot convert QVariant type:" << v.typeName() << "to RpcValue";
			return chainpack::RpcValue();
		}
		return v.toString().toStdString();
	}
}

QStringList Utils::rpcValueToStringList(const shv::chainpack::RpcValue &rpcval)
{
	QStringList ret;
	for(const chainpack::RpcValue &v : rpcval.toList())
		ret << QString::fromStdString(v.asString());
	return ret;
}

shv::chainpack::RpcValue Utils::stringListToRpcValue(const QStringList &sl)
{
	shv::chainpack::RpcValue::List ret;
	for(const QString &s : sl)
		ret.push_back(s.toStdString());
	return shv::chainpack::RpcValue(ret);
}

QString Utils::joinPath(const QString &p1, const QString &p2)
{
	QStringView sv1(p1);
	while(!sv1.isEmpty() && sv1.last() == '/')
		sv1.chop(1);
	QStringView sv2(p2);
	while(!sv2.isEmpty() && sv2.first() == '/')
		sv2 = sv2.mid(1);
	if(sv2.isEmpty())
		return sv1.toString();
	if(sv1.isEmpty())
		return sv2.toString();
	return sv1.toString() + '/' + sv2.toString();
}

QString Utils::joinPath(const QString &p1, const QString &p2, const QString &p3)
{
	return joinPath(joinPath(p1, p2), p3);
}

bool Utils::isValueNotAvailable(const QVariant &val)
{
	return !val.isValid();
}

bool Utils::isDefaultQVariantValue(const QVariant &val)
{
	if(!val.isValid())
		return true;
	if(val.isNull())
		return true;
	switch(val.userType()) {
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
	case QMetaType::Short:
	case QMetaType::Long:
	case QMetaType::ULong:
	case QMetaType::Int:
	case QMetaType::UInt: return val.toInt() == 0;
	case QMetaType::Char: return val.toInt() == 0;
	case QMetaType::QString: return val.toString().isEmpty();
	case QMetaType::QDateTime: return !val.toDateTime().isValid();
	default:
		return false;
	}
}

static uint32_t crc32_checksum(const uint8_t *data, int size)
{
	uint32_t crc = 0xFFFFFFFF;

	for (int i = 0; i < size; i++) {
		uint8_t byte = data[i];
		for (int j = 0; j < 8; j++) {
			uint32_t bit = (byte ^ crc) & 1;
			crc >>= 1;
			if (bit)
				crc = crc ^ 0xEDB88320;
			byte >>= 1;
		}
	}

	return ~crc;
}

std::vector<uint8_t> Utils::compressGZip(const std::vector<uint8_t> &data)
{
	QByteArray compressed_data = qCompress(QByteArray::fromRawData(reinterpret_cast<const char *>(data.data()), data.size()));

	// Remove 4 bytes of length added by Qt a 2 bytes of zlib header from the beginning
	compressed_data = compressed_data.mid(6);

	// Remove 4 bytes of ADLER-32 zlib checksum from the end
	compressed_data.chop(4);

	// GZIP header according to GZIP File Format Specification (RFC 1952)
	static const char gzip_header[] = {'\x1f', '\x8b', '\x08', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03'};
	compressed_data.prepend(gzip_header, sizeof(gzip_header));

	const uint32_t crc32 = crc32_checksum(data.data(), data.size());
	compressed_data.append(crc32 & 0xff);
	compressed_data.append((crc32 >> 8) & 0xff);
	compressed_data.append((crc32 >> 16) & 0xff);
	compressed_data.append((crc32 >> 24) & 0xff);

	const uint32_t data_size = data.size();
	compressed_data.append(data_size & 0xff);
	compressed_data.append((data_size >> 8) & 0xff);
	compressed_data.append((data_size >> 16) & 0xff);
	compressed_data.append((data_size >> 24) & 0xff);

	return shv::chainpack::RpcValue::Blob(compressed_data.cbegin(), compressed_data.cend());
}

} // namespace coreqt
} // namespace shv
