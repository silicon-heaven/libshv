#include <shv/iotqt/utils.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QCryptographicHash>
#include <QVariant>

namespace shv::iotqt::utils {
std::string sha1Hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
	hash.addData(QByteArrayView(s.data(), static_cast<int>(s.length())));
#else
	hash.addData(s.data(), static_cast<int>(s.length()));
#endif

	return std::string(hash.result().toHex().constData());
}
}
