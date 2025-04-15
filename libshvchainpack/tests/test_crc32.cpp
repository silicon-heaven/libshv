#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <shv/chainpack/crc32.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <zlib.h>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

DOCTEST_TEST_CASE("CRC SHV3")
{
	shv::chainpack::Crc32Shv3 crc;
	vector<uint8_t> data = { 0x01,0x8B,0x41,0x41,0x48,0x41,0x49,0x86,0x07,0x66,0x6F,0x6F,0x2F,0x62,0x61,0x72,0x4A,0x86,0x03,0x62,0x61,0x7A,0xFF,0x8A,0x41,0x86,0x05,0x68,0x65,0x6C,0x6C,0x6F,0xFF };
	crc.add(data.data(), data.size());
	auto my_crc = crc.finalize();

	REQUIRE(0xd7a14e96 == my_crc);
}

DOCTEST_TEST_CASE("CRC on strings")
{
	for(const auto &s : {
		""s,
		"a"s,
		"ab"s,
		"auto data = reinterpret_cast<const unsigned char*>(s.data());"s,
		"\0\0\0\0\0\0\0\0\0\0\0\0"s,
	}) {
		auto zip_crc = crc32(0L, Z_NULL, 0);
		auto data = reinterpret_cast<const unsigned char*>(s.data());
		zip_crc = crc32(zip_crc, data, static_cast<unsigned int>(s.size()));

		shv::chainpack::Crc32 crc;
		crc.add(s.data(), s.size());
		CAPTURE(s);
		REQUIRE(zip_crc == crc.finalize());
	}
}

