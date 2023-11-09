#include <shv/iotqt/utils/network.h>

#include <shv/coreqt/log.h>

#include <QAbstractSocket>
#include <QNetworkInterface>

namespace shv::iotqt::utils {

uint32_t Network::toIntIPv4Address(const std::string &addr)
{
	uint32_t ret = 0;
	std::istringstream is(addr);
	while(is) {
		std::array<char, 32> s;
		is.getline(s.data(), s.size(), '.');
		int i = std::atoi(s.data());
		ret = (ret << 8) + static_cast<uint8_t>(i);
	}
	return ret;
}

bool Network::isGlobalIPv4Address(uint32_t addr)
{
	// This is an IPv4 address or an IPv6 v4-mapped address includes all
	 // IPv6 v4-compat addresses, except for ::ffff:0.0.0.0 (because `a' is
	 // zero). See setAddress(quint8*) below, which calls convertToIpv4(),
	 // for details.
	 // Source: RFC 5735
	 if ((addr & 0xff000000U) == 0x7f000000U)   // 127.0.0.0/8
		 return false;//LoopbackAddress;
	 if ((addr & 0xf0000000U) == 0xe0000000U)   // 224.0.0.0/4
		 return false;//MulticastAddress;
	 if ((addr & 0xffff0000U) == 0xa9fe0000U)   // 169.254.0.0/16
		 return false;//LinkLocalAddress;
	 if ((addr & 0xff000000U) == 0)             // 0.0.0.0/8 except 0.0.0.0 (handled below)
		 return false;//LocalNetAddress;
	 if ((addr & 0xf0000000U) == 0xf0000000U) { // 240.0.0.0/4
		 if (addr == 0xffffffffU)               // 255.255.255.255
			 return false;//BroadcastAddress;
		 return false;//UnknownAddress;
	 }
	 // Not testing for PrivateNetworkAddress and TestNetworkAddress
	 // since we don't need them yet.
	return true;
}

bool Network::isPublicIPv4Address(uint32_t addr)
{
	if(isGlobalIPv4Address(addr)) {
		if((addr & 0xFF000000) == 0x0A000000) // 10.0.0.0/8
			return false;
		if((addr & 0xFFF00000) == 0xAC100000) // 172.16.0.0/12
			return false;
		if((addr & 0xFFFF0000) == 0xC0A80000) // 192.168.0.0/16
			return false;
		return true;
	}
	return false;
}

QHostAddress Network::primaryPublicIPv4Address()
{
#ifndef Q_OS_WASM
	QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
	for(const QHostAddress &addr : addrs) {
		if(addr.protocol() == QAbstractSocket::IPv4Protocol) {
			quint32 a = addr.toIPv4Address();
			if(isPublicIPv4Address(a)) {
				return addr;
			}
		}
	}
#endif
	return QHostAddress();
}

QHostAddress Network::primaryIPv4Address()
{
#ifndef Q_OS_WASM
	QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
	for(const QHostAddress &addr : addrs) {
		if(addr.protocol() == QAbstractSocket::IPv4Protocol) {
			quint32 a = addr.toIPv4Address();
			if(isGlobalIPv4Address(a)) {
				return addr;
			}
		}
	}
	return QHostAddress(QHostAddress::LocalHost);
#else
	return QHostAddress();
#endif
}

}
