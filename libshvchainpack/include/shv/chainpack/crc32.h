#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shv::chainpack {

using crc32_t = uint32_t;
constexpr crc32_t CRC_POSIX_POLY_REV = 0xEDB88320L;

template<crc32_t POLY_REV>
class Crc32
{
public:
	constexpr Crc32() {
		auto crc32_for_byte = [](uint8_t b)
		{
			constexpr auto MSB_MASK = static_cast<crc32_t>(0xFF) << ((sizeof(crc32_t) - 1) * 8);
			auto r = static_cast<crc32_t>(b);
			for(int j = 0; j < 8; ++j)
				r = (r & 1? 0: POLY_REV) ^ (r >> 1);
			return r ^ MSB_MASK;
		};
		for(crc32_t i = 0; i < 0x100; ++i) {
			m_table[i] = crc32_for_byte(static_cast<uint8_t>(i));
		}
	}

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
private:
	crc32_t m_remainder = 0;
	crc32_t m_table[256];
};

using Crc32Posix = Crc32<CRC_POSIX_POLY_REV>;

}
