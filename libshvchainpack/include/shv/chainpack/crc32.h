#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace shv::chainpack {

using crc32_t = uint32_t;

// Reflected polynomial for CRC-32/ISO-HDLC
constexpr crc32_t CRC_POSIX_POLY_REV = 0xEDB88320L;

namespace {
template<crc32_t POLY>
consteval auto make_table() {
	auto crc32_for_byte = [](uint8_t b)
	{
		crc32_t r = b;
		for (int j = 0; j < 8; ++j) {
			if (r & 1)
				r = (r >> 1) ^ POLY;
			else
				r >>= 1;
		}
		return r;
	};
	std::array<crc32_t, 256> table{};
	for (crc32_t i = 0; i < 256; ++i) {
		table[i] = crc32_for_byte(static_cast<uint8_t>(i));
	}
	return table;
}
}

template<crc32_t POLY>
class Crc32
{
public:
	void add(uint8_t b) {
		const auto ix = static_cast<uint8_t>(m_remainder ^ b);
		m_remainder = m_table[ix] ^ (m_remainder >> 8);
	}

	void add(const void* data, size_t n) {
		auto* p = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < n; ++i) {
			add(p[i]);
		}
	}

	// Standard CRC-32 remainder finalization
	crc32_t result() const { return m_remainder ^ 0xFFFFFFFF; }
	void reset() { m_remainder = 0xFFFFFFFF; }

private:
	crc32_t m_remainder = 0xFFFFFFFF;
	static constexpr auto m_table = make_table<POLY>();
};

using Crc32Posix = Crc32<CRC_POSIX_POLY_REV>;
using Crc32Shv3 = Crc32Posix;

}
