#include <shv/coreqt/utils.h>
#include <shv/coreqt/rpc.h>

#include <shv/chainpack/rpcvalue.h>

#include <shv/core/log.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringView>
#include <QVariant>
#include <QDateTime>

namespace shv::coreqt {

QVariant Utils::rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok)
{
	return rpc::rpcValueToQVariant(v, ok);
}

chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v, bool *ok)
{
	return rpc::qVariantToRpcValue(v, ok);
}

QStringList Utils::rpcValueToStringList(const chainpack::RpcValue &rpcval)
{
	return rpc::rpcValueToStringList(rpcval);
}

chainpack::RpcValue Utils::stringListToRpcValue(const QStringList &sl)
{
	return rpc::stringListToRpcValue(sl);
}

std::string utils::joinPath(const std::string& p1, const std::string& p2)
{
	return core::utils::joinPath(p1, p2);
}

QString utils::joinPath(const QString &p1, const QString &p2)
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
	case QMetaType::UInt:
	case QMetaType::Char: return val.toInt() == 0;
	case QMetaType::QString: return val.toString().isEmpty();
	case QMetaType::QDateTime: return !val.toDateTime().isValid();
	default:
		return false;
	}
}

namespace {
uint32_t crc32_checksum(const uint8_t *data, int size)
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
}

std::vector<uint8_t> Utils::compressGZip(const std::vector<uint8_t> &data)
{
	QByteArray compressed_data = qCompress(QByteArray::fromRawData(reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size())));

	// Remove 4 bytes of length added by Qt a 2 bytes of zlib header from the beginning
	compressed_data = compressed_data.mid(6);

	// Remove 4 bytes of ADLER-32 zlib checksum from the end
	compressed_data.chop(4);

	// GZIP header according to GZIP File Format Specification (RFC 1952)
	static const std::array gzip_header = {'\x1f', '\x8b', '\x08', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03'};
	compressed_data.prepend(gzip_header.data(), gzip_header.size());

	const uint32_t crc32 = crc32_checksum(data.data(), static_cast<int>(data.size()));
	compressed_data.append(static_cast<char>(crc32 & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 8) & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 16) & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 24) & 0xff));

	const auto data_size = static_cast<uint32_t>(data.size());
	compressed_data.append(static_cast<char>(data_size & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 8) & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 16) & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 24) & 0xff));

	return shv::chainpack::RpcValue::Blob(compressed_data.cbegin(), compressed_data.cend());
}

QJsonValue utils::rpcValueToJson(const shv::chainpack::RpcValue& v)
{
	switch (v.type()) {
	case shv::chainpack::RpcValue::Type::Invalid:
	case shv::chainpack::RpcValue::Type::Null:
		return QJsonValue::Null;
	case shv::chainpack::RpcValue::Type::UInt:
	case shv::chainpack::RpcValue::Type::Int:
		return QJsonValue(v.toInt());
	case shv::chainpack::RpcValue::Type::Double:
	case shv::chainpack::RpcValue::Type::Decimal:
		return QJsonValue(v.toDouble());
	case shv::chainpack::RpcValue::Type::Bool:
		return QJsonValue(v.toBool());
	case shv::chainpack::RpcValue::Type::String: {
		return QJsonValue(QString::fromStdString(v.asString()));
	}
	case shv::chainpack::RpcValue::Type::Blob: {
		const auto &blob = v.asBlob();
		return QJsonValue(QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(blob.data()), static_cast<qsizetype>(blob.size())).toBase64()));
	}
	case shv::chainpack::RpcValue::Type::DateTime: {
		return QJsonValue(QString::fromStdString(v.toDateTime().toIsoString(shv::chainpack::RpcValue::DateTime::MsecPolicy::Always, true)));
	}
	case shv::chainpack::RpcValue::Type::List: {
		QJsonArray arr;
		const auto& list = v.asList();
		std::transform(list.cbegin(), list.cend(), std::back_inserter(arr), [] (const shv::chainpack::RpcValue& elem) { return rpcValueToJson(elem); });
		return arr;
	}
	case shv::chainpack::RpcValue::Type::Map: {
		QJsonObject object;

		for (const auto& [key, val] : v.asMap()) {
			object[QString::fromStdString(key)] = rpcValueToJson(val);
		}
		return object;
	}
	case shv::chainpack::RpcValue::Type::IMap: {
		QJsonObject object;
		for (const auto& [key, val] : v.asIMap()) {
			object[QString::number(key)] = rpcValueToJson(val);
		}
		return object;
	}
	}
	__builtin_unreachable();
}

QByteArray utils::jsonValueToByteArray(const QJsonValue& json)
{
	switch (json.type()) {
	case QJsonValue::Object:
		return QJsonDocument(json.toObject()).toJson();
	case QJsonValue::Array:
		return QJsonDocument(json.toArray()).toJson();
	case QJsonValue::Null:
		return QByteArray("null");
	case QJsonValue::Bool:
		return QByteArray(json.toBool() ? "true" : "false");
	case QJsonValue::Double:
		return QByteArray::number(json.toDouble());
	case QJsonValue::String:
		return ('"' + json.toString() + '"').toLatin1();
	case QJsonValue::Undefined:
		return QByteArray("undefined");
	}
	__builtin_unreachable();
}

[[noreturn]] void utils::qcoro_unhandled_exception(std::exception& ex)
{
	shvError() << "A coroutine has thrown an unhandled exception. The program will abort now.\n\n"
		<< "Exception was:" << ex.what();
	std::terminate();
}

} // namespace shv
