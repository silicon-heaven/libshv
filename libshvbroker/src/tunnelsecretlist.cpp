#include <shv/broker/tunnelsecretlist.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QtGlobal>

#if QT_VERSION >= 0x050A00
	#define HAVE_QT_RANDOM_GENERATOR
#endif

#ifdef HAVE_QT_RANDOM_GENERATOR
#include <QRandomGenerator>
#endif

#include <array>

namespace shv::broker {

static constexpr int64_t max_age_msec = 10*1000;

bool TunnelSecretList::checkSecret(const std::string &s)
{
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	for (auto& sc : m_secretList) {
		int64_t age = now - sc.createdMsec;
		if(age < 0)
			continue; // this should never happen
		if(age > max_age_msec)
			continue;
		if(sc.secret == s) {
			sc.hitCnt++;
			if(sc.hitCnt <= 2)
				return true;
		}
	}
	return false;
}

std::string TunnelSecretList::createSecret()
{
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	removeOldSecrets(now);

	static constexpr size_t DATA_LEN = 64;
	std::array<uint32_t, DATA_LEN> data;
#ifdef HAVE_QT_RANDOM_GENERATOR
	QRandomGenerator::global()->generate(data.data(), data.data() + DATA_LEN);
#else
	for(size_t i=0; i<DATA_LEN; i++)
		data[i] = std::rand();
#endif
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 3
	hash.addData(QByteArrayView(reinterpret_cast<const char*>(data.data()), DATA_LEN * sizeof(data[0])));
#else
	hash.addData(reinterpret_cast<const char*>(data.data()), DATA_LEN * sizeof(data[0]));
#endif
	Secret sc;
	sc.createdMsec = now;
	sc.secret = hash.result().toHex().constData();
	m_secretList.push_back(sc);
	return sc.secret;
}

void TunnelSecretList::removeOldSecrets(int64_t now)
{
	m_secretList.erase(
		std::remove_if(m_secretList.begin(),
						m_secretList.end(),
						[now](const Secret &sc){
							int64_t age = now - sc.createdMsec;
							return age < 0 || age > max_age_msec;
						}),
		m_secretList.end()
	);
}

}
