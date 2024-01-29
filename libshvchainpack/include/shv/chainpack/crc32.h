#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace shv::chainpack {

using crc32_t = uint32_t;

// CRC-32/ISO-HDLC
constexpr crc32_t CRC_POSIX_POLY_REV = 0xEDB88320L;

template<crc32_t POLY>
consteval auto make_table() {
	auto crc32_for_byte = [](uint8_t b)
	{
		constexpr auto MSB_MASK = static_cast<crc32_t>(0xFF) << ((sizeof(crc32_t) - 1) * 8);
		auto r = static_cast<crc32_t>(b);
		for(int j = 0; j < 8; ++j)
			r = (r & 1? 0: POLY) ^ (r >> 1);
		return r ^ MSB_MASK;
	};
	std::array<crc32_t, 256> table;
	for(crc32_t i = 0; i < 0x100; ++i) {
		table[i] = crc32_for_byte(static_cast<uint8_t>(i));
	}
	return table;
}

template<crc32_t POLY>
class Crc32
{
public:
	void add(uint8_t b) {
		const auto ix = static_cast<uint8_t>(m_remainder ^ static_cast<crc32_t>(b));
		m_remainder = m_table[ix] ^ (m_remainder >> 8);
	}
	void add(const void *data, size_t n) {
		for (size_t i = 0; i < n; ++i) {
			add(static_cast<const uint8_t*>(data)[i]);
		}
	}
	crc32_t remainder() const { return m_remainder; }
	void setRemainder(crc32_t rem) { m_remainder = rem; }
	void reset() { setRemainder(0); }
private:
	crc32_t m_remainder = 0;
	static constexpr auto m_table = make_table<POLY>();
};

using Crc32Posix = Crc32<CRC_POSIX_POLY_REV>;
using Crc32Shv3 = Crc32Posix;

}
