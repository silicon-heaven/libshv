#pragma once

#include <shv/core/shvcore_export.h>

#include <limits>
#include <functional>
#include <cstdint>
#include <cstddef>
#include <string>


namespace shv::core::utils {

class LIBSHVCORE_EXPORT Crypt
{
public:
	using Generator = std::function< uint32_t (uint32_t) >;
public:
	Crypt(const Generator& gen = nullptr);
public:
	static Generator createGenerator(uint32_t a, uint32_t b, uint32_t max_rand);

	/// any of function, functor or lambda can be set as a random number generator
	void setGenerator(const Generator& gen);

	/// @a min_length minimal length of digest
	/// @return string crypted by 0-9, A-Z, a-z characters
	std::string encrypt(const std::string &data, size_t min_length = 16) const;

	std::string decrypt(const std::string &data) const;
private:
	std::string decodeArray(const std::string &ba) const;
private:
	Generator m_generator;
};

}

